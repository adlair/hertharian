#include "bootstrap_enemy_pursuit.h"

#include "collision_trace.h"
#include "enemy_pursuit_runtime.h"
#include "enemy_runtime_population.h"

#include <math.h>
#include <stddef.h>

static const HTHSpatialTransform bootstrap_enemy_transform = {
    {3.0F, 0.95F, 3.0F},
    0.0F
};
static const HTHDynamicBody bootstrap_enemy_body = {
    {0.30F, 0.90F, 0.30F},
    {0.0F, 0.0F, 0.0F}
};
static const HTHHealth bootstrap_enemy_health = {100.0F, 100.0F};
static const HTHHealth bootstrap_player_health = {100.0F, 100.0F};
static const float bootstrap_enemy_perception_radius = 8.0F;
static const float bootstrap_enemy_attack_range = 1.25F;
static const float bootstrap_enemy_chase_speed = 2.0F;
static const float bootstrap_enemy_attack_damage = 10.0F;
static const double bootstrap_enemy_attack_interval_seconds = 1.0;
static const double bootstrap_simulation_max_delta_seconds = 0.1;

static bool handle_is_invalid(HTHEntityHandle handle)
{
    return hth_entity_handle_equal(handle, hth_entity_handle_invalid());
}

static bool integration_is_inactive(
    const HTHBootstrapEnemyPursuit *integration)
{
    return integration != NULL &&
           handle_is_invalid(integration->player_target_bridge.target_entity) &&
           handle_is_invalid(integration->enemy);
}

static bool placement_is_clear(const HTHCollisionWorld *collision_world)
{
    HTHTrace trace;
    const HTHVec3 mins = {
        -bootstrap_enemy_body.half_extents.x,
        -bootstrap_enemy_body.half_extents.y,
        -bootstrap_enemy_body.half_extents.z
    };
    const HTHVec3 maxs = bootstrap_enemy_body.half_extents;

    return hth_collision_world_trace_aabb(
               collision_world, bootstrap_enemy_transform.position,
               bootstrap_enemy_transform.position, mins, maxs, &trace) &&
           !trace.start_solid;
}

void hth_bootstrap_enemy_pursuit_initialize(
    HTHBootstrapEnemyPursuit *integration)
{
    if (integration == NULL) {
        return;
    }
    integration->player_target_bridge.target_entity =
        hth_entity_handle_invalid();
    integration->enemy = hth_entity_handle_invalid();
}

bool hth_bootstrap_enemy_pursuit_simulation_delta(
    double raw_delta_seconds, double *out_delta_seconds)
{
    if (out_delta_seconds == NULL || !isfinite(raw_delta_seconds) ||
        raw_delta_seconds < 0.0) {
        return false;
    }
    *out_delta_seconds = raw_delta_seconds >
            bootstrap_simulation_max_delta_seconds
        ? bootstrap_simulation_max_delta_seconds
        : raw_delta_seconds;
    return true;
}

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
    const HTHPlayerBody *player)
{
    const HTHEnemyRuntimeSpawnSpec spec = {
        bootstrap_enemy_transform,
        bootstrap_enemy_body,
        bootstrap_enemy_health
    };

    if (!integration_is_inactive(integration) || entities == NULL ||
        actors == NULL || enemies == NULL || cadences == NULL ||
        spatial == NULL ||
        bodies == NULL || health == NULL ||
        !hth_collision_world_is_valid(collision_world) ||
        !hth_player_body_is_valid(player)) {
        return HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_INVALID;
    }
    if (!placement_is_clear(collision_world)) {
        return HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_START_SOLID;
    }
    if (!hth_player_target_bridge_create(
            &integration->player_target_bridge, entities, actors, spatial,
            bodies, health, player, bootstrap_player_health)) {
        return HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_BRIDGE_FAILED;
    }
    if (!hth_enemy_runtime_spawn(
            entities, actors, enemies, cadences, spatial, bodies, health,
            &spec,
            &integration->enemy)) {
        (void)hth_player_target_bridge_destroy(
            &integration->player_target_bridge, entities, actors, spatial,
            bodies, health);
        return HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_ENEMY_FAILED;
    }
    return HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK;
}

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
    bool player_dead,
    double delta_seconds)
{
    HTHEntityHandle player_target;
    const HTHEntityHandle *excluded_targets = NULL;
    size_t excluded_target_count = 0U;
    HTHEntityHandle dead_player[1];
    HTHEntityHandle candidates[1];

    if (integration == NULL || entities == NULL ||
        !hth_entity_registry_is_alive(entities, integration->enemy)) {
        return HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_PURSUIT_FAILED;
    }
    if (!hth_player_target_bridge_sync(
            &integration->player_target_bridge, entities, spatial, player)) {
        return HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_BRIDGE_SYNC_FAILED;
    }
    if (!hth_player_target_bridge_get_target(
            &integration->player_target_bridge, entities, spatial,
            &player_target)) {
        return HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_GET_TARGET_FAILED;
    }
    candidates[0] = player_target;
    if (player_dead) {
        dead_player[0] = player_target;
        excluded_targets = dead_player;
        excluded_target_count = 1U;
    }
    if (!hth_enemy_pursuit_runtime_step(
            entities, actors, enemies, spatial, bodies, health, targets,
            cadences,
            collision_world, candidates, 1U, excluded_targets,
            excluded_target_count,
            bootstrap_enemy_perception_radius, bootstrap_enemy_attack_range,
            bootstrap_enemy_chase_speed, bootstrap_enemy_attack_damage,
            bootstrap_enemy_attack_interval_seconds, delta_seconds)) {
        return HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_PURSUIT_FAILED;
    }
    return HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK;
}

HTHBootstrapEnemyPursuitCleanupResult hth_bootstrap_enemy_pursuit_cleanup(
    HTHBootstrapEnemyPursuit *integration,
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHEnemyStore *enemies,
    HTHEnemyAttackCadenceStore *cadences,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHEnemyTargetStore *targets)
{
    HTHBootstrapEnemyPursuitCleanupResult result = {false, false};

    if (integration == NULL) {
        return result;
    }
    if (!handle_is_invalid(integration->enemy)) {
        if (hth_enemy_runtime_despawn(
                entities, actors, enemies, cadences, spatial, bodies, health,
                targets, integration->enemy)) {
            integration->enemy = hth_entity_handle_invalid();
        } else {
            result.enemy_despawn_failed = true;
        }
    }
    if (!handle_is_invalid(
            integration->player_target_bridge.target_entity)) {
        if (hth_player_target_bridge_destroy(
                &integration->player_target_bridge, entities, actors,
                spatial, bodies, health)) {
            integration->player_target_bridge.target_entity =
                hth_entity_handle_invalid();
        } else {
            result.bridge_destroy_failed = true;
        }
    }
    return result;
}
