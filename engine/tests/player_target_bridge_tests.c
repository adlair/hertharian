#include "player_target_bridge.h"

#include "actor.h"
#include "dynamic_body.h"
#include "enemy.h"
#include "enemy_decision.h"
#include "enemy_los.h"
#include "enemy_perception.h"
#include "enemy_pursuit_runtime.h"
#include "enemy_runtime_population.h"
#include "enemy_seek.h"
#include "enemy_target.h"
#include "enemy_target_selection.h"
#include "health.h"

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
    HTHSpatialStore *spatial;
    HTHActorStore *actors;
    HTHEnemyStore *enemies;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
    HTHEnemyTargetStore *targets;
} Fixture;

static HTHPlayerTargetBridge inactive_bridge(void)
{
    HTHPlayerTargetBridge bridge = {hth_entity_handle_invalid()};

    return bridge;
}

static bool handle_invalid(HTHEntityHandle handle)
{
    return hth_entity_handle_equal(handle, hth_entity_handle_invalid());
}

static bool player_equal(const HTHPlayerBody *left,
                         const HTHPlayerBody *right)
{
    return left->position.x == right->position.x &&
           left->position.y == right->position.y &&
           left->position.z == right->position.z &&
           left->velocity.x == right->velocity.x &&
           left->velocity.y == right->velocity.y &&
           left->velocity.z == right->velocity.z &&
           left->half_width == right->half_width &&
           left->height == right->height &&
           left->eye_height == right->eye_height &&
           left->grounded == right->grounded;
}

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->actors = hth_actor_store_create();
    fixture->enemies = hth_enemy_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
    fixture->targets = hth_enemy_target_store_create();
    return fixture->entities != NULL && fixture->spatial != NULL &&
           fixture->actors != NULL && fixture->enemies != NULL &&
           fixture->bodies != NULL && fixture->health != NULL &&
           fixture->targets != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_target_store_destroy(fixture->targets);
    hth_health_store_destroy(fixture->health);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_spatial_store_destroy(fixture->spatial);
    hth_entity_registry_destroy(fixture->entities);
}

static HTHPlayerBody player_body(HTHVec3 position)
{
    HTHPlayerBody player;

    if (!hth_player_body_init(&player, position)) {
        abort();
    }
    player.height = 2.0F;
    player.eye_height = 1.5F;
    return player;
}

static bool transform_matches_player(HTHSpatialTransform transform,
                                     const HTHPlayerBody *player)
{
    const float anchor_y = (float)((double)player->position.y +
                                   (double)player->height * 0.5);

    return transform.position.x == player->position.x &&
           transform.position.y == anchor_y &&
           transform.position.z == player->position.z &&
           transform.yaw == 0.0F;
}

static bool bridge_has_exact_composition(const Fixture *fixture,
                                         HTHEntityHandle entity)
{
    return hth_entity_registry_is_alive(fixture->entities, entity) &&
           hth_spatial_store_has(fixture->spatial, fixture->entities,
                                 entity) &&
           !hth_actor_store_has(fixture->actors, fixture->entities, entity) &&
           !hth_enemy_store_has(fixture->enemies, fixture->entities,
                                fixture->actors, entity) &&
           !hth_dynamic_body_has(fixture->bodies, fixture->entities,
                                 entity) &&
           !hth_health_store_has(fixture->health, fixture->entities,
                                 fixture->actors, entity);
}

static bool spawn_enemy(Fixture *fixture, HTHVec3 position,
                        HTHEntityHandle *out_enemy)
{
    const HTHEnemyRuntimeSpawnSpec spec = {
        {position, 0.0F},
        {{0.25F, 0.5F, 0.25F}, {0.0F, 0.0F, 0.0F}},
        {100.0F, 100.0F}
    };

    return hth_enemy_runtime_spawn(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, fixture->bodies, fixture->health, &spec,
        out_enemy);
}

static bool destroy_enemy(Fixture *fixture, HTHEntityHandle enemy)
{
    return hth_enemy_runtime_despawn(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, fixture->bodies, fixture->health, fixture->targets,
        enemy);
}

