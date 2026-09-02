#ifndef HTH_PLAYER_TARGET_BRIDGE_H
#define HTH_PLAYER_TARGET_BRIDGE_H

#include "entity.h"
#include "player_body.h"
#include "spatial.h"

#include <stdbool.h>

typedef struct {
    HTHEntityHandle target_entity;
} HTHPlayerTargetBridge;

bool hth_player_target_bridge_create(
    HTHPlayerTargetBridge *bridge,
    HTHEntityRegistry *entities,
    HTHSpatialStore *spatial,
    const HTHPlayerBody *player);
bool hth_player_target_bridge_sync(
    HTHPlayerTargetBridge *bridge,
    const HTHEntityRegistry *entities,
    HTHSpatialStore *spatial,
    const HTHPlayerBody *player);
bool hth_player_target_bridge_get_target(
    const HTHPlayerTargetBridge *bridge,
    const HTHEntityRegistry *entities,
    const HTHSpatialStore *spatial,
    HTHEntityHandle *out_target);
bool hth_player_target_bridge_destroy(
    HTHPlayerTargetBridge *bridge,
    HTHEntityRegistry *entities,
    HTHSpatialStore *spatial);

#endif
