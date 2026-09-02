#include "player_target_bridge.h"

#include <float.h>
#include <math.h>

static bool bridge_is_inactive(const HTHPlayerTargetBridge *bridge)
{
    return bridge != NULL && hth_entity_handle_equal(
        bridge->target_entity, hth_entity_handle_invalid());
}

static bool bridge_is_current(const HTHPlayerTargetBridge *bridge,
                              const HTHEntityRegistry *entities,
                              const HTHSpatialStore *spatial)
{
    return bridge != NULL && entities != NULL && spatial != NULL &&
           hth_entity_registry_is_alive(entities, bridge->target_entity) &&
           hth_spatial_store_has(spatial, entities, bridge->target_entity);
}

static bool player_anchor_transform(const HTHPlayerBody *player,
                                    HTHSpatialTransform *out_transform)
{
    double anchor_x;
    double anchor_y;
    double anchor_z;

    if (out_transform == NULL || !hth_player_body_is_valid(player)) {
        return false;
    }

    anchor_x = (double)player->position.x;
    anchor_y = (double)player->position.y +
               (double)player->height * 0.5;
    anchor_z = (double)player->position.z;
    if (!isfinite(anchor_x) || !isfinite(anchor_y) || !isfinite(anchor_z) ||
        anchor_x < -(double)FLT_MAX || anchor_x > (double)FLT_MAX ||
        anchor_y < -(double)FLT_MAX || anchor_y > (double)FLT_MAX ||
        anchor_z < -(double)FLT_MAX || anchor_z > (double)FLT_MAX) {
        return false;
    }

    out_transform->position.x = (float)anchor_x;
    out_transform->position.y = (float)anchor_y;
    out_transform->position.z = (float)anchor_z;
    out_transform->yaw = 0.0F;
    return true;
}

bool hth_player_target_bridge_create(
    HTHPlayerTargetBridge *bridge,
    HTHEntityRegistry *entities,
    HTHSpatialStore *spatial,
    const HTHPlayerBody *player)
{
    HTHSpatialTransform transform;
    HTHEntityHandle entity;

    if (!bridge_is_inactive(bridge)) {
        return false;
    }
    bridge->target_entity = hth_entity_handle_invalid();
    if (entities == NULL || spatial == NULL ||
        !player_anchor_transform(player, &transform) ||
        !hth_entity_registry_create_entity(entities, &entity)) {
        return false;
    }
    if (!hth_spatial_store_attach(spatial, entities, entity, &transform)) {
        (void)hth_entity_registry_destroy_entity(entities, entity);
        return false;
    }
    bridge->target_entity = entity;
    return true;
}

bool hth_player_target_bridge_sync(
    HTHPlayerTargetBridge *bridge,
    const HTHEntityRegistry *entities,
    HTHSpatialStore *spatial,
    const HTHPlayerBody *player)
{
    HTHSpatialTransform transform;

    if (!bridge_is_current(bridge, entities, spatial) ||
        !player_anchor_transform(player, &transform)) {
        return false;
    }
    return hth_spatial_store_set(spatial, entities, bridge->target_entity,
                                 &transform);
}

bool hth_player_target_bridge_get_target(
    const HTHPlayerTargetBridge *bridge,
    const HTHEntityRegistry *entities,
    const HTHSpatialStore *spatial,
    HTHEntityHandle *out_target)
{
    if (out_target == NULL) {
        return false;
    }
    *out_target = hth_entity_handle_invalid();
    if (!bridge_is_current(bridge, entities, spatial)) {
        return false;
    }
    *out_target = bridge->target_entity;
    return true;
}

bool hth_player_target_bridge_destroy(
    HTHPlayerTargetBridge *bridge,
    HTHEntityRegistry *entities,
    HTHSpatialStore *spatial)
{
    HTHEntityHandle entity;
    HTHSpatialTransform transform;

    if (!bridge_is_current(bridge, entities, spatial) ||
        !hth_spatial_store_get(spatial, entities, bridge->target_entity,
                               &transform)) {
        return false;
    }
    entity = bridge->target_entity;
    if (!hth_spatial_store_remove(spatial, entities, entity)) {
        return false;
    }
    if (!hth_entity_registry_destroy_entity(entities, entity)) {
        (void)hth_spatial_store_attach(spatial, entities, entity,
                                       &transform);
        return false;
    }
    bridge->target_entity = hth_entity_handle_invalid();
    return true;
}
