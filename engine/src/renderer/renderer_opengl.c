#include "renderer_opengl.h"

#include <GL/gl.h>
#include <GL/glext.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct HTHOpenGLBackend {
    HTHPlatform *platform;
    HTHPlatformGraphicsContext *context;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
};

static const char *gl_string(GLenum name)
{
    const GLubyte *value = glGetString(name);
    return value != NULL ? (const char *)value : "unavailable";
}

HTHOpenGLBackend *hth_renderer_opengl_create(HTHPlatform *platform)
{
    HTHOpenGLBackend *backend;
    bool core_profile;
    int depth_bits = 0;
    int double_buffer = 0;
    int major = 0;
    int minor = 0;
    int stencil_bits = 0;
    core_profile = false;

    backend = calloc(1, sizeof(*backend));
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

    if (!hth_platform_graphics_context_info(
            &major, &minor, &core_profile, &double_buffer,
            &depth_bits, &stencil_bits) ||
        major < 3 || (major == 3 && minor < 3) || !core_profile) {
        fprintf(stderr,
                "Renderer initialization failed: OpenGL 3.3 Core required; "
                "received %d.%d%s.\n",
                major, minor, core_profile ? " Core" : " non-Core");
        hth_renderer_opengl_destroy(backend);
        return NULL;
    }
    if (double_buffer == 0 || depth_bits < 24 || stencil_bits < 8) {
        fprintf(stderr,
                "Renderer initialization failed: double buffer with 24-bit "
                "depth and 8-bit stencil required; received double=%d, "
                "depth=%d, stencil=%d.\n",
                double_buffer, depth_bits, stencil_bits);
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
    if (glGetError() != GL_NO_ERROR) {
        fputs("Renderer initialization failed: OpenGL state setup failed.\n",
              stderr);
        hth_renderer_opengl_destroy(backend);
        return NULL;
    }

    puts("Renderer backend: OpenGL");
    printf("OpenGL version: %s\n", gl_string(GL_VERSION));
    printf("OpenGL renderer: %s\n", gl_string(GL_RENDERER));
    printf("OpenGL vendor: %s\n", gl_string(GL_VENDOR));
    printf("GLSL version: %s\n", gl_string(GL_SHADING_LANGUAGE_VERSION));
    puts("Renderer initialized.");
    return backend;
}

void hth_renderer_opengl_destroy(HTHOpenGLBackend *backend)
{
    if (backend == NULL) {
        return;
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
    if (glGetError() != GL_NO_ERROR) {
        fputs("Renderer frame failed: OpenGL clear failed.\n", stderr);
        return false;
    }
    if (!hth_platform_graphics_swap(backend->platform)) {
        fputs("Renderer frame/presentation failed.\n", stderr);
        return false;
    }
    return true;
}
