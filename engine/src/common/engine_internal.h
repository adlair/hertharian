#ifndef HTH_ENGINE_INTERNAL_H
#define HTH_ENGINE_INTERNAL_H

#include "hth_engine.h"
#include "actor.h"
#include "dynamic_body.h"
#include "enemy.h"
#include "enemy_target.h"
#include "entity.h"
#include "health.h"
#include "spatial.h"
#include "world.h"

#include <stdbool.h>

struct HTHEngineWorldState {
    HTHWorld world;
    HTHEntityRegistry *entity_registry;
    HTHSpatialStore *spatial_store;
    HTHDynamicBodyStore *dynamic_body_store;
    HTHActorStore *actor_store;
    HTHEnemyStore *enemy_store;
    HTHHealthStore *health_store;
    HTHEnemyTargetStore *enemy_target_store;
};

bool hth_engine_init_with_level_id(HTHEngine *engine,
                                   const HTHEngineConfig *config,
                                   const char *level_id);

#endif
