#include "renderer.h"
#include "renderer_opengl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
                                 const HTHWorld *world)
{
    HTHOpenGLStaticDraw *draws;
    HTHRenderer *renderer;
    size_t draw_count = 0;
    size_t index;

    renderer = calloc(1, sizeof(*renderer));
    if (renderer == NULL || platform == NULL || camera == NULL ||
        !hth_world_is_finalized(world)) {
        fputs("Renderer initialization failed: allocation failed.\n", stderr);
        free(renderer);
        return NULL;
    }

    draws = calloc(hth_world_static_object_count(world), sizeof(*draws));
    if (draws == NULL && hth_world_static_object_count(world) > 0U) {
        fputs("Renderer initialization failed: draw allocation failed.\n",
              stderr);
        free(renderer);
        return NULL;
    }
    for (index = 0; index < hth_world_static_object_count(world); ++index) {
        const HTHWorldStaticObject *object =
            hth_world_static_object(world, index);
        const float *color;
        static const float floor_color[4] = {0.22F, 0.24F, 0.28F, 1.0F};
        static const float wall_color[4] = {0.30F, 0.45F, 0.62F, 1.0F};
        static const float box_color[4] = {0.90F, 0.20F, 0.48F, 1.0F};
        static const float low_step_color[4] = {0.18F, 0.78F, 0.32F, 1.0F};
        static const float limit_step_color[4] = {1.00F, 0.72F, 0.10F, 1.0F};
        static const float high_ledge_color[4] = {0.95F, 0.30F, 0.12F, 1.0F};
        static const float platform_color[4] = {0.16F, 0.68F, 0.78F, 1.0F};
        static const float corner_color[4] = {0.55F, 0.28F, 0.75F, 1.0F};
        static const float corridor_corner_color[4] = {
            0.48F, 0.30F, 0.72F, 1.0F
        };

        if (object == NULL ||
            (object->flags & HTH_WORLD_OBJECT_VISIBLE) == 0U) {
            continue;
        }
        switch (object->visual_class) {
        case HTH_WORLD_VISUAL_FLOOR: color = floor_color; break;
        case HTH_WORLD_VISUAL_WALL: color = wall_color; break;
        case HTH_WORLD_VISUAL_BOX: color = box_color; break;
        case HTH_WORLD_VISUAL_LOW_STEP: color = low_step_color; break;
        case HTH_WORLD_VISUAL_LIMIT_STEP: color = limit_step_color; break;
        case HTH_WORLD_VISUAL_HIGH_LEDGE: color = high_ledge_color; break;
        case HTH_WORLD_VISUAL_PLATFORM: color = platform_color; break;
        case HTH_WORLD_VISUAL_CORNER: color = corner_color; break;
        case HTH_WORLD_VISUAL_CORRIDOR_CORNER:
            color = corridor_corner_color;
            break;
        default:
            fputs("Renderer initialization failed: visible object has no "
                  "diagnostic visual class.\n", stderr);
            free(draws);
            free(renderer);
            return NULL;
        }
        draws[draw_count].bounds = object->bounds;
        memcpy(draws[draw_count].color, color,
               sizeof(draws[draw_count].color));
        draw_count++;
    }

    renderer->platform = platform;
    renderer->camera = *camera;
    renderer->backend = hth_renderer_opengl_create(
        platform, draws, draw_count);
    free(draws);
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
