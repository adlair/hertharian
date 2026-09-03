#include "player_target_bridge.h"

#include "actor.h"
#include "damage_intent.h"
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
    HTHEnemyAttackCadenceStore *cadences;
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
    fixture->cadences = hth_enemy_attack_cadence_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
    fixture->targets = hth_enemy_target_store_create();
    return fixture->entities != NULL && fixture->spatial != NULL &&
           fixture->actors != NULL && fixture->enemies != NULL &&
           fixture->cadences != NULL &&
           fixture->bodies != NULL && fixture->health != NULL &&
           fixture->targets != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_target_store_destroy(fixture->targets);
    hth_enemy_attack_cadence_store_destroy(fixture->cadences);
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

static bool create_bridge(Fixture *fixture, HTHPlayerTargetBridge *bridge,
                          const HTHPlayerBody *player)
{
    return hth_player_target_bridge_create(
        bridge, fixture->entities, fixture->actors, fixture->spatial,
        fixture->bodies, fixture->health, player,
        (HTHHealth){100.0F, 100.0F});
}

static bool create_bridge_with_health(Fixture *fixture,
                                      HTHPlayerTargetBridge *bridge,
                                      const HTHPlayerBody *player,
                                      HTHHealth initial_health)
{
    return hth_player_target_bridge_create(
        bridge, fixture->entities, fixture->actors, fixture->spatial,
        fixture->bodies, fixture->health, player, initial_health);
}

static bool destroy_bridge(Fixture *fixture, HTHPlayerTargetBridge *bridge)
{
    return hth_player_target_bridge_destroy(
        bridge, fixture->entities, fixture->actors, fixture->spatial,
        fixture->bodies, fixture->health);
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
           hth_actor_store_has(fixture->actors, fixture->entities, entity) &&
           hth_spatial_store_has(fixture->spatial, fixture->entities,
                                 entity) &&
           hth_health_store_has(fixture->health, fixture->entities,
                                fixture->actors, entity) &&
           !hth_enemy_store_has(fixture->enemies, fixture->entities,
                                fixture->actors, entity) &&
           !hth_dynamic_body_has(fixture->bodies, fixture->entities,
                                 entity);
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
        fixture->cadences,
        fixture->spatial, fixture->bodies, fixture->health, &spec,
        out_enemy);
}

