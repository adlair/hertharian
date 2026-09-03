#ifndef HTH_ENEMY_RUNTIME_POPULATION_H
#define HTH_ENEMY_RUNTIME_POPULATION_H

#include "actor.h"
#include "dynamic_body.h"
#include "enemy.h"
#include "enemy_attack_cadence_store.h"
#include "enemy_target.h"
#include "entity.h"
#include "health.h"
#include "spatial.h"

#include <stdbool.h>

typedef struct {
    HTHSpatialTransform transform;
    HTHDynamicBody body;
    HTHHealth health;
} HTHEnemyRuntimeSpawnSpec;

bool hth_enemy_runtime_spawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHEnemyStore *enemies,
    HTHEnemyAttackCadenceStore *cadences,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    const HTHEnemyRuntimeSpawnSpec *spec,
    HTHEntityHandle *out_enemy);

bool hth_enemy_runtime_despawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHEnemyStore *enemies,
    HTHEnemyAttackCadenceStore *cadences,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHEnemyTargetStore *targets,
    HTHEntityHandle enemy);

#endif
