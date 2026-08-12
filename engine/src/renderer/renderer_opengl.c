#include "renderer_opengl.h"

#include <GL/gl.h>
#include <GL/glext.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef void (APIENTRYP HTHPFNGLDRAWARRAYSPROC)(GLenum mode, GLint first,
                                                GLsizei count);

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
    PFNGLGENVERTEXARRAYSPROC gen_vertex_arrays;
    PFNGLBINDVERTEXARRAYPROC bind_vertex_array;
    PFNGLDELETEVERTEXARRAYSPROC delete_vertex_arrays;
    PFNGLGENBUFFERSPROC gen_buffers;
    PFNGLBINDBUFFERPROC bind_buffer;
    PFNGLBUFFERDATAPROC buffer_data;
    PFNGLDELETEBUFFERSPROC delete_buffers;
    PFNGLENABLEVERTEXATTRIBARRAYPROC enable_vertex_attrib_array;
    PFNGLVERTEXATTRIBPOINTERPROC vertex_attrib_pointer;
    HTHPFNGLDRAWARRAYSPROC draw_arrays;
} HTHOpenGLFunctions;

typedef struct {
    GLfloat position[2];
} HTHBootstrapVertex;

struct HTHOpenGLBackend {
    HTHPlatform *platform;
    HTHPlatformGraphicsContext *context;
    HTHOpenGLFunctions gl;
    GLuint program;
    GLuint vao;
    GLuint vbo;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
};

static const GLchar vertex_shader_source[] =
    "#version 330 core\n"
    "layout(location = 0) in vec2 position;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

static const GLchar fragment_shader_source[] =
    "#version 330 core\n"
    "out vec4 fragment_color;\n"
    "void main()\n"
    "{\n"
    "    fragment_color = vec4(0.80, 0.25, 0.40, 1.0);\n"
    "}\n";

static const HTHBootstrapVertex bootstrap_vertices[] = {
    {{ 0.0F,  0.6F}},
    {{-0.6F, -0.5F}},
    {{ 0.6F, -0.5F}},
};

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
    HTH_LOAD_GL_FUNCTION(backend, draw_arrays,
                         HTHPFNGLDRAWARRAYSPROC, "glDrawArrays");
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

static bool create_geometry(HTHOpenGLBackend *backend)
{
    backend->gl.gen_vertex_arrays(1, &backend->vao);
    backend->gl.gen_buffers(1, &backend->vbo);
    if (backend->vao == 0 || backend->vbo == 0) {
        fputs("Renderer initialization failed: VAO/VBO creation failed.\n",
              stderr);
        return false;
    }

    backend->gl.bind_vertex_array(backend->vao);
    backend->gl.bind_buffer(GL_ARRAY_BUFFER, backend->vbo);
    backend->gl.buffer_data(GL_ARRAY_BUFFER,
                            (GLsizeiptr)sizeof(bootstrap_vertices),
                            bootstrap_vertices, GL_STATIC_DRAW);
    backend->gl.enable_vertex_attrib_array(0);
    backend->gl.vertex_attrib_pointer(
        0, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(HTHBootstrapVertex),
        (const void *)offsetof(HTHBootstrapVertex, position));
    backend->gl.bind_buffer(GL_ARRAY_BUFFER, 0);
    backend->gl.bind_vertex_array(0);
    if (glGetError() != GL_NO_ERROR) {
        fputs("Renderer initialization failed: vertex setup failed.\n", stderr);
        return false;
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

HTHOpenGLBackend *hth_renderer_opengl_create(HTHPlatform *platform)
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
    if (!hth_renderer_opengl_resize(backend)) {
        fputs("Renderer initialization failed: invalid framebuffer size.\n",
              stderr);
        hth_renderer_opengl_destroy(backend);
        return NULL;
    }

    glClearColor(0.05F, 0.02F, 0.08F, 1.0F);
    glClearDepth(1.0);
    glClearStencil(0);
    backend->program = create_program(backend);
    if (backend->program == 0 || !create_geometry(backend) ||
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
    if (backend == NULL) {
        return;
    }
    if (backend->context != NULL) {
        if (backend->gl.use_program != NULL) {
            backend->gl.use_program(0);
        }
        if (backend->vbo != 0 && backend->gl.delete_buffers != NULL) {
            backend->gl.delete_buffers(1, &backend->vbo);
        }
        if (backend->vao != 0 && backend->gl.delete_vertex_arrays != NULL) {
            backend->gl.delete_vertex_arrays(1, &backend->vao);
        }
        if (backend->program != 0 && backend->gl.delete_program != NULL) {
            backend->gl.delete_program(backend->program);
        }
    }
    hth_platform_graphics_destroy_context(backend->context);
    free(backend);
}

bool hth_renderer_opengl_resize(HTHOpenGLBackend *backend)
{
    uint32_t height;
    uint32_t width;

    if (backend == NULL ||
        !hth_platform_framebuffer_size(backend->platform, &width, &height) ||
        width > INT_MAX || height > INT_MAX) {
        return false;
    }
    backend->framebuffer_width = width;
    backend->framebuffer_height = height;
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    return glGetError() == GL_NO_ERROR;
}

bool hth_renderer_opengl_frame(HTHOpenGLBackend *backend)
{
    if (backend == NULL) {
        return false;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    backend->gl.use_program(backend->program);
    backend->gl.bind_vertex_array(backend->vao);
    backend->gl.draw_arrays(GL_TRIANGLES, 0, 3);
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
