#include "renderer_opengl.h"

#include <GL/gl.h>
#include <GL/glext.h>

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (APIENTRYP HTHPFNGLDRAWELEMENTSPROC)(GLenum mode, GLsizei count,
                                                  GLenum type,
                                                  const void *indices);

typedef struct {
    PFNGLCREATESHADERPROC create_shader;
    PFNGLSHADERSOURCEPROC shader_source;
    PFNGLCOMPILESHADERPROC compile_shader;
    PFNGLGETSHADERIVPROC get_shader_iv;
    PFNGLGETSHADERINFOLOGPROC get_shader_info_log;
    PFNGLDELETESHADERPROC delete_shader;
    PFNGLCREATEPROGRAMPROC create_program;
    PFNGLATTACHSHADERPROC attach_shader;
    PFNGLLINKPROGRAMPROC link_program;
    PFNGLGETPROGRAMIVPROC get_program_iv;
    PFNGLGETPROGRAMINFOLOGPROC get_program_info_log;
    PFNGLUSEPROGRAMPROC use_program;
    PFNGLDELETEPROGRAMPROC delete_program;
    PFNGLGETUNIFORMLOCATIONPROC get_uniform_location;
    PFNGLUNIFORMMATRIX4FVPROC uniform_matrix_4fv;
    PFNGLUNIFORM4FVPROC uniform_4fv;
    PFNGLUNIFORM1IPROC uniform_1i;
    PFNGLACTIVETEXTUREPROC active_texture;
    PFNGLGENVERTEXARRAYSPROC gen_vertex_arrays;
    PFNGLBINDVERTEXARRAYPROC bind_vertex_array;
    PFNGLDELETEVERTEXARRAYSPROC delete_vertex_arrays;
    PFNGLGENBUFFERSPROC gen_buffers;
    PFNGLBINDBUFFERPROC bind_buffer;
    PFNGLBUFFERDATAPROC buffer_data;
    PFNGLDELETEBUFFERSPROC delete_buffers;
    PFNGLENABLEVERTEXATTRIBARRAYPROC enable_vertex_attrib_array;
    PFNGLVERTEXATTRIBPOINTERPROC vertex_attrib_pointer;
    HTHPFNGLDRAWELEMENTSPROC draw_elements;
} HTHOpenGLFunctions;

typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei index_count;
} HTHOpenGLGeometry;

typedef struct {
    HTHGeometryPrimitive primitive;
    float base_color[4];
    GLuint texture;
    bool has_texture;
} HTHOpenGLRuntimeDraw;

struct HTHOpenGLBackend {
    HTHPlatform *platform;
    HTHPlatformGraphicsContext *context;
    HTHOpenGLFunctions gl;
    GLuint program;
    HTHOpenGLGeometry geometries[HTH_GEOMETRY_PRIMITIVE_COUNT];
    GLint model_location;
    GLint view_location;
    GLint projection_location;
    GLint base_color_location;
    GLint use_texture_location;
    GLint base_texture_location;
    HTHOpenGLRuntimeDraw *static_draws;
    HTHMat4 *static_models;
    size_t static_draw_count;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
};

static const GLchar vertex_shader_source[] =
    "#version 330 core\n"
    "layout(location = 0) in vec3 a_position;\n"
    "layout(location = 1) in vec2 a_uv;\n"
    "uniform mat4 u_model;\n"
    "uniform mat4 u_view;\n"
    "uniform mat4 u_projection;\n"
    "out vec2 v_uv;\n"
    "void main()\n"
    "{\n"
    "    v_uv = a_uv;\n"
    "    gl_Position = u_projection * u_view * u_model *\n"
    "                  vec4(a_position, 1.0);\n"
    "}\n";

static const GLchar fragment_shader_source[] =
    "#version 330 core\n"
    "uniform vec4 u_base_color;\n"
    "uniform int u_use_texture;\n"
    "uniform sampler2D u_base_texture;\n"
    "in vec2 v_uv;\n"
    "out vec4 fragment_color;\n"
    "void main()\n"
    "{\n"
    "    vec4 surface = u_base_color;\n"
    "    if (u_use_texture == 1) {\n"
    "        surface *= texture(u_base_texture, v_uv);\n"
    "    }\n"
    "    fragment_color = surface;\n"
    "}\n";

