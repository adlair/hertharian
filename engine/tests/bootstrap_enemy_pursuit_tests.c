#include "bootstrap_enemy_pursuit.h"
#include "player_movement.h"
#include "runtime_body_visual.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,      \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

typedef struct {
    HTHEntityRegistry *entities;
    HTHActorStore *actors;
    HTHEnemyStore *enemies;
    HTHEnemyAttackCadenceStore *cadences;
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
    HTHEnemyTargetStore *targets;
    HTHCollisionWorld world;
} Fixture;

static bool handle_is_invalid(HTHEntityHandle handle)
{
    return hth_entity_handle_equal(handle, hth_entity_handle_invalid());
}

static HTHPlayerBody player_at(float x, float z)
{
    HTHPlayerBody player;

    if (!hth_player_body_init(&player, (HTHVec3){x, 0.05F, z})) {
        abort();
    }
    return player;
}

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->enemies = hth_enemy_store_create();
    fixture->cadences = hth_enemy_attack_cadence_store_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
    fixture->targets = hth_enemy_target_store_create();
    fixture->world.obstacles[0] =
        (HTHAABB){{100.0F, -10.0F, -10.0F},
                  {101.0F, 10.0F, 10.0F}};
    fixture->world.obstacle_count = 1U;
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->cadences != NULL &&
           fixture->spatial != NULL &&
           fixture->bodies != NULL && fixture->health != NULL &&
           fixture->targets != NULL &&
           hth_collision_world_is_valid(&fixture->world);
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_target_store_destroy(fixture->targets);
    hth_enemy_attack_cadence_store_destroy(fixture->cadences);
    hth_health_store_destroy(fixture->health);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_entity_registry_destroy(fixture->entities);
}

static HTHBootstrapEnemyPursuitCreateResult create_integration(
    Fixture *fixture, HTHBootstrapEnemyPursuit *integration,
    const HTHPlayerBody *player)
{
    return hth_bootstrap_enemy_pursuit_create(
        integration, fixture->entities, fixture->actors, fixture->enemies,
        fixture->cadences,
        fixture->spatial, fixture->bodies, fixture->health, &fixture->world,
        player);
}

static HTHBootstrapEnemyPursuitStepResult step_integration(
    Fixture *fixture, HTHBootstrapEnemyPursuit *integration,
    const HTHPlayerBody *player, float delta_seconds)
{
    return hth_bootstrap_enemy_pursuit_step(
        integration, fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, fixture->bodies, fixture->health, fixture->targets,
        fixture->cadences, &fixture->world, player, (double)delta_seconds);
}

static HTHBootstrapEnemyPursuitCleanupResult cleanup_integration(
    Fixture *fixture, HTHBootstrapEnemyPursuit *integration)
{
    return hth_bootstrap_enemy_pursuit_cleanup(
        integration, fixture->entities, fixture->actors, fixture->enemies,
        fixture->cadences,
        fixture->spatial, fixture->bodies, fixture->health, fixture->targets);
}

static bool cleanup_succeeded(HTHBootstrapEnemyPursuitCleanupResult result)
{
    return !result.enemy_despawn_failed && !result.bridge_destroy_failed;
}

static bool test_canonical_state_and_delta(void)
{
    HTHBootstrapEnemyPursuit integration = {{{1U, 2U}}, {3U, 4U}};
    double delta = -1.0;

    hth_bootstrap_enemy_pursuit_initialize(&integration);
    CHECK(handle_is_invalid(integration.player_target_bridge.target_entity));
    CHECK(handle_is_invalid(integration.enemy));
    hth_bootstrap_enemy_pursuit_initialize(NULL);
    CHECK(hth_bootstrap_enemy_pursuit_simulation_delta(0.0, &delta));
    CHECK(delta == 0.0);
    CHECK(hth_bootstrap_enemy_pursuit_simulation_delta(0.05, &delta));
    CHECK(delta == 0.05);
    CHECK(hth_bootstrap_enemy_pursuit_simulation_delta(10.0, &delta));
    CHECK(delta == 0.1);
    CHECK(!hth_bootstrap_enemy_pursuit_simulation_delta(-1.0, &delta));
    CHECK(!hth_bootstrap_enemy_pursuit_simulation_delta(NAN, &delta));
    CHECK(!hth_bootstrap_enemy_pursuit_simulation_delta(0.0, NULL));
    return true;
}

