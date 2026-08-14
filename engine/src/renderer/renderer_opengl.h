#ifndef HTH_RENDERER_OPENGL_H
#define HTH_RENDERER_OPENGL_H

#include "hth_math.h"
#include "aabb.h"
#include "platform.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct HTHOpenGLBackend HTHOpenGLBackend;

typedef struct {
    HTHAABB bounds;
    float color[4];
} HTHOpenGLStaticDraw;

HTHOpenGLBackend *hth_renderer_opengl_create(
    HTHPlatform *platform, const HTHOpenGLStaticDraw *draws,
    size_t draw_count);
void hth_renderer_opengl_destroy(HTHOpenGLBackend *backend);
bool hth_renderer_opengl_resize(HTHOpenGLBackend *backend,
                                uint32_t width, uint32_t height);
bool hth_renderer_opengl_set_camera_matrices(HTHOpenGLBackend *backend,
                                             const HTHMat4 *view,
                                             const HTHMat4 *projection);
bool hth_renderer_opengl_frame(HTHOpenGLBackend *backend);

#endif