static bool destroy_enemy(Fixture *fixture, HTHEntityHandle enemy)
{
    return hth_enemy_runtime_despawn(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->cadences,
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
    HTHHealth health;
    HTHEntityHandle target = {3U, 7U};
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_bridge(&fixture, &bridge, &player));
    CHECK(player_equal(&player, &before));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &target));
    CHECK(hth_entity_handle_equal(target, bridge.target_entity));
    CHECK(bridge_has_exact_composition(&fixture, target));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, target, &health));
    CHECK(health.current == 100.0F && health.maximum == 100.0F);
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

    CHECK(!create_bridge(&fixture, &bridge, &player));
    CHECK(hth_entity_handle_equal(bridge.target_entity, target));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 1U);

    CHECK(destroy_bridge(&fixture, &bridge));
    CHECK(handle_invalid(bridge.target_entity));
    CHECK(!hth_entity_registry_is_alive(fixture.entities, target));
    CHECK(!hth_spatial_store_has(fixture.spatial, fixture.entities, target));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    CHECK(!destroy_bridge(&fixture, &bridge));
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
    const HTHHealth invalid_health[] = {
        {-1.0F, 100.0F},
        {101.0F, 100.0F},
        {0.0F, 0.0F},
        {0.0F, -1.0F},
        {NAN, 100.0F},
        {0.0F, NAN},
        {INFINITY, 100.0F},
        {-INFINITY, 100.0F},
        {0.0F, INFINITY},
        {0.0F, -INFINITY}
    };
    HTHHealth stored_health;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(!hth_player_target_bridge_create(
        NULL, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health, &player,
        (HTHHealth){100.0F, 100.0F}));
    CHECK(!hth_player_target_bridge_create(
        &bridge, NULL, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health, &player, (HTHHealth){100.0F, 100.0F}));
    CHECK(handle_invalid(bridge.target_entity));
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, NULL, fixture.spatial, fixture.bodies,
        fixture.health, &player, (HTHHealth){100.0F, 100.0F}));
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, NULL, fixture.bodies,
        fixture.health, &player, (HTHHealth){100.0F, 100.0F}));
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, fixture.spatial, NULL,
        fixture.health, &player, (HTHHealth){100.0F, 100.0F}));
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, NULL, &player, (HTHHealth){100.0F, 100.0F}));
    CHECK(!hth_player_target_bridge_create(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health, NULL,
        (HTHHealth){100.0F, 100.0F}));
    CHECK(!hth_player_target_bridge_create(
        &stale, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health, &player,
        (HTHHealth){100.0F, 100.0F}));
    CHECK(stale.target_entity.index == 4U &&
          stale.target_entity.generation == 9U);

    for (index = 0U;
         index < sizeof(invalid_health) / sizeof(invalid_health[0]);
         ++index) {
        CHECK(!create_bridge_with_health(
            &fixture, &bridge, &player, invalid_health[index]));
        CHECK(handle_invalid(bridge.target_entity));
        CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    }
    CHECK(create_bridge_with_health(
        &fixture, &bridge, &player, (HTHHealth){0.0F, 100.0F}));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, bridge.target_entity,
                               &stored_health));
    CHECK(stored_health.current == 0.0F &&
          stored_health.maximum == 100.0F);
    CHECK(destroy_bridge(&fixture, &bridge));

    invalid = player;
    invalid.height = 0.0F;
    CHECK(!create_bridge(&fixture, &bridge, &invalid));
    invalid = player;
    invalid.eye_height = invalid.height;
    CHECK(!create_bridge(&fixture, &bridge, &invalid));
    for (index = 0U; index < sizeof(nonfinite) / sizeof(nonfinite[0]);
         ++index) {
        invalid = player;
        invalid.position.x = nonfinite[index];
        CHECK(!create_bridge(&fixture, &bridge, &invalid));
        invalid = player;
        invalid.height = nonfinite[index];
        CHECK(!create_bridge(&fixture, &bridge, &invalid));
    }

    invalid = player;
    invalid.position.y = FLT_MAX;
    invalid.height = FLT_MAX;
    invalid.eye_height = 1.0F;
    CHECK(hth_player_body_is_valid(&invalid));
    CHECK(!create_bridge(&fixture, &bridge, &invalid));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);

    player.position.y = FLT_MAX / 4.0F;
    player.height = FLT_MAX / 4.0F;
    player.eye_height = 1.0F;
    CHECK(create_bridge(&fixture, &bridge, &player));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                bridge.target_entity, &transform));
    CHECK(transform_matches_player(transform, &player));
    CHECK(destroy_bridge(&fixture, &bridge));
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
    CHECK(create_bridge(&fixture, &bridge, &player));
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
    CHECK(destroy_bridge(&fixture, &bridge));
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
    CHECK(!destroy_bridge(&fixture, &bridge));
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
    CHECK(create_bridge(&fixture, &bridge, &player));
    stale = bridge.target_entity;
    CHECK(!hth_player_target_bridge_destroy(
        NULL, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, fixture.health));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, NULL, fixture.actors, fixture.spatial, fixture.bodies,
        fixture.health));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, NULL, fixture.spatial, fixture.bodies,
        fixture.health));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.actors, NULL, fixture.bodies,
        fixture.health));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.actors, fixture.spatial, NULL,
        fixture.health));
    CHECK(!hth_player_target_bridge_destroy(
        &bridge, fixture.entities, fixture.actors, fixture.spatial,
        fixture.bodies, NULL));
    CHECK(hth_entity_handle_equal(bridge.target_entity, stale));

    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, stale));
    CHECK(!hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    output = (HTHEntityHandle){2U, 3U};
    CHECK(!hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &output));
    CHECK(handle_invalid(output));
    CHECK(!destroy_bridge(&fixture, &bridge));
    CHECK(hth_entity_handle_equal(bridge.target_entity, stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));

    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(replacement.index == stale.index &&
          replacement.generation != stale.generation);
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   replacement, &replacement_transform));
    CHECK(!hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(!destroy_bridge(&fixture, &bridge));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                replacement, &observed));
    CHECK(memcmp(&observed, &replacement_transform, sizeof(observed)) == 0);
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   replacement));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, replacement));

    bridge = inactive_bridge();
    CHECK(create_bridge(&fixture, &bridge, &player));
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
    CHECK(!destroy_bridge(&fixture, &bridge));
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
    CHECK(create_bridge(&fixture, &bridge, &player));
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
        &world, fixture.targets, enemy, &target, 1U,
        hth_entity_handle_invalid(), 10.0F, &selected));
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
        fixture.bodies, fixture.health, fixture.targets, fixture.cadences,
        &world, candidates, 1U, hth_entity_handle_invalid(), 10.0F, 0.0F,
        2.0F, 0.0F, 1.0, 0.5));
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
        fixture.bodies, fixture.health, fixture.targets, fixture.cadences,
        &world, candidates, 1U, hth_entity_handle_invalid(), 10.0F, 0.0F,
        2.0F, 0.0F, 1.0, 0.5));
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
    CHECK(destroy_bridge(&fixture, &bridge));
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

