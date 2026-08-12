#include "renderer.h"
#include "renderer_opengl.h"

#include <stdio.h>
#include <stdlib.h>

struct HTHRenderer {
    HTHOpenGLBackend *backend;
};

HTHRenderer *hth_renderer_create(HTHPlatform *platform)
{
    HTHRenderer *renderer;

    renderer = calloc(1, sizeof(*renderer));
    if (renderer == NULL) {
        fputs("Renderer initialization failed: allocation failed.\n", stderr);
        return NULL;
    }

    renderer->backend = hth_renderer_opengl_create(platform);
    if (renderer->backend == NULL) {
        free(renderer);
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
    return renderer != NULL &&
           hth_renderer_opengl_resize(renderer->backend);
}

bool hth_renderer_frame(HTHRenderer *renderer)
{
    return renderer != NULL &&
           hth_renderer_opengl_frame(renderer->backend);
}