#define HTH_LOAD_GL_FUNCTION(backend, member, type, name)                   \
    do {                                                                    \
        HTHGraphicsProcedure procedure =                                    \
            hth_platform_graphics_get_proc_address((backend)->platform,     \
                                                   (name));                  \
        if (procedure == NULL) {                                             \
            fprintf(stderr, "Renderer initialization failed: missing "      \
                    "OpenGL function %s.\n", (name));                       \
            return false;                                                    \
        }                                                                    \
        (backend)->gl.member = (type)procedure;                              \
    } while (0)

static const char *gl_string(GLenum name)
{
    const GLubyte *value = glGetString(name);
    return value != NULL ? (const char *)value : "unavailable";
}

static bool load_gl_functions(HTHOpenGLBackend *backend)
{
    HTH_LOAD_GL_FUNCTION(backend, create_shader,
                         PFNGLCREATESHADERPROC, "glCreateShader");
    HTH_LOAD_GL_FUNCTION(backend, shader_source,
                         PFNGLSHADERSOURCEPROC, "glShaderSource");
    HTH_LOAD_GL_FUNCTION(backend, compile_shader,
                         PFNGLCOMPILESHADERPROC, "glCompileShader");
    HTH_LOAD_GL_FUNCTION(backend, get_shader_iv,
                         PFNGLGETSHADERIVPROC, "glGetShaderiv");
    HTH_LOAD_GL_FUNCTION(backend, get_shader_info_log,
                         PFNGLGETSHADERINFOLOGPROC, "glGetShaderInfoLog");
    HTH_LOAD_GL_FUNCTION(backend, delete_shader,
                         PFNGLDELETESHADERPROC, "glDeleteShader");
    HTH_LOAD_GL_FUNCTION(backend, create_program,
                         PFNGLCREATEPROGRAMPROC, "glCreateProgram");
    HTH_LOAD_GL_FUNCTION(backend, attach_shader,
                         PFNGLATTACHSHADERPROC, "glAttachShader");
    HTH_LOAD_GL_FUNCTION(backend, link_program,
                         PFNGLLINKPROGRAMPROC, "glLinkProgram");
    HTH_LOAD_GL_FUNCTION(backend, get_program_iv,
                         PFNGLGETPROGRAMIVPROC, "glGetProgramiv");
    HTH_LOAD_GL_FUNCTION(backend, get_program_info_log,
                         PFNGLGETPROGRAMINFOLOGPROC, "glGetProgramInfoLog");
    HTH_LOAD_GL_FUNCTION(backend, use_program,
                         PFNGLUSEPROGRAMPROC, "glUseProgram");
    HTH_LOAD_GL_FUNCTION(backend, delete_program,
                         PFNGLDELETEPROGRAMPROC, "glDeleteProgram");
    HTH_LOAD_GL_FUNCTION(backend, get_uniform_location,
                         PFNGLGETUNIFORMLOCATIONPROC, "glGetUniformLocation");
    HTH_LOAD_GL_FUNCTION(backend, uniform_matrix_4fv,
                         PFNGLUNIFORMMATRIX4FVPROC, "glUniformMatrix4fv");
    HTH_LOAD_GL_FUNCTION(backend, uniform_4fv,
                         PFNGLUNIFORM4FVPROC, "glUniform4fv");
    HTH_LOAD_GL_FUNCTION(backend, uniform_1i,
                         PFNGLUNIFORM1IPROC, "glUniform1i");
    HTH_LOAD_GL_FUNCTION(backend, active_texture,
                         PFNGLACTIVETEXTUREPROC, "glActiveTexture");
    HTH_LOAD_GL_FUNCTION(backend, gen_vertex_arrays,
                         PFNGLGENVERTEXARRAYSPROC, "glGenVertexArrays");
    HTH_LOAD_GL_FUNCTION(backend, bind_vertex_array,
                         PFNGLBINDVERTEXARRAYPROC, "glBindVertexArray");
    HTH_LOAD_GL_FUNCTION(backend, delete_vertex_arrays,
                         PFNGLDELETEVERTEXARRAYSPROC, "glDeleteVertexArrays");
    HTH_LOAD_GL_FUNCTION(backend, gen_buffers,
                         PFNGLGENBUFFERSPROC, "glGenBuffers");
    HTH_LOAD_GL_FUNCTION(backend, bind_buffer,
                         PFNGLBINDBUFFERPROC, "glBindBuffer");
    HTH_LOAD_GL_FUNCTION(backend, buffer_data,
                         PFNGLBUFFERDATAPROC, "glBufferData");
    HTH_LOAD_GL_FUNCTION(backend, delete_buffers,
                         PFNGLDELETEBUFFERSPROC, "glDeleteBuffers");
    HTH_LOAD_GL_FUNCTION(backend, enable_vertex_attrib_array,
                         PFNGLENABLEVERTEXATTRIBARRAYPROC,
                         "glEnableVertexAttribArray");
    HTH_LOAD_GL_FUNCTION(backend, vertex_attrib_pointer,
                         PFNGLVERTEXATTRIBPOINTERPROC, "glVertexAttribPointer");
    HTH_LOAD_GL_FUNCTION(backend, draw_elements,
                         HTHPFNGLDRAWELEMENTSPROC, "glDrawElements");
    return true;
}

