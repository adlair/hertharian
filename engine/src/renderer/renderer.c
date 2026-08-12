#include "renderer.h"
#include "renderer_opengl.h"

#include <stdio.h>
#include <stdlib.h>

struct HTHRenderer {
    HTHPlatform *platform;
    HTHOpenGLBackend *backend;
    HTHCamera camera;
    bool drawable;
};

HTHRenderer *hth_renderer_create(HTHPlatform *platform,
                                 const HTHCamera *camera)
{
    HTHRenderer *renderer;

    renderer = calloc(1, sizeof(*renderer));
    if (renderer == NULL || platform == NULL || camera == NULL) {
        fputs("Renderer initialization failed: allocation failed.\n", stderr);
        free(renderer);
        return NULL;
    }

    renderer->platform = platform;
    renderer->camera = *camera;
    renderer->backend = hth_renderer_opengl_create(platform);
    if (renderer->backend == NULL) {
        free(renderer);
        return NULL;
    }
    if (!hth_renderer_resize(renderer)) {
        fputs("Renderer initialization failed: invalid camera or framebuffer.\n",
              stderr);
        hth_renderer_destroy(renderer);
        return NULL;
    }
    return renderer;
}

void hth_renderer_destroy(HTHRenderer *renderer)
{
    if (renderer == NULL) {
        return;
    }
    hth_renderer_opengl_destroy(renderer->backend);
    free(renderer);
    puts("Renderer shutdown complete.");
}

bool hth_renderer_resize(HTHRenderer *renderer)
{
    HTHMat4 model;
    HTHMat4 projection;
    HTHMat4 view;
    uint32_t height;
    uint32_t width;

    if (renderer == NULL ||
        !hth_platform_framebuffer_size(renderer->platform, &width, &height)) {
        return false;
    }
    if (width == 0 || height == 0) {
        renderer->drawable = false;
        return true;
    }
    if (!hth_camera_view_matrix(&renderer->camera, &view) ||
        !hth_camera_projection_matrix(&renderer->camera, width, height,
                                      &projection)) {
        return false;
    }
    model = hth_mat4_identity();
    if (!hth_renderer_opengl_resize(renderer->backend, width, height) ||
        !hth_renderer_opengl_set_matrices(renderer->backend, &model, &view,
                                          &projection)) {
        return false;
    }
    renderer->drawable = true;
    return true;
}

bool hth_renderer_frame(HTHRenderer *renderer)
{
    return renderer != NULL &&
           (!renderer->drawable ||
            hth_renderer_opengl_frame(renderer->backend));
}