static bool test_create_composition_and_cleanup(void)
{
    Fixture fixture;
    HTHBootstrapEnemyPursuit integration;
    HTHPlayerBody player = player_at(0.0F, 3.0F);
    HTHSpatialTransform enemy_transform;
    HTHSpatialTransform proxy_transform;
    HTHDynamicBody body;
    HTHHealth health;
    HTHEntityHandle target;

    CHECK(fixture_create(&fixture));
    hth_bootstrap_enemy_pursuit_initialize(&integration);
    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK);
    CHECK(hth_entity_registry_live_count(fixture.entities) == 2U);
    CHECK(hth_player_target_bridge_get_target(
        &integration.player_target_bridge, fixture.entities, fixture.spatial,
        &target));
    CHECK(!hth_entity_handle_equal(target, integration.enemy));
    CHECK(hth_entity_registry_is_alive(fixture.entities, target));
    CHECK(hth_spatial_store_has(fixture.spatial, fixture.entities, target));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities,
                               fixture.actors, target));
    CHECK(!hth_dynamic_body_has(fixture.bodies, fixture.entities, target));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &health));
    CHECK(health.current == 100.0F && health.maximum == 100.0F);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &proxy_transform));
    CHECK(proxy_transform.position.x == 0.0F &&
          proxy_transform.position.y == 0.95F &&
          proxy_transform.position.z == 3.0F);
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                              integration.enemy));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, integration.enemy));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &enemy_transform));
    CHECK(enemy_transform.position.x == 3.0F &&
          enemy_transform.position.y == 0.95F &&
          enemy_transform.position.z == 3.0F &&
          enemy_transform.yaw == 0.0F);
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities,
                               integration.enemy, &body));
    CHECK(body.half_extents.x == 0.30F &&
          body.half_extents.y == 0.90F &&
          body.half_extents.z == 0.30F && body.velocity.x == 0.0F &&
          body.velocity.y == 0.0F && body.velocity.z == 0.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, integration.enemy, &health));
    CHECK(health.current == 100.0F && health.maximum == 100.0F);
    CHECK(hth_health_store_get(
        fixture.health, fixture.entities, fixture.actors,
        integration.player_target_bridge.target_entity, &health));
    CHECK(health.current == 100.0F && health.maximum == 100.0F);
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &target));
    CHECK(handle_is_invalid(target));
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    CHECK(handle_is_invalid(integration.player_target_bridge.target_entity));
    CHECK(handle_is_invalid(integration.enemy));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_create_failures_and_partial_cleanup(void)
{
    Fixture fixture;
    HTHBootstrapEnemyPursuit integration;
    HTHPlayerBody player = player_at(0.0F, 3.0F);
    HTHPlayerBody unrepresentable = player;

    CHECK(fixture_create(&fixture));
    hth_bootstrap_enemy_pursuit_initialize(&integration);
    fixture.world.obstacles[0] =
        (HTHAABB){{2.0F, 0.0F, 2.0F}, {4.0F, 2.0F, 4.0F}};
    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_START_SOLID);
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));

    fixture.world.obstacles[0] =
        (HTHAABB){{100.0F, -10.0F, -10.0F},
                  {101.0F, 10.0F, 10.0F}};
    unrepresentable.position.y = FLT_MAX;
    unrepresentable.height = FLT_MAX;
    unrepresentable.eye_height = 1.0F;
    CHECK(create_integration(&fixture, &integration, &unrepresentable) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_BRIDGE_FAILED);
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);

    CHECK(hth_player_target_bridge_create(
        &integration.player_target_bridge, fixture.entities, fixture.actors,
        fixture.spatial, fixture.bodies, fixture.health, &player,
        (HTHHealth){100.0F, 100.0F}));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 1U);
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    fixture_destroy(&fixture);
    return true;
}

static bool test_acquisition_current_sync_and_stable_target(void)
{
    Fixture fixture;
    HTHBootstrapEnemyPursuit integration;
    HTHPlayerBody player = player_at(0.0F, 3.0F);
    HTHSpatialTransform transform;
    HTHEntityHandle target;
    HTHEntityHandle repeated;

    CHECK(fixture_create(&fixture));
    hth_bootstrap_enemy_pursuit_initialize(&integration);
    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK);
    CHECK(step_integration(&fixture, &integration, &player, 0.0F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &target));
    CHECK(hth_entity_handle_equal(
        target, integration.player_target_bridge.target_entity));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &transform));
    CHECK(transform.position.x == 3.0F && transform.position.z == 3.0F);

    player.position = (HTHVec3){3.0F, 0.05F, 6.0F};
    CHECK(step_integration(&fixture, &integration, &player, 0.1F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &transform));
    CHECK(transform.position.x == 3.0F &&
          fabsf(transform.position.z - 3.2F) < 1.0e-6F);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &repeated));
    CHECK(hth_entity_handle_equal(repeated, target));
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    fixture_destroy(&fixture);
    return true;
}