#undef HTH_LOAD_GL_FUNCTION

static void print_shader_log(HTHOpenGLBackend *backend, GLuint shader,
                             const char *stage)
{
    GLchar *log;
    GLint length = 0;

    backend->gl.get_shader_iv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return;
    }
    log = malloc((size_t)length);
    if (log == NULL) {
        fprintf(stderr, "%s shader info log unavailable: allocation failed.\n",
                stage);
        return;
    }
    backend->gl.get_shader_info_log(shader, length, NULL, log);
    fprintf(stderr, "%s shader info log:\n%s\n", stage, log);
    free(log);
}

static GLuint compile_shader(HTHOpenGLBackend *backend, GLenum type,
                             const GLchar *source, const char *stage)
{
    GLint compiled = GL_FALSE;
    GLuint shader = backend->gl.create_shader(type);

    if (shader == 0) {
        fprintf(stderr, "%s shader compilation failed: creation failed.\n",
                stage);
        return 0;
    }
    backend->gl.shader_source(shader, 1, &source, NULL);
    backend->gl.compile_shader(shader);
    backend->gl.get_shader_iv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        fprintf(stderr, "%s shader compilation failed.\n", stage);
        print_shader_log(backend, shader, stage);
        backend->gl.delete_shader(shader);
        return 0;
    }
    return shader;
}

static void print_program_log(HTHOpenGLBackend *backend, GLuint program)
{
    GLchar *log;
    GLint length = 0;

    backend->gl.get_program_iv(program, GL_INFO_LOG_LENGTH, &length);
    if (length <= 1) {
        return;
    }
    log = malloc((size_t)length);
    if (log == NULL) {
        fputs("Program info log unavailable: allocation failed.\n", stderr);
        return;
    }
    backend->gl.get_program_info_log(program, length, NULL, log);
    fprintf(stderr, "Program link info log:\n%s\n", log);
    free(log);
}