static bool test_damage_target_identity_composition(void)
{
    Fixture fixture;
    HTHPlayerTargetBridge bridge = inactive_bridge();
    HTHPlayerBody player = player_body((HTHVec3){1.0F, 0.0F, 0.0F});
    HTHPlayerBody player_before;
    HTHCollisionWorld world = {
        {{{100.0F, 100.0F, 100.0F}, {101.0F, 101.0F, 101.0F}}}, 1U
    };
    HTHEntityHandle enemy;
    HTHEntityHandle player_target;
    HTHEntityHandle selected;
    HTHEntityHandle replacement;
    HTHEnemyIntent enemy_intent;
    HTHDamageIntent damage_intent;
    HTHDamageResolution resolution;
    HTHSpatialTransform player_spatial_before;
    HTHSpatialTransform player_spatial_after;
    HTHSpatialTransform enemy_spatial_before;
    HTHSpatialTransform enemy_spatial_after;
    HTHDynamicBody enemy_body_before;
    HTHDynamicBody enemy_body_after;
    HTHHealth player_health;
    HTHHealth enemy_health_before;
    HTHHealth enemy_health_after;
    HTHHealingResult healing;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_enemy(&fixture, (HTHVec3){0.0F, 1.0F, 0.0F}, &enemy));
    CHECK(create_bridge_with_health(
        &fixture, &bridge, &player, (HTHHealth){80.0F, 100.0F}));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &player_target));
    CHECK(bridge_has_exact_composition(&fixture, player_target));
    CHECK(hth_enemy_target_select(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &world, fixture.targets, enemy, &player_target, 1U,
        hth_entity_handle_invalid(), 10.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, player_target));
    CHECK(hth_enemy_decision_evaluate_with_attack(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &world, enemy, 10.0F, 2.0F, &enemy_intent));
    CHECK(enemy_intent.kind == HTH_ENEMY_INTENT_ATTACK &&
          hth_entity_handle_equal(enemy_intent.target, player_target));

    damage_intent = (HTHDamageIntent){enemy, player_target, 25.0F};
    CHECK(hth_damage_intent_is_valid(
        &damage_intent, fixture.entities, fixture.actors));
    player_before = player;
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                player_target, &player_spatial_before));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &enemy_spatial_before));
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, enemy,
                               &enemy_body_before));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, enemy, &enemy_health_before));
    CHECK(hth_damage_intent_resolve(
        &damage_intent, fixture.entities, fixture.actors, fixture.health,
        &resolution));
    CHECK(resolution.applied && resolution.damage.previous == 80.0F &&
          resolution.damage.current == 55.0F &&
          resolution.damage.applied == 25.0F &&
          !resolution.damage.became_zero);
    CHECK(player_equal(&player, &player_before));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                player_target, &player_spatial_after));
    CHECK(memcmp(&player_spatial_before, &player_spatial_after,
                 sizeof(player_spatial_before)) == 0);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &enemy_spatial_after));
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, enemy,
                               &enemy_body_after));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, enemy, &enemy_health_after));
    CHECK(memcmp(&enemy_spatial_before, &enemy_spatial_after,
                 sizeof(enemy_spatial_before)) == 0);
    CHECK(memcmp(&enemy_body_before, &enemy_body_after,
                 sizeof(enemy_body_before)) == 0);
    CHECK(memcmp(&enemy_health_before, &enemy_health_after,
                 sizeof(enemy_health_before)) == 0);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    CHECK(hth_entity_handle_equal(selected, player_target));

    player.position.x = 2.0F;
    CHECK(hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                player_target, &player_spatial_after));
    CHECK(transform_matches_player(player_spatial_after, &player));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, player_target,
                               &player_health));
    CHECK(player_health.current == 55.0F &&
          player_health.maximum == 100.0F);
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                              player_target));
    CHECK(hth_entity_handle_equal(bridge.target_entity, player_target));

    damage_intent.amount = 0.0F;
    CHECK(hth_damage_intent_resolve(
        &damage_intent, fixture.entities, fixture.actors, fixture.health,
        &resolution));
    CHECK(resolution.applied && resolution.damage.current == 55.0F &&
          resolution.damage.applied == 0.0F);
    damage_intent.amount = 1000.0F;
    CHECK(hth_damage_intent_resolve(
        &damage_intent, fixture.entities, fixture.actors, fixture.health,
        &resolution));
    CHECK(resolution.applied && resolution.damage.current == 0.0F &&
          resolution.damage.applied == 55.0F &&
          resolution.damage.became_zero);
    CHECK(bridge_has_exact_composition(&fixture, player_target));
    CHECK(hth_health_store_apply_healing(
        fixture.health, fixture.entities, fixture.actors, player_target,
        40.0F, &healing));
    CHECK(healing.previous == 0.0F && healing.current == 40.0F &&
          healing.applied == 40.0F);

    player.position = (HTHVec3){3.0F, 2.0F, -4.0F};
    CHECK(hth_player_target_bridge_sync(
        &bridge, fixture.entities, fixture.spatial, &player));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, player_target,
                               &player_health));
    CHECK(player_health.current == 40.0F && player_health.maximum == 100.0F);
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                              player_target));
    CHECK(hth_entity_handle_equal(bridge.target_entity, player_target));

    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, player_target));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &selected));
    CHECK(hth_entity_handle_equal(selected, player_target));
    CHECK(hth_damage_intent_is_valid(
        &damage_intent, fixture.entities, fixture.actors));
    CHECK(hth_damage_intent_resolve(
        &damage_intent, fixture.entities, fixture.actors, fixture.health,
        &resolution));
    CHECK(!resolution.applied);
    CHECK(hth_health_store_attach(
        fixture.health, fixture.entities, fixture.actors, player_target,
        (HTHHealth){40.0F, 100.0F}));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 player_target));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &selected));
    CHECK(hth_entity_handle_equal(selected, player_target));
    CHECK(!hth_damage_intent_is_valid(
        &damage_intent, fixture.entities, fixture.actors));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 player_target));
    CHECK(hth_damage_intent_is_valid(
        &damage_intent, fixture.entities, fixture.actors));

    CHECK(destroy_bridge(&fixture, &bridge));
    CHECK(!hth_damage_intent_is_valid(
        &damage_intent, fixture.entities, fixture.actors));
    CHECK(!hth_entity_registry_is_alive(fixture.entities, player_target));
    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities,
                               player_target));
    CHECK(!hth_spatial_store_has(fixture.spatial, fixture.entities,
                                 player_target));
    CHECK(!hth_health_store_has(fixture.health, fixture.entities,
                                fixture.actors, player_target));
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    CHECK(create_bridge(&fixture, &bridge, &player));
    replacement = bridge.target_entity;
    CHECK(replacement.index == player_target.index &&
          replacement.generation != player_target.generation);
    CHECK(!hth_damage_intent_is_valid(
        &damage_intent, fixture.entities, fixture.actors));
    CHECK(!hth_health_store_get(fixture.health, fixture.entities,
                                fixture.actors, player_target,
                                &player_health));
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &selected));
    damage_intent.target = replacement;
    damage_intent.amount = 10.0F;
    CHECK(hth_damage_intent_is_valid(
        &damage_intent, fixture.entities, fixture.actors));

    CHECK(destroy_bridge(&fixture, &bridge));
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
    CHECK(create_bridge(&fixture, &first, &first_player));
    CHECK(create_bridge(&fixture, &second, &second_player));
    CHECK(!hth_entity_handle_equal(first.target_entity,
                                   second.target_entity));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 2U);
    first_player.position.x = 9.0F;
    CHECK(hth_player_target_bridge_sync(
        &first, fixture.entities, fixture.spatial, &first_player));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities,
                                second.target_entity, &transform));
    CHECK(transform_matches_player(transform, &second_player));
    CHECK(destroy_bridge(&fixture, &first));
    CHECK(hth_entity_registry_is_alive(fixture.entities,
                                       second.target_entity));
    CHECK(destroy_bridge(&fixture, &second));
    first = inactive_bridge();
    CHECK(create_bridge(&other_fixture, &first, &first_player));
    CHECK(hth_entity_registry_live_count(fixture.entities) == 0U);
    CHECK(hth_entity_registry_live_count(other_fixture.entities) == 1U);
    CHECK(destroy_bridge(&other_fixture, &first));
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
        {"damage target identity composition",
         test_damage_target_identity_composition},
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
