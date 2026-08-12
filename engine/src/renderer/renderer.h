#ifndef HTH_RENDERER_H
#define HTH_RENDERER_H

#include "platform.h"

#include <stdbool.h>

typedef struct HTHRenderer HTHRenderer;

HTHRenderer *hth_renderer_create(HTHPlatform *platform);
void hth_renderer_destroy(HTHRenderer *renderer);
bool hth_renderer_resize(HTHRenderer *renderer);
bool hth_renderer_frame(HTHRenderer *renderer);

#endif