static GLuint create_program(HTHOpenGLBackend *backend)
{
    GLint linked = GL_FALSE;
    GLuint fragment_shader;
    GLuint program;
    GLuint vertex_shader;

    vertex_shader = compile_shader(backend, GL_VERTEX_SHADER,
                                   vertex_shader_source, "Vertex");
    if (vertex_shader == 0) {
        return 0;
    }
    fragment_shader = compile_shader(backend, GL_FRAGMENT_SHADER,
                                     fragment_shader_source, "Fragment");
    if (fragment_shader == 0) {
        backend->gl.delete_shader(vertex_shader);
        return 0;
    }

    program = backend->gl.create_program();
    if (program == 0) {
        fputs("Program link failed: creation failed.\n", stderr);
        backend->gl.delete_shader(fragment_shader);
        backend->gl.delete_shader(vertex_shader);
        return 0;
    }
    backend->gl.attach_shader(program, vertex_shader);
    backend->gl.attach_shader(program, fragment_shader);
    backend->gl.link_program(program);
    backend->gl.get_program_iv(program, GL_LINK_STATUS, &linked);
    backend->gl.delete_shader(fragment_shader);
    backend->gl.delete_shader(vertex_shader);
    if (linked == GL_FALSE) {
        fputs("Program link failed.\n", stderr);
        print_program_log(backend, program);
        backend->gl.delete_program(program);
        return 0;
    }
    return program;
}

static bool upload_geometry(HTHOpenGLBackend *backend,
                            HTHGeometryPrimitive primitive)
{
    HTHGeometryView source;
    HTHOpenGLGeometry *geometry;
    size_t index_bytes;
    size_t vertex_bytes;

    if (primitive < HTH_GEOMETRY_PRIMITIVE_BOX ||
        primitive >= HTH_GEOMETRY_PRIMITIVE_COUNT ||
        !hth_geometry_get(primitive, &source) ||
        source.vertex_count > SIZE_MAX / sizeof(*source.vertices) ||
        source.index_count > SIZE_MAX / sizeof(*source.indices)) {
        return false;
    }
    vertex_bytes = source.vertex_count * sizeof(*source.vertices);
    index_bytes = source.index_count * sizeof(*source.indices);
    if (vertex_bytes > (size_t)PTRDIFF_MAX ||
        index_bytes > (size_t)PTRDIFF_MAX || source.index_count > INT_MAX) {
        return false;
    }
    geometry = &backend->geometries[primitive];
    backend->gl.gen_vertex_arrays(1, &geometry->vao);
    backend->gl.gen_buffers(1, &geometry->vbo);
    backend->gl.gen_buffers(1, &geometry->ebo);
    if (geometry->vao == 0 || geometry->vbo == 0 || geometry->ebo == 0) {
        fputs("Renderer initialization failed: VAO/VBO/EBO creation failed.\n",
              stderr);
        return false;
    }

    backend->gl.bind_vertex_array(geometry->vao);
    backend->gl.bind_buffer(GL_ARRAY_BUFFER, geometry->vbo);
    backend->gl.buffer_data(GL_ARRAY_BUFFER, (GLsizeiptr)vertex_bytes,
                            source.vertices, GL_STATIC_DRAW);
    backend->gl.bind_buffer(GL_ELEMENT_ARRAY_BUFFER, geometry->ebo);
    backend->gl.buffer_data(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)index_bytes,
                            source.indices, GL_STATIC_DRAW);
    backend->gl.enable_vertex_attrib_array(0);
    backend->gl.vertex_attrib_pointer(
        0, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(HTHRenderVertex),
        (const void *)offsetof(HTHRenderVertex, position));
    backend->gl.enable_vertex_attrib_array(1);
    backend->gl.vertex_attrib_pointer(
        1, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(HTHRenderVertex),
        (const void *)offsetof(HTHRenderVertex, uv));
    backend->gl.bind_vertex_array(0);
    backend->gl.bind_buffer(GL_ARRAY_BUFFER, 0);
    geometry->index_count = (GLsizei)source.index_count;
    if (glGetError() != GL_NO_ERROR) {
        fputs("Renderer initialization failed: indexed geometry setup "
              "failed.\n", stderr);
        return false;
    }
    return true;
}

static bool create_geometry(HTHOpenGLBackend *backend)
{
    int primitive;

    for (primitive = 0; primitive < (int)HTH_GEOMETRY_PRIMITIVE_COUNT;
         ++primitive) {
        if (!upload_geometry(backend, (HTHGeometryPrimitive)primitive)) {
            return false;
        }
    }
    return true;
}