static bool test_player_movement_precedes_proxy_sync(void)
{
    Fixture fixture;
    HTHBootstrapEnemyPursuit integration;
    HTHPlayerBody player = player_at(0.0F, 3.0F);
    HTHMovementConfig config = hth_movement_config_default();
    const HTHPlayerMovementIntent intent = {
        {1.0F, 0.0F, 0.0F},
        1.0F,
        false
    };
    HTHPlayerMovementResult movement_result;
    HTHSpatialTransform enemy_transform;
    HTHSpatialTransform proxy_transform;
    HTHEntityHandle target;
    double simulation_delta;

    CHECK(fixture_create(&fixture));
    hth_bootstrap_enemy_pursuit_initialize(&integration);
    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK);
    CHECK(hth_bootstrap_enemy_pursuit_simulation_delta(
        10.0, &simulation_delta));
    CHECK(simulation_delta == 0.1);
    player.grounded = true;
    CHECK(hth_player_movement_step_with_result(
        &player, &fixture.world, &config, &intent, simulation_delta,
        &movement_result));
    CHECK(player.position.x > 0.0F);
    CHECK(step_integration(&fixture, &integration, &player,
                           (float)simulation_delta) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_player_target_bridge_get_target(
        &integration.player_target_bridge, fixture.entities, fixture.spatial,
        &target));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &proxy_transform));
    CHECK(proxy_transform.position.x == player.position.x);
    CHECK(proxy_transform.position.y ==
          player.position.y + player.height * 0.5F);
    CHECK(proxy_transform.position.z == player.position.z);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &target));
    CHECK(hth_entity_handle_equal(
        target, integration.player_target_bridge.target_entity));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &enemy_transform));
    CHECK(fabsf(enemy_transform.position.x - 2.8F) < 1.0e-6F);
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    fixture_destroy(&fixture);
    return true;
}

static bool test_noop_policies_and_static_collision(void)
{
    Fixture fixture;
    HTHBootstrapEnemyPursuit integration;
    HTHPlayerBody player = player_at(20.0F, 3.0F);
    HTHSpatialTransform transform;
    HTHDynamicBody body;
    HTHDynamicBody body_before;
    HTHEntityHandle target;

    CHECK(fixture_create(&fixture));
    hth_bootstrap_enemy_pursuit_initialize(&integration);
    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK);
    CHECK(step_integration(&fixture, &integration, &player, 0.1F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &target));
    player = player_at(0.0F, 3.0F);
    fixture.world.obstacles[0] =
        (HTHAABB){{1.4F, 0.0F, 2.5F}, {1.6F, 2.0F, 3.5F}};
    CHECK(step_integration(&fixture, &integration, &player, 0.1F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &target));
    fixture.world.obstacles[0] =
        (HTHAABB){{2.35F, 0.0F, 2.5F}, {2.60F, 0.50F, 3.5F}};
    CHECK(step_integration(&fixture, &integration, &player, 0.1F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &target));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &transform));
    CHECK(transform.position.x >= 2.89F && transform.position.x <= 3.0F);
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities,
                               integration.enemy, &body_before));

    player = player_at(transform.position.x, transform.position.z);
    CHECK(step_integration(&fixture, &integration, &player, 0.1F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities,
                               integration.enemy, &body));
    CHECK(body.velocity.x == body_before.velocity.x &&
          body.velocity.y == body_before.velocity.y &&
          body.velocity.z == body_before.velocity.z);
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    fixture_destroy(&fixture);
    return true;
}