static bool test_create_get_and_exact_composition(void)
{
    Fixture fixture;
    HTHPlayerTargetBridge bridge = inactive_bridge();
    HTHPlayerBody player = player_body((HTHVec3){2.0F, 3.0F, -4.0F});
    HTHPlayerBody before = player;
    HTHSpatialTransform transform;
    HTHEntityHandle target = {3U, 7U};
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(player_equal(&player, &before));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &target));
    CHECK(hth_entity_handle_equal(target, bridge.target_entity));
    CHECK(bridge_has_exact_composition(&fixture, target));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &transform));
    CHECK(transform_matches_player(transform, &player));
    for (index = 0U; index < 32U; ++index) {
        HTHEntityHandle repeated = {3U, 7U};

        CHECK(hth_player_target_bridge_get_target(
            &bridge, fixture.entities, fixture.spatial, &repeated));
        CHECK(hth_entity_handle_equal(repeated, target));
    }

    player.position = (HTHVec3){10.0F, 20.0F, 30.0F};
    player.height = 6.0F;
    CHECK(hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &transform));
    CHECK(transform.position.x == 10.0F &&
          transform.position.y == 23.0F &&
          transform.position.z == 30.0F);
    player.position = (HTHVec3){-5.0F, -2.0F, 7.0F};
    player.height = 4.0F;
    CHECK(hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &transform));
    CHECK(transform.position.x == -5.0F &&
          transform.position.y == 0.0F &&
          transform.position.z == 7.0F);

    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(hth_entity_handle_equal(bridge.target_entity, target));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 1U);

    CHECK(hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    CHECK(handle_invalid(bridge.target_entity));
    CHECK(!hth_entity_registry_is_alive(fixture.entities, target));
    CHECK(!hth_spatial_store_has(fixture.spatial, fixture.entities, target));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    fixture_destroy(&fixture);
    return true;
}

static bool test_create_validation_and_representability(void)
{
    Fixture fixture;
    HTHPlayerTargetBridge bridge = inactive_bridge();
    HTHPlayerTargetBridge stale = {{4U, 9U}};
    HTHPlayerBody player = player_body((HTHVec3){0.0F, 0.0F, 0.0F});
    HTHPlayerBody invalid;
    HTHSpatialTransform transform;
    const float nonfinite[] = {NAN, INFINITY, -INFINITY};
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(!hth_player_target_bridge_create(
        NULL, fixture.entities, fixture.spatial, &player));
    CHECK(!hth_player_target_bridge_create(
        &bridge, NULL, fixture.spatial, &player));
    CHECK(handle_invalid(bridge.target_entity));
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, NULL, &player));
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, NULL));
    CHECK(!hth_player_target_bridge_create(
        &stale, fixture.entities, fixture.spatial, &player));
    CHECK(stale.target_entity.index == 4U &&
          stale.target_entity.generation == 9U);

    invalid = player;
    invalid.height = 0.0F;
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &invalid));
    invalid = player;
    invalid.eye_height = invalid.height;
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &invalid));
    for (index = 0U; index < sizeof(nonfinite) / sizeof(nonfinite[0]);
         ++index) {
        invalid = player;
        invalid.position.x = nonfinite[index];
        CHECK(!hth_player_target_bridge_create(
            &bridge, fixture.entities, fixture.spatial, &invalid));
        invalid = player;
        invalid.height = nonfinite[index];
        CHECK(!hth_player_target_bridge_create(
            &bridge, fixture.entities, fixture.spatial, &invalid));
    }

    invalid = player;
    invalid.position.y = FLT_MAX;
    invalid.height = FLT_MAX;
    invalid.eye_height = 1.0F;
    CHECK(hth_player_body_is_valid(&invalid));
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &invalid));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);

    player.position.y = FLT_MAX / 4.0F;
    player.height = FLT_MAX / 4.0F;
    player.eye_height = 1.0F;
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                bridge.target_entity, &transform));
    CHECK(transform_matches_player(transform, &player));
    CHECK(hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    fixture_destroy(&fixture);
    return true;
}

static bool test_sync_contract_and_churn(void)
{
    Fixture fixture;
    HTHPlayerTargetBridge bridge = inactive_bridge();
    HTHPlayerBody player = player_body((HTHVec3){0.0F, 0.0F, 0.0F});
    HTHPlayerBody invalid;
    HTHSpatialTransform before;
    HTHSpatialTransform after;
    HTHEntityHandle target;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &player));
    target = bridge.target_entity;
    for (index = 0U; index < 256U; ++index) {
        player.position.x = (float)index * 0.25F;
        player.position.y = -(float)index * 0.125F;
        player.position.z = (float)index * -0.5F;
        player.velocity = (HTHVec3){(float)index, -(float)index, 9.0F};
        player.grounded = (index % 2U) != 0U;
        CHECK(hth_player_target_bridge_sync(
            &bridge, fixture.entities, fixture.spatial, &player));
        CHECK(hth_entity_handle_equal(bridge.target_entity, target));
        CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                    &after));
        CHECK(transform_matches_player(after, &player));
    }
    CHECK(hth_entity_registry_live_count(fixture.entities) == 1U);

    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &before));
    player.velocity = (HTHVec3){-500.0F, 300.0F, 700.0F};
    CHECK(hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);

    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &before));
    invalid = player;
    invalid.position.z = NAN;
    CHECK(!hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &invalid));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);
    CHECK(hth_entity_handle_equal(bridge.target_entity, target));
    invalid = player;
    invalid.position.y = FLT_MAX;
    invalid.height = FLT_MAX;
    invalid.eye_height = 1.0F;
    CHECK(hth_player_body_is_valid(&invalid));
    CHECK(!hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &invalid));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);

    CHECK(!hth_player_target_bridge_sync(
        NULL, fixture.entities, fixture.spatial, &player));
    CHECK(!hth_player_target_bridge_sync(
        &bridge, NULL, fixture.spatial, &player));
    CHECK(!hth_player_target_bridge_sync(
        &bridge, fixture.entities, NULL, &player));
    CHECK(!hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, NULL));
    CHECK(hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    fixture_destroy(&fixture);
    return true;
}