static bool upload_texture(HTHOpenGLBackend *backend,
                           const HTHRendererStaticDraw *draw,
                           GLuint *out_texture)
{
    GLint previous_unpack_alignment = 4;
    GLuint texture = 0;

    if (draw->texture_pixels == NULL || draw->texture_width == 0U ||
        draw->texture_height == 0U || draw->texture_width > INT_MAX ||
        draw->texture_height > INT_MAX) {
        return false;
    }
    glGenTextures(1, &texture);
    if (texture == 0U) {
        return false;
    }
    backend->gl.active_texture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, (GLsizei)draw->texture_width,
                 (GLsizei)draw->texture_height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, draw->texture_pixels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previous_unpack_alignment);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &texture);
        return false;
    }
    *out_texture = texture;
    return true;
}

static bool create_static_models(HTHOpenGLBackend *backend,
                                 const HTHRendererStaticDraw *draws,
                                 size_t draw_count)
{
    size_t index;

    if (draw_count == 0U) {
        return true;
    }
    if (draws == NULL) {
        return false;
    }
    if (draw_count > SIZE_MAX / sizeof(*backend->static_draws) ||
        draw_count > SIZE_MAX / sizeof(*backend->static_models)) {
        return false;
    }
    backend->static_draws = calloc(
        draw_count, sizeof(*backend->static_draws));
    backend->static_models = malloc(
        draw_count * sizeof(*backend->static_models));
    if (backend->static_draws == NULL || backend->static_models == NULL) {
        return false;
    }
    backend->static_draw_count = draw_count;
    for (index = 0; index < draw_count; ++index) {
        const HTHAABB *bounds = &draws[index].bounds;
        HTHMat4 model = hth_mat4_identity();
        size_t component;

        if (!hth_aabb_is_valid(bounds)) {
            return false;
        }
        if (draws[index].primitive < HTH_GEOMETRY_PRIMITIVE_BOX ||
            draws[index].primitive >= HTH_GEOMETRY_PRIMITIVE_COUNT) {
            return false;
        }
        for (component = 0U; component < 4U; ++component) {
            if (!isfinite(draws[index].base_color[component]) ||
                draws[index].base_color[component] < 0.0F ||
                draws[index].base_color[component] > 1.0F) {
                return false;
            }
        }
        memcpy(backend->static_draws[index].base_color,
               draws[index].base_color,
               sizeof(backend->static_draws[index].base_color));
        backend->static_draws[index].primitive = draws[index].primitive;
        backend->static_draws[index].has_texture = draws[index].has_texture;
        if (draws[index].has_texture &&
            !upload_texture(backend, &draws[index],
                            &backend->static_draws[index].texture)) {
            return false;
        }
        model.elements[0] = bounds->max.x - bounds->min.x;
        model.elements[5] = bounds->max.y - bounds->min.y;
        model.elements[10] = bounds->max.z - bounds->min.z;
        model.elements[12] = (bounds->min.x + bounds->max.x) * 0.5F;
        model.elements[13] = (bounds->min.y + bounds->max.y) * 0.5F;
        model.elements[14] = (bounds->min.z + bounds->max.z) * 0.5F;
        backend->static_models[index] = model;
    }
    return true;
}

static bool validate_context(void)
{
    bool core_profile = false;
    int depth_bits = 0;
    int double_buffer = 0;
    int major = 0;
    int minor = 0;
    int stencil_bits = 0;

    if (!hth_platform_graphics_context_info(
            &major, &minor, &core_profile, &double_buffer,
            &depth_bits, &stencil_bits) ||
        major < 3 || (major == 3 && minor < 3) || !core_profile) {
        fprintf(stderr,
                "Renderer initialization failed: OpenGL 3.3 Core required; "
                "received %d.%d%s.\n",
                major, minor, core_profile ? " Core" : " non-Core");
        return false;
    }
    if (double_buffer == 0 || depth_bits < 24 || stencil_bits < 8) {
        fprintf(stderr,
                "Renderer initialization failed: double buffer with 24-bit "
                "depth and 8-bit stencil required; received double=%d, "
                "depth=%d, stencil=%d.\n",
                double_buffer, depth_bits, stencil_bits);
        return false;
    }
    return true;
}