static bool test_attack_range_transitions(void)
{
    Fixture fixture;
    HTHBootstrapEnemyPursuit integration;
    HTHPlayerBody player = player_at(1.75F, 3.0F);
    HTHVec3 authoritative_player_position;
    HTHSpatialTransform before_attack;
    HTHSpatialTransform after_attack;
    HTHSpatialTransform after_resume;
    HTHDynamicBody body_before_attack;
    HTHDynamicBody body_after_attack;
    HTHHealth health;
    HTHEntityHandle target;

    CHECK(fixture_create(&fixture));
    hth_bootstrap_enemy_pursuit_initialize(&integration);
    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK);

    CHECK(step_integration(&fixture, &integration, &player, 0.5F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &before_attack));
    CHECK(fabsf(before_attack.position.x - 3.0F) < 1.0e-6F);

    player = player_at(0.0F, 3.0F);
    authoritative_player_position = player.position;
    CHECK(step_integration(&fixture, &integration, &player, 0.5F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(step_integration(&fixture, &integration, &player, 0.5F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &before_attack));
    CHECK(fabsf(before_attack.position.x - 1.0F) < 1.0e-6F);
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities,
                               integration.enemy, &body_before_attack));
    CHECK(fabsf(body_before_attack.velocity.x + 2.0F) < 1.0e-6F);
    CHECK(step_integration(&fixture, &integration, &player, 0.5F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &after_attack));
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities,
                               integration.enemy, &body_after_attack));
    CHECK(memcmp(&before_attack, &after_attack, sizeof(before_attack)) == 0);
    CHECK(memcmp(&body_before_attack, &body_after_attack,
                 sizeof(body_before_attack)) == 0);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &target));
    CHECK(hth_entity_handle_equal(
        target, integration.player_target_bridge.target_entity));
    CHECK(player.position.x == authoritative_player_position.x &&
          player.position.y == authoritative_player_position.y &&
          player.position.z == authoritative_player_position.z);
    CHECK(hth_health_store_get(
        fixture.health, fixture.entities, fixture.actors,
        integration.player_target_bridge.target_entity, &health));
    CHECK(health.current == 80.0F && health.maximum == 100.0F);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, integration.enemy, &health));
    CHECK(health.current == 100.0F && health.maximum == 100.0F);

    player = player_at(4.0F, 3.0F);
    CHECK(step_integration(&fixture, &integration, &player, 0.5F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &after_resume));
    CHECK(fabsf(after_resume.position.x - 2.0F) < 1.0e-6F);
    CHECK(after_resume.position.x > after_attack.position.x);

    player = player_at(2.0F, 3.0F);
    CHECK(step_integration(&fixture, &integration, &player, 0.5F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &after_attack));
    CHECK(memcmp(&after_resume, &after_attack, sizeof(after_resume)) == 0);

    player = player_at(0.0F, 3.0F);
    fixture.world.obstacles[0] =
        (HTHAABB){{0.9F, -1.0F, 2.5F}, {1.1F, 2.0F, 3.5F}};
    CHECK(step_integration(&fixture, &integration, &player, 0.5F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                integration.enemy, &after_attack));
    CHECK(memcmp(&after_resume, &after_attack, sizeof(after_resume)) == 0);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &target));
    CHECK(hth_entity_handle_equal(
        target, integration.player_target_bridge.target_entity));

    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    fixture_destroy(&fixture);
    return true;
}

static bool test_failures_cleanup_order_and_restart(void)
{
    Fixture fixture;
    HTHBootstrapEnemyPursuit integration;
    HTHPlayerBody player = player_at(0.0F, 3.0F);
    HTHBootstrapEnemyPursuitCleanupResult cleanup;
    HTHEntityHandle first_proxy;
    HTHEntityHandle first_enemy;
    HTHEntityHandle target;

    CHECK(fixture_create(&fixture));
    hth_bootstrap_enemy_pursuit_initialize(&integration);
    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK);
    first_proxy = integration.player_target_bridge.target_entity;
    first_enemy = integration.enemy;
    CHECK(step_integration(&fixture, &integration, &player, 0.0F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        integration.enemy, &target));
    CHECK(hth_entity_handle_equal(target, first_proxy));
    cleanup = cleanup_integration(&fixture, &integration);
    CHECK(cleanup_succeeded(cleanup));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    CHECK(!hth_entity_registry_is_alive(fixture.entities, first_proxy));
    CHECK(!hth_entity_registry_is_alive(fixture.entities, first_enemy));

    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK);
    CHECK(!hth_entity_handle_equal(
        first_proxy, integration.player_target_bridge.target_entity));
    CHECK(!hth_entity_handle_equal(first_enemy, integration.enemy));
    CHECK(!hth_entity_registry_is_alive(fixture.entities, first_proxy));
    CHECK(!hth_entity_registry_is_alive(fixture.entities, first_enemy));
    CHECK(hth_bootstrap_enemy_pursuit_step(
              &integration, fixture.entities, fixture.actors,
              fixture.enemies, fixture.spatial, fixture.bodies,
              fixture.health, NULL, fixture.cadences, &fixture.world,
              &player, 0.0) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_PURSUIT_FAILED);
    CHECK(hth_player_target_bridge_destroy(
        &integration.player_target_bridge, fixture.entities,
        fixture.actors, fixture.spatial, fixture.bodies, fixture.health));
    CHECK(step_integration(&fixture, &integration, &player, 0.0F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_BRIDGE_SYNC_FAILED);
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_post_pursuit_runtime_visual(void)
{
    static const float white[4] = {1.0F, 1.0F, 1.0F, 1.0F};
    Fixture fixture;
    HTHBootstrapEnemyPursuit integration;
    HTHPlayerBody player = player_at(0.0F, 3.0F);
    HTHSpatialTransform post_pursuit;
    HTHDynamicBody body;
    HTHRendererTransientDraw draw;
    HTHVec4 center;
    HTHVec4 maximum;
    HTHEntityHandle enemy;
    HTHEntityHandle target_before;
    HTHEntityHandle target_after;

    CHECK(fixture_create(&fixture));
    hth_bootstrap_enemy_pursuit_initialize(&integration);
    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK);
    enemy = integration.enemy;
    CHECK(step_integration(&fixture, &integration, &player, 0.1F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &post_pursuit));
    CHECK(post_pursuit.position.x < 3.0F);
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, enemy, &body));
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &target_before));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, enemy, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_READY);
    center = hth_mat4_transform_vec4(
        draw.model, (HTHVec4){0.0F, 0.0F, 0.0F, 1.0F});
    maximum = hth_mat4_transform_vec4(
        draw.model, (HTHVec4){0.5F, 0.5F, 0.5F, 1.0F});
    CHECK(fabsf(center.x - post_pursuit.position.x) < 1.0e-6F &&
          fabsf(center.y - post_pursuit.position.y) < 1.0e-6F &&
          fabsf(center.z - post_pursuit.position.z) < 1.0e-6F);
    CHECK(fabsf(maximum.x -
                (post_pursuit.position.x + body.half_extents.x)) < 1.0e-6F &&
          fabsf(maximum.y -
                (post_pursuit.position.y + body.half_extents.y)) < 1.0e-6F &&
          fabsf(maximum.z -
                (post_pursuit.position.z + body.half_extents.z)) < 1.0e-6F);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &target_after));
    CHECK(hth_entity_handle_equal(target_before, target_after));
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    CHECK(hth_runtime_body_visual_build(
              fixture.entities, fixture.spatial, fixture.bodies, enemy, white,
              &draw) == HTH_RUNTIME_BODY_VISUAL_NOT_RENDERABLE);
    fixture_destroy(&fixture);
    return true;
}

