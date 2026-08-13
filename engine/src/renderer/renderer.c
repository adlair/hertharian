#include "renderer.h"
#include "renderer_opengl.h"

#include <stdio.h>
#include <stdlib.h>

struct HTHRenderer {
    HTHPlatform *platform;
    HTHOpenGLBackend *backend;
    HTHCamera camera;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    bool drawable;
};

static bool update_camera_matrices(HTHRenderer *renderer)
{
    HTHMat4 projection;
    HTHMat4 view;

    if (!hth_camera_view_matrix(&renderer->camera, &view) ||
        !hth_camera_projection_matrix(&renderer->camera,
                                      renderer->framebuffer_width,
                                      renderer->framebuffer_height,
                                      &projection)) {
        return false;
    }
    return hth_renderer_opengl_set_camera_matrices(renderer->backend, &view,
                                                   &projection);
}

HTHRenderer *hth_renderer_create(HTHPlatform *platform,
                                 const HTHCamera *camera,
                                 const HTHCollisionWorld *collision_world)
{
    HTHRenderer *renderer;

    renderer = calloc(1, sizeof(*renderer));
    if (renderer == NULL || platform == NULL || camera == NULL ||
        !hth_collision_world_is_valid(collision_world)) {
        fputs("Renderer initialization failed: allocation failed.\n", stderr);
        free(renderer);
        return NULL;
    }

    renderer->platform = platform;
    renderer->camera = *camera;
    renderer->backend = hth_renderer_opengl_create(platform, collision_world);
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
    uint32_t height;
    uint32_t width;

    if (renderer == NULL ||
        !hth_platform_framebuffer_size(renderer->platform, &width, &height)) {
        return false;
    }
    if (width == 0 || height == 0) {
        renderer->framebuffer_width = width;
        renderer->framebuffer_height = height;
        renderer->drawable = false;
        return true;
    }
    renderer->framebuffer_width = width;
    renderer->framebuffer_height = height;
    if (!hth_renderer_opengl_resize(renderer->backend, width, height) ||
        !update_camera_matrices(renderer)) {
        return false;
    }
    renderer->drawable = true;
    return true;
}

bool hth_renderer_set_camera(HTHRenderer *renderer,
                             const HTHCamera *camera)
{
    if (renderer == NULL || camera == NULL) {
        return false;
    }
    renderer->camera = *camera;
    return !renderer->drawable || update_camera_matrices(renderer);
}

bool hth_renderer_frame(HTHRenderer *renderer)
{
    return renderer != NULL &&
           (!renderer->drawable ||
            hth_renderer_opengl_frame(renderer->backend));
}
