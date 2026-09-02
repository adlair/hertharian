#include "runtime_body_visual.h"

#include <math.h>
#include <stddef.h>

static HTHRendererTransientDraw empty_draw(void)
{
    const HTHRendererTransientDraw draw = {
        HTH_GEOMETRY_PRIMITIVE_BOX,
        {{0.0F}},
        {0.0F, 0.0F, 0.0F, 0.0F}
    };

    return draw;
}

static bool color_is_valid(const float color[4])
{
    size_t component;

    if (color == NULL) {
        return false;
    }
    for (component = 0U; component < 4U; ++component) {
        if (!isfinite(color[component]) || color[component] < 0.0F ||
            color[component] > 1.0F) {
            return false;
        }
    }
    return true;
}

HTHRuntimeBodyVisualResult hth_runtime_body_visual_build(
    const HTHEntityRegistry *entities,
    const HTHSpatialStore *spatial,
    const HTHDynamicBodyStore *bodies,
    HTHEntityHandle entity,
    const float base_color[4],
    HTHRendererTransientDraw *out_draw)
{
    HTHSpatialTransform transform;
    HTHDynamicBody body;
    HTHMat4 model;
    HTHMat4 rotation;
    HTHMat4 scale;
    float cosine;
    float sine;
    float scale_x;
    float scale_y;
    float scale_z;
    size_t component;

    if (out_draw != NULL) {
        *out_draw = empty_draw();
    }
    if (entities == NULL || spatial == NULL || bodies == NULL ||
        !color_is_valid(base_color) || out_draw == NULL) {
        return HTH_RUNTIME_BODY_VISUAL_ERROR;
    }
    if (!hth_entity_registry_is_alive(entities, entity) ||
        !hth_spatial_store_get(spatial, entities, entity, &transform) ||
        !hth_dynamic_body_get(bodies, entities, entity, &body)) {
        return HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE;
    }

    scale_x = body.half_extents.x * 2.0F;
    scale_y = body.half_extents.y * 2.0F;
    scale_z = body.half_extents.z * 2.0F;
    cosine = cosf(transform.yaw);
    sine = sinf(transform.yaw);
    if (!isfinite(scale_x) || !isfinite(scale_y) || !isfinite(scale_z) ||
        !isfinite(cosine) || !isfinite(sine)) {
        return HTH_RUNTIME_BODY_VISUAL_ERROR;
    }

    rotation = hth_mat4_identity();
    rotation.elements[0] = cosine;
    rotation.elements[2] = sine;
    rotation.elements[8] = -sine;
    rotation.elements[10] = cosine;
    scale = hth_mat4_identity();
    scale.elements[0] = scale_x;
    scale.elements[5] = scale_y;
    scale.elements[10] = scale_z;
    model = hth_mat4_multiply(
        hth_mat4_translation(transform.position),
        hth_mat4_multiply(rotation, scale));
    for (component = 0U; component < 16U; ++component) {
        if (!isfinite(model.elements[component])) {
            return HTH_RUNTIME_BODY_VISUAL_ERROR;
        }
    }

    out_draw->primitive = HTH_GEOMETRY_PRIMITIVE_BOX;
    out_draw->model = model;
    for (component = 0U; component < 4U; ++component) {
        out_draw->base_color[component] = base_color[component];
    }
    return HTH_RUNTIME_BODY_VISUAL_READY;
}