static bool test_production_damage_cadence_and_zero_health(void)
{
    Fixture fixture;
    HTHBootstrapEnemyPursuit integration;
    HTHPlayerBody player = player_at(1.75F, 3.0F);
    HTHHealth health;
    HTHEntityHandle target;
    size_t index;

    CHECK(fixture_create(&fixture));
    hth_bootstrap_enemy_pursuit_initialize(&integration);
    CHECK(create_integration(&fixture, &integration, &player) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_CREATE_OK);
    target = integration.player_target_bridge.target_entity;
    CHECK(step_integration(&fixture, &integration, &player, 0.0F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &health));
    CHECK(health.current == 90.0F);
    CHECK(step_integration(&fixture, &integration, &player, 0.5F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &health));
    CHECK(health.current == 90.0F);
    CHECK(step_integration(&fixture, &integration, &player, 0.5F) ==
          HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &health));
    CHECK(health.current == 80.0F);
    for (index = 0U; index < 8U; ++index) {
        CHECK(step_integration(&fixture, &integration, &player, 1.0F) ==
              HTH_BOOTSTRAP_ENEMY_PURSUIT_STEP_OK);
    }
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &health));
    CHECK(health.current == 0.0F && health.maximum == 100.0F);
    CHECK(hth_entity_registry_is_alive(fixture.entities, target));
    CHECK(hth_entity_registry_is_alive(fixture.entities,
                                       integration.enemy));
    CHECK(cleanup_succeeded(cleanup_integration(&fixture, &integration)));
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    const struct {
        const char *name;
        bool (*run)(void);
    } tests[] = {
        {"canonical state/delta", test_canonical_state_and_delta},
        {"create composition/cleanup", test_create_composition_and_cleanup},
        {"create failures/partial cleanup",
         test_create_failures_and_partial_cleanup},
        {"acquisition/current sync/stable target",
         test_acquisition_current_sync_and_stable_target},
        {"Player movement precedes proxy sync",
         test_player_movement_precedes_proxy_sync},
        {"no-op policies/static collision",
         test_noop_policies_and_static_collision},
        {"attack range transitions", test_attack_range_transitions},
        {"failure/cleanup order/restart",
         test_failures_cleanup_order_and_restart},
        {"post-pursuit runtime visual", test_post_pursuit_runtime_visual},
        {"production damage/cadence/zero Health",
          test_production_damage_cadence_and_zero_health}
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index].run()) {
            fprintf(stderr, "FAILED: %s\n", tests[index].name);
            return EXIT_FAILURE;
        }
        printf("PASS: %s\n", tests[index].name);
    }
    return EXIT_SUCCESS;
}
