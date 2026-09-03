#ifndef HTH_BOOTSTRAP_ENEMY_PURSUIT_H
#define HTH_BOOTSTRAP_ENEMY_PURSUIT_H

#include "actor.h"
#include "collision_world.h"
#include "dynamic_body.h"
#include "enemy.h"
#include "enemy_attack_cadence_store.h"
#include "enemy_target.h"
#include "entity.h"
#include "health.h"
#include "player_body.h"
#include "player_target_bridge.h"
#include "spatial.h"

#include <stdbool.h>

typedef struct {
    HTHPlayerTargetBridge player_target_bridge;
    HTHEntityHandle enemy;
} HTHBootstrapEnemyPursuit;

typedef enum {
    HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK = 0,
    HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_INVALID,
    HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_START_SOLID,
    HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_BRIDGE_FAILED,
    HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_ENEMY_FAILED
} HTHBootstrapEnemyPursuitCreateResult;

typedef enum {
    HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK = 0,
    HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_BRIDGE_SYNC_FAILED,
    HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_GET_TARGET_FAILED,
    HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_PURSUIT_FAILED
} HTHBootstrapEnemyPursuitStepResult;

typedef struct {
    bool enemy_despawn_failed;
    bool bridge_destroy_failed;
} HTHBootstrapEnemyPursuitCleanupResult;

void hth_bootstrap_enemy_pursuit_initialize(
    HTHBootstrapEnemyPursuit *integration);
bool hth_bootstrap_enemy_pursuit_simulation_delta(
    double raw_delta_seconds, double *out_delta_seconds);
HTHBootstrapEnemyPursuitCreateResult hth_bootstrap_enemy_pursuit_create(
    HTHBootstrapEnemyPursuit *integration,
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHEnemyStore *enemies,
    HTHEnemyAttackCadenceStore *cadences,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    const HTHCollisionWorld *collision_world,
    const HTHPlayerBody *player);
HTHBootstrapEnemyPursuitStepResult hth_bootstrap_enemy_pursuit_step(
    HTHBootstrapEnemyPursuit *integration,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHEnemyTargetStore *targets,
    HTHEnemyAttackCadenceStore *cadences,
    const HTHCollisionWorld *collision_world,
    const HTHPlayerBody *player,
    double delta_seconds);
HTHBootstrapEnemyPursuitCleanupResult hth_bootstrap_enemy_pursuit_cleanup(
    HTHBootstrapEnemyPursuit *integration,
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHEnemyStore *enemies,
    HTHEnemyAttackCadenceStore *cadences,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHEnemyTargetStore *targets);

#endif