static bool cache_uniform_locations(HTHOpenGLBackend *backend)
{
    backend->model_location =
        backend->gl.get_uniform_location(backend->program, "u_model");
    backend->view_location =
        backend->gl.get_uniform_location(backend->program, "u_view");
    backend->projection_location =
        backend->gl.get_uniform_location(backend->program, "u_projection");
    backend->base_color_location =
        backend->gl.get_uniform_location(backend->program, "u_base_color");
    backend->use_texture_location =
        backend->gl.get_uniform_location(backend->program, "u_use_texture");
    backend->base_texture_location =
        backend->gl.get_uniform_location(backend->program, "u_base_texture");
    if (backend->model_location < 0 || backend->view_location < 0 ||
        backend->projection_location < 0 ||
        backend->base_color_location < 0 ||
        backend->use_texture_location < 0 ||
        backend->base_texture_location < 0) {
        fprintf(stderr,
                "Renderer initialization failed: required uniform missing "
                "(model=%d, view=%d, projection=%d, color=%d, use=%d, "
                "sampler=%d).\n",
                backend->model_location, backend->view_location,
                backend->projection_location, backend->base_color_location,
                backend->use_texture_location,
                backend->base_texture_location);
        return false;
    }
    return true;
}

static bool initialize_sampler(HTHOpenGLBackend *backend)
{
    backend->gl.use_program(backend->program);
    backend->gl.uniform_1i(backend->base_texture_location, 0);
    backend->gl.use_program(0);
    return glGetError() == GL_NO_ERROR;
}

