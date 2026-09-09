#ifndef HTH_PLAYER_REVIVE_TARGET_SELECTION_H
#define HTH_PLAYER_REVIVE_TARGET_SELECTION_H

#include "player_lifecycle_runtime.h"
#include "spatial.h"

#include <stdbool.h>

bool hth_player_revive_target_select(
    const HTHPlayerLifecycleRuntime *lifecycle,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    const HTHSpatialStore *spatial,
    HTHPlayerSlot reviver_slot,
    float revive_range,
    HTHPlayerSlot *out_target_slot);

#endif