static bool test_stale_handles_and_destroy_contract(void)
{
    Fixture fixture;
    HTHPlayerTargetBridge bridge = inactive_bridge();
    HTHPlayerBody player = player_body((HTHVec3){1.0F, 2.0F, 3.0F});
    HTHSpatialTransform replacement_transform = {
        {10.0F, 11.0F, 12.0F}, 0.75F
    };
    HTHSpatialTransform observed;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHEntityHandle output = {2U, 3U};

    CHECK(fixture_create(&fixture));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    CHECK(!hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &output));
    CHECK(handle_invalid(output));
    output = (HTHEntityHandle){2U, 3U};
    CHECK(!hth_player_target_bridge_get_target(
        NULL, fixture.entities, fixture.spatial, &output));
    CHECK(handle_invalid(output));
    output = (HTHEntityHandle){2U, 3U};
    CHECK(!hth_player_target_bridge_get_target(
        &bridge, NULL, fixture.spatial, &output));
    CHECK(handle_invalid(output));
    output = (HTHEntityHandle){2U, 3U};
    CHECK(!hth_player_target_bridge_get_target(
        &bridge, fixture.entities, NULL, &output));
    CHECK(handle_invalid(output));
    CHECK(!hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, NULL));
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &player));
    stale = bridge.target_entity;
    CHECK(!hth_player_target_bridge_destroy(
        NULL, fixture.entities, fixture.spatial));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, NULL, fixture.spatial));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, NULL));
    CHECK(hth_entity_handle_equal(bridge.target_entity, stale));

    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, stale));
    CHECK(!hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    output = (HTHEntityHandle){2U, 3U};
    CHECK(!hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &output));
    CHECK(handle_invalid(output));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    CHECK(hth_entity_handle_equal(bridge.target_entity, stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));

    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(replacement.index == stale.index &&
          replacement.generation != stale.generation);
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   replacement, &replacement_transform));
    CHECK(!hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                replacement, &observed));
    CHECK(memcmp(&observed, &replacement_transform, sizeof(observed)) == 0);
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   replacement));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, replacement));

    bridge = inactive_bridge();
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &player));
    stale = bridge.target_entity;
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(!hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(!hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &output));
    CHECK(handle_invalid(output));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(replacement.index == stale.index &&
          replacement.generation != stale.generation);
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   replacement, &replacement_transform));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                replacement, &observed));
    CHECK(memcmp(&observed, &replacement_transform, sizeof(observed)) == 0);
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   replacement));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, replacement));
    fixture_destroy(&fixture);
    return true;
}

