#ifndef HTH_RUNTIME_BODY_VISUAL_H
#define HTH_RUNTIME_BODY_VISUAL_H

#include "dynamic_body.h"
#include "entity.h"
#include "renderer_types.h"
#include "spatial.h"

typedef enum {
    HTH_RUNTIME_BODY_VISUAL_READY = 0,
    HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE,
    HTH_RUNTIME_BODY_VISUAL_ERROR
} HTHRuntimeBodyVisualResult;

HTHRuntimeBodyVisualResult hth_runtime_body_visual_build(
    const HTHEntityRegistry *entities,
    const HTHSpatialStore *spatial,
    const HTHDynamicBodyStore *bodies,
    HTHEntityHandle entity,
    const float base_color[4],
    HTHRendererTransientDraw *out_draw);

#endif
