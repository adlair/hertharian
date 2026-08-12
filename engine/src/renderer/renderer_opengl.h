#ifndef HTH_RENDERER_OPENGL_H
#define HTH_RENDERER_OPENGL_H

#include "platform.h"

#include <stdbool.h>

typedef struct HTHOpenGLBackend HTHOpenGLBackend;

HTHOpenGLBackend *hth_renderer_opengl_create(HTHPlatform *platform);
void hth_renderer_opengl_destroy(HTHOpenGLBackend *backend);
bool hth_renderer_opengl_resize(HTHOpenGLBackend *backend);
bool hth_renderer_opengl_frame(HTHOpenGLBackend *backend);

#endif
