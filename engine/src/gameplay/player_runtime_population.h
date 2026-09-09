#ifndef HTH_PLAYER_RUNTIME_POPULATION_H
#define HTH_PLAYER_RUNTIME_POPULATION_H

#include "actor.h"
#include "dynamic_body.h"
#include "entity.h"
#include "health.h"
#include "player_lifecycle_runtime.h"
#include "spatial.h"

#include <stdbool.h>

typedef struct HTHPlayerRuntimeSpawnSpec {
    HTHSpatialTransform transform;
    HTHHealth health;
} HTHPlayerRuntimeSpawnSpec;

bool hth_player_runtime_spawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHPlayerLifecycleRuntime *lifecycle,
    const HTHPlayerRuntimeSpawnSpec *spec,
    HTHPlayerSlot *out_slot);

bool hth_player_runtime_despawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHPlayerLifecycleRuntime *lifecycle,
    HTHPlayerSlot slot);

#endif