HTHOpenGLBackend *hth_renderer_opengl_create(
    HTHPlatform *platform, const HTHRendererStaticDraw *draws,
    size_t draw_count)
{
    HTHOpenGLBackend *backend = calloc(1, sizeof(*backend));

    if (backend == NULL) {
        fputs("Renderer initialization failed: backend allocation failed.\n",
              stderr);
        return NULL;
    }
    backend->platform = platform;
    backend->context = hth_platform_graphics_create_context(platform);
    if (backend->context == NULL ||
        !hth_platform_graphics_make_current(platform, backend->context)) {
        fputs("Renderer initialization failed: OpenGL context unavailable.\n",
              stderr);
        hth_renderer_opengl_destroy(backend);
        return NULL;
    }
    if (!validate_context() || !load_gl_functions(backend)) {
        hth_renderer_opengl_destroy(backend);
        return NULL;
    }

    if (!hth_platform_graphics_set_swap_interval(0)) {
        fputs("Warning: could not disable OpenGL VSync; continuing with "
              "engine frame pacing.\n", stderr);
    }
    glClearColor(0.05F, 0.02F, 0.08F, 1.0F);
    glClearDepth(1.0);
    glClearStencil(0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    backend->program = create_program(backend);
    if (backend->program == 0 || !cache_uniform_locations(backend) ||
        !initialize_sampler(backend) ||
        !create_geometry(backend) ||
        !create_static_models(backend, draws, draw_count) ||
        glGetError() != GL_NO_ERROR) {
        fputs("Renderer initialization failed: graphics pipeline setup "
              "failed.\n", stderr);
        hth_renderer_opengl_destroy(backend);
        return NULL;
    }
    puts("Renderer backend: OpenGL");
    printf("OpenGL version: %s\n", gl_string(GL_VERSION));
    printf("OpenGL renderer: %s\n", gl_string(GL_RENDERER));
    printf("OpenGL vendor: %s\n", gl_string(GL_VENDOR));
    printf("GLSL version: %s\n", gl_string(GL_SHADING_LANGUAGE_VERSION));
    puts("Graphics pipeline initialized.");
    puts("Renderer initialized.");
    return backend;
}

void hth_renderer_opengl_destroy(HTHOpenGLBackend *backend)
{
    size_t geometry_index;
    size_t index;

    if (backend == NULL) {
        return;
    }
    if (backend->context != NULL) {
        if (backend->gl.use_program != NULL) {
            backend->gl.use_program(0);
        }
        for (index = 0U; index < backend->static_draw_count; ++index) {
            if (backend->static_draws != NULL &&
                backend->static_draws[index].texture != 0U) {
                glDeleteTextures(1, &backend->static_draws[index].texture);
            }
        }
        for (geometry_index = 0U;
             geometry_index < HTH_GEOMETRY_PRIMITIVE_COUNT;
             ++geometry_index) {
            HTHOpenGLGeometry *geometry =
                &backend->geometries[geometry_index];

            if (geometry->ebo != 0U && backend->gl.delete_buffers != NULL) {
                backend->gl.delete_buffers(1, &geometry->ebo);
            }
            if (geometry->vbo != 0U && backend->gl.delete_buffers != NULL) {
                backend->gl.delete_buffers(1, &geometry->vbo);
            }
            if (geometry->vao != 0U &&
                backend->gl.delete_vertex_arrays != NULL) {
                backend->gl.delete_vertex_arrays(1, &geometry->vao);
            }
        }
        if (backend->program != 0 && backend->gl.delete_program != NULL) {
            backend->gl.delete_program(backend->program);
        }
    }
    hth_platform_graphics_destroy_context(backend->context);
    free(backend->static_draws);
    free(backend->static_models);
    free(backend);
}

bool hth_renderer_opengl_resize(HTHOpenGLBackend *backend,
                                uint32_t width, uint32_t height)
{
    if (backend == NULL || width == 0 || height == 0 ||
        width > INT_MAX || height > INT_MAX) {
        return false;
    }
    backend->framebuffer_width = width;
    backend->framebuffer_height = height;
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    return glGetError() == GL_NO_ERROR;
}

bool hth_renderer_opengl_set_camera_matrices(HTHOpenGLBackend *backend,
                                             const HTHMat4 *view,
                                             const HTHMat4 *projection)
{
    if (backend == NULL || view == NULL || projection == NULL) {
        return false;
    }

    backend->gl.use_program(backend->program);
    backend->gl.uniform_matrix_4fv(backend->view_location, 1, GL_FALSE,
                                   view->elements);
    backend->gl.uniform_matrix_4fv(backend->projection_location, 1, GL_FALSE,
                                   projection->elements);
    backend->gl.use_program(0);
    return glGetError() == GL_NO_ERROR;
}

bool hth_renderer_opengl_frame(HTHOpenGLBackend *backend)
{
    size_t index;

    if (backend == NULL) {
        return false;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    backend->gl.use_program(backend->program);
    backend->gl.active_texture(GL_TEXTURE0);
    for (index = 0; index < backend->static_draw_count; ++index) {
        const HTHOpenGLRuntimeDraw *draw = &backend->static_draws[index];
        const HTHOpenGLGeometry *geometry =
            &backend->geometries[draw->primitive];

        backend->gl.bind_vertex_array(geometry->vao);
        backend->gl.uniform_matrix_4fv(
            backend->model_location, 1, GL_FALSE,
            backend->static_models[index].elements);
        backend->gl.uniform_4fv(
            backend->base_color_location, 1,
            draw->base_color);
        backend->gl.uniform_1i(
            backend->use_texture_location,
            draw->has_texture ? 1 : 0);
        glBindTexture(GL_TEXTURE_2D, draw->texture);
        backend->gl.draw_elements(GL_TRIANGLES, geometry->index_count,
                                  GL_UNSIGNED_INT, NULL);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    backend->gl.bind_vertex_array(0);
    backend->gl.use_program(0);
    if (glGetError() != GL_NO_ERROR) {
        fputs("Renderer frame failed: OpenGL draw failed.\n", stderr);
        return false;
    }
    if (!hth_platform_graphics_swap(backend->platform)) {
        fputs("Renderer frame/presentation failed.\n", stderr);
        return false;
    }
    return true;
}
