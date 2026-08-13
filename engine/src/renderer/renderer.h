#ifndef HTH_RENDERER_H
#define HTH_RENDERER_H

#include "hth_camera.h"
#include "collision_world.h"
#include "platform.h"

#include <stdbool.h>

typedef struct HTHRenderer HTHRenderer;

HTHRenderer *hth_renderer_create(HTHPlatform *platform,
                                 const HTHCamera *camera,
                                 const HTHCollisionWorld *collision_world);
void hth_renderer_destroy(HTHRenderer *renderer);
bool hth_renderer_resize(HTHRenderer *renderer);
bool hth_renderer_set_camera(HTHRenderer *renderer,
                             const HTHCamera *camera);
bool hth_renderer_frame(HTHRenderer *renderer);

#endif
