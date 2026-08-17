#ifndef HTH_ACTOR_SPAWN_H
#define HTH_ACTOR_SPAWN_H

#include "actor.h"
#include "dynamic_body.h"
#include "entity.h"
#include "health.h"
#include "spatial.h"

#include <stdbool.h>

typedef struct {
    bool has_spatial;
    HTHSpatialTransform transform;
    bool has_body;
    HTHDynamicBody body;
    bool has_health;
    HTHHealth health;
} HTHActorSpawnSpec;

bool hth_actor_spawn(HTHEntityRegistry *entities, HTHActorStore *actors,
                     HTHSpatialStore *spatial, HTHDynamicBodyStore *bodies,
                     HTHHealthStore *health, const HTHActorSpawnSpec *spec,
                     HTHEntityHandle *out_entity);

bool hth_actor_despawn(HTHEntityRegistry *entities, HTHActorStore *actors,
                       HTHSpatialStore *spatial,
                       HTHDynamicBodyStore *bodies, HTHHealthStore *health,
                       HTHEntityHandle entity);

#endif