static bool test_ai_foundation_composition(void)
{
    Fixture fixture;
    HTHPlayerTargetBridge bridge = inactive_bridge();
    HTHPlayerBody player = player_body((HTHVec3){3.0F, 0.0F, 0.0F});
    HTHPlayerBody before = player;
    HTHCollisionWorld world = {
        {{{100.0F, 100.0F, 100.0F}, {101.0F, 101.0F, 101.0F}}}, 1U
    };
    HTHCollisionWorld blocked_world = {
        {{{1.0F, 0.5F, -0.5F}, {2.0F, 1.5F, 0.5F}}}, 1U
    };
    HTHEnemyIntent intent;
    HTHVec3 direction;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle selected;
    HTHSpatialTransform enemy_transform;
    HTHSpatialTransform proxy_transform;
    const HTHEntityHandle *candidates;
    uint32_t proxy_index;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_enemy(&fixture, (HTHVec3){0.0F, 1.0F, 0.0F}, &enemy));
    CHECK(hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &target));
    CHECK(hth_enemy_perception_can_perceive(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        enemy, target, 10.0F));
    CHECK(hth_enemy_los_has_line_of_sight(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &world, enemy, target));
    CHECK(!hth_enemy_los_has_line_of_sight(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &blocked_world, enemy, target));
    CHECK(hth_enemy_target_select(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &world, fixture.targets, enemy, &target, 1U, 10.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, target));
    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &blocked_world, enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_IDLE);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    CHECK(hth_entity_handle_equal(selected, target));
    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &world, enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_PURSUE &&
          hth_entity_handle_equal(intent.target, target));
    CHECK(hth_enemy_seek_compute(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        enemy, target, &direction));
    CHECK(direction.x == 1.0F && direction.y == 0.0F &&
          direction.z == 0.0F);

    CHECK(hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                       enemy));
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    candidates = &target;
    CHECK(hth_enemy_pursuit_runtime_step(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, fixture.targets, &world, candidates, 1U, 10.0F,
        2.0F, 0.5F));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &enemy_transform));
    CHECK(enemy_transform.position.x == 1.0F &&
          enemy_transform.position.y == 1.0F &&
          enemy_transform.position.z == 0.0F);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    CHECK(hth_entity_handle_equal(selected, target));
    CHECK(player_equal(&player, &before));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &proxy_transform));
    CHECK(transform_matches_player(proxy_transform, &player));

    player.position = (HTHVec3){1.0F, 0.0F, 4.0F};
    before = player;
    CHECK(hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(player_equal(&player, &before));
    CHECK(hth_enemy_seek_compute(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        enemy, target, &direction));
    CHECK(direction.x == 0.0F && direction.y == 0.0F &&
          direction.z == 1.0F);
    CHECK(hth_enemy_pursuit_runtime_step(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, fixture.targets, &world, candidates, 1U, 10.0F,
        2.0F, 0.5F));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &enemy_transform));
    CHECK(enemy_transform.position.x == 1.0F &&
          enemy_transform.position.y == 1.0F &&
          enemy_transform.position.z == 1.0F);
    CHECK(player_equal(&player, &before));
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    CHECK(hth_entity_handle_equal(selected, target));

    player.position.x = 20.0F;
    CHECK(hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(!hth_enemy_perception_can_perceive(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        enemy, target, 10.0F));
    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &world, enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_IDLE);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    CHECK(hth_entity_handle_equal(selected, target));

    proxy_index = target.index;
    CHECK(hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.spatial));
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &target));
    CHECK(handle_invalid(target));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &selected));
    CHECK(selected.index == proxy_index);
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &target));
    CHECK(handle_invalid(target));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, selected));
    CHECK(destroy_enemy(&fixture, enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_multiple_bridges_are_independent(void)
{
    Fixture fixture;
    Fixture other_fixture;
    HTHPlayerTargetBridge first = inactive_bridge();
    HTHPlayerTargetBridge second = inactive_bridge();
    HTHPlayerBody first_player = player_body((HTHVec3){1.0F, 0.0F, 0.0F});
    HTHPlayerBody second_player = player_body((HTHVec3){2.0F, 0.0F, 0.0F});
    HTHSpatialTransform transform;

    CHECK(fixture_create(&fixture));
    CHECK(fixture_create(&other_fixture));
    CHECK(hth_player_target_bridge_create(
        &first, fixture.entities, fixture.spatial, &first_player));
    CHECK(hth_player_target_bridge_create(
        &second, fixture.entities, fixture.spatial, &second_player));
    CHECK(!hth_entity_handle_equal(first.target_entity,
                                   second.target_entity));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 2U);
    first_player.position.x = 9.0F;
    CHECK(hth_player_target_bridge_sync(
        &first, fixture.entities, fixture.spatial, &first_player));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                second.target_entity, &transform));
    CHECK(transform_matches_player(transform, &second_player));
    CHECK(hth_player_target_bridge_destroy(
        &first, fixture.entities, fixture.spatial));
    CHECK(hth_entity_registry_is_alive(fixture.entities,
                                       second.target_entity));
    CHECK(hth_player_target_bridge_destroy(
        &second, fixture.entities, fixture.spatial));
    first = inactive_bridge();
    CHECK(hth_player_target_bridge_create(
        &first, other_fixture.entities, other_fixture.spatial,
        &first_player));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    CHECK(hth_entity_registry_live_count(other_fixture.entities) == 1U);
    CHECK(hth_player_target_bridge_destroy(
        &first, other_fixture.entities, other_fixture.spatial));
    fixture_destroy(&other_fixture);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    const struct {
        const char *name;
        bool (*run)(void);
    } tests[] = {
        {"create/get/exact composition", test_create_get_and_exact_composition},
        {"create validation/representability",
         test_create_validation_and_representability},
        {"sync contract/churn", test_sync_contract_and_churn},
        {"stale handles/destroy", test_stale_handles_and_destroy_contract},
        {"AI foundation composition", test_ai_foundation_composition},
        {"multiple bridges", test_multiple_bridges_are_independent}
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
