#ifndef HTH_ENGINE_INTERNAL_H
#define HTH_ENGINE_INTERNAL_H

#include "hth_engine.h"
#include "actor.h"
#include "bootstrap_enemy_pursuit.h"
#include "dynamic_body.h"
#include "enemy.h"
#include "enemy_attack_cadence_store.h"
#include "enemy_target.h"
#include "entity.h"
#include "health.h"
#include "player_lifecycle_runtime.h"
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
    HTHEnemyAttackCadenceStore *enemy_attack_cadence_store;
    HTHHealthStore *health_store;
    HTHEnemyTargetStore *enemy_target_store;
    HTHBootstrapEnemyPursuit bootstrap_enemy_pursuit;
    HTHPlayerLifecycleRuntime player_lifecycle_runtime;
    HTHPlayerSlot local_player_slot;
};

bool hth_engine_init_with_level_id(HTHEngine *engine,
                                   const HTHEngineConfig *config,
                                   const char *level_id);

#endif
