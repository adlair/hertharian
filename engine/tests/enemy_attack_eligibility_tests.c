#include "enemy_attack_eligibility.h"

#include "dynamic_body.h"
#include "enemy_runtime_population.h"
#include "enemy_target.h"
#include "health.h"
#include "player_body.h"
#include "player_target_bridge.h"

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
} Fixture;

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
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->cadences != NULL &&
           fixture->spatial != NULL &&
           fixture->bodies != NULL && fixture->health != NULL &&
           fixture->targets != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_target_store_destroy(fixture->targets);
    hth_enemy_attack_cadence_store_destroy(fixture->cadences);
    hth_health_store_destroy(fixture->health);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static HTHSpatialTransform transform(float x, float y, float z, float yaw)
{
    const HTHSpatialTransform value = {{x, y, z}, yaw};

    return value;
}

static HTHCollisionWorld clear_world(void)
{
    const HTHCollisionWorld world = {0};

    return world;
}

static HTHCollisionWorld world_with_box(HTHAABB box)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = box;
    world.obstacle_count = 1U;
    return world;
}

static bool create_entity(Fixture *fixture, HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity);
}

static bool create_spatial_entity(Fixture *fixture,
                                  HTHSpatialTransform value,
                                  HTHEntityHandle *out_entity)
{
    return create_entity(fixture, out_entity) &&
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_entity, &value);
}

static bool create_enemy(Fixture *fixture, HTHSpatialTransform value,
                         HTHEntityHandle *out_enemy)
{
    return create_entity(fixture, out_enemy) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_enemy) &&
           hth_enemy_store_attach(fixture->enemies, fixture->entities,
                                  fixture->actors, *out_enemy) &&
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_enemy, &value);
}

static bool evaluate(Fixture *fixture, const HTHCollisionWorld *world,
                     HTHEntityHandle enemy, HTHEntityHandle target,
                     float range, bool *out_eligible)
{
    return hth_enemy_attack_eligibility_evaluate(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, world, enemy, target, range, out_eligible);
}

static bool expect_structural_failure(
    const HTHEntityRegistry *entities, const HTHActorStore *actors,
    const HTHEnemyStore *enemies, const HTHSpatialStore *spatial,
    const HTHCollisionWorld *world, HTHEntityHandle enemy,
    HTHEntityHandle target, float range)
{
    bool eligible = true;

    return !hth_enemy_attack_eligibility_evaluate(
               entities, actors, enemies, spatial, world, enemy, target,
               range, &eligible) &&
           !eligible;
}

static bool test_pointer_scalar_and_output_contract(void)
{
    Fixture fixture;
    HTHCollisionWorld world = clear_world();
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle invalid = hth_entity_handle_invalid();

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial_entity(&fixture,
                                transform(1.0F, 0.0F, 0.0F, 0.0F),
                                &target));
    CHECK(expect_structural_failure(NULL, fixture.actors, fixture.enemies,
                                    fixture.spatial, &world, enemy, target,
                                    1.0F));
    CHECK(expect_structural_failure(fixture.entities, NULL, fixture.enemies,
                                    fixture.spatial, &world, enemy, target,
                                    1.0F));
    CHECK(expect_structural_failure(fixture.entities, fixture.actors, NULL,
                                    fixture.spatial, &world, enemy, target,
                                    1.0F));
    CHECK(expect_structural_failure(fixture.entities, fixture.actors,
                                    fixture.enemies, NULL, &world, enemy,
                                    target, 1.0F));
    CHECK(expect_structural_failure(fixture.entities, fixture.actors,
                                    fixture.enemies, fixture.spatial, NULL,
                                    enemy, target, 1.0F));
    CHECK(!hth_enemy_attack_eligibility_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &world, enemy, target, 1.0F, NULL));
    CHECK(expect_structural_failure(fixture.entities, fixture.actors,
                                    fixture.enemies, fixture.spatial, &world,
                                    enemy, target, -1.0F));
    CHECK(expect_structural_failure(fixture.entities, fixture.actors,
                                    fixture.enemies, fixture.spatial, &world,
                                    enemy, target, NAN));
    CHECK(expect_structural_failure(fixture.entities, fixture.actors,
                                    fixture.enemies, fixture.spatial, &world,
                                    enemy, target, INFINITY));
    CHECK(expect_structural_failure(fixture.entities, fixture.actors,
                                    fixture.enemies, fixture.spatial, &world,
                                    enemy, target, -INFINITY));
    CHECK(expect_structural_failure(fixture.entities, fixture.actors,
                                    fixture.enemies, fixture.spatial, &world,
                                    invalid, invalid, 0.0F));
    fixture_destroy(&fixture);
    return true;
}

static bool test_enemy_and_target_semantics(void)
{
    Fixture fixture;
    HTHCollisionWorld world = clear_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform near = transform(1.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle valid_enemy;
    HTHEntityHandle target;
    HTHEntityHandle entity;
    bool eligible = true;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, origin, &valid_enemy));
    CHECK(create_spatial_entity(&fixture, near, &target));

    CHECK(create_spatial_entity(&fixture, origin, &entity));
    CHECK(!evaluate(&fixture, &world, entity, target, 2.0F, &eligible));
    CHECK(!eligible);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, entity));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, entity, target, 2.0F, &eligible));
    CHECK(!eligible);

    CHECK(create_entity(&fixture, &entity));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, entity));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, entity));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, entity, target, 2.0F, &eligible));
    CHECK(!eligible);

    CHECK(create_entity(&fixture, &entity));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, valid_enemy, entity, 2.0F,
                    &eligible));
    CHECK(!eligible);
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, entity));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, valid_enemy, entity, 2.0F,
                    &eligible));
    CHECK(!eligible);

    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(evaluate(&fixture, &world, valid_enemy, target, 1.0F,
                   &eligible));
    CHECK(eligible);
    CHECK(!hth_health_store_has(fixture.health, fixture.entities,
                                fixture.actors, valid_enemy));
    CHECK(!hth_dynamic_body_has(fixture.bodies, fixture.entities,
                                valid_enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_self_and_range_geometry(void)
{
    Fixture fixture;
    HTHCollisionWorld world = clear_world();
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    bool eligible = true;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 1.0F),
                       &enemy));
    CHECK(evaluate(&fixture, &world, enemy, enemy, 0.0F, &eligible));
    CHECK(!eligible);
    CHECK(create_spatial_entity(&fixture,
                                transform(0.0F, 0.0F, 0.0F, -1.0F),
                                &target));
    CHECK(evaluate(&fixture, &world, enemy, target, 0.0F, &eligible));
    CHECK(eligible);

    CHECK(hth_spatial_store_set(
        fixture.spatial, fixture.entities, target,
        &(HTHSpatialTransform){{1.0F, 0.0F, 0.0F}, 0.0F}));
    CHECK(evaluate(&fixture, &world, enemy, target, 0.0F, &eligible));
    CHECK(!eligible);
    CHECK(evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(eligible);
    CHECK(hth_spatial_store_set(
        fixture.spatial, fixture.entities, target,
        &(HTHSpatialTransform){{nextafterf(1.0F, 0.0F), 0.0F, 0.0F},
                              0.0F}));
    CHECK(evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(eligible);
    CHECK(hth_spatial_store_set(
        fixture.spatial, fixture.entities, target,
        &(HTHSpatialTransform){{nextafterf(1.0F, 2.0F), 0.0F, 0.0F},
                              0.0F}));
    CHECK(evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(!eligible);
    CHECK(hth_spatial_store_set(
        fixture.spatial, fixture.entities, target,
        &(HTHSpatialTransform){{3.0F, 4.0F, 0.0F}, 0.0F}));
    CHECK(evaluate(&fixture, &world, enemy, target, 5.0F, &eligible));
    CHECK(eligible);
    CHECK(hth_spatial_store_set(
        fixture.spatial, fixture.entities, target,
        &(HTHSpatialTransform){{0.0F, 3.0F, 4.0F}, 0.0F}));
    CHECK(evaluate(&fixture, &world, enemy, target, 5.0F, &eligible));
    CHECK(eligible);
    CHECK(evaluate(&fixture, &world, enemy, target, 4.0F, &eligible));
    CHECK(!eligible);
    fixture_destroy(&fixture);
    return true;
}

static bool test_los_semantics(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = clear_world();
    HTHCollisionWorld blocked = world_with_box(
        (HTHAABB){{1.0F, -1.0F, -1.0F}, {2.0F, 1.0F, 1.0F}});
    HTHCollisionWorld start_solid = world_with_box(
        (HTHAABB){{-0.5F, -0.5F, -0.5F}, {0.5F, 0.5F, 0.5F}});
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    bool eligible = false;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial_entity(&fixture,
                                transform(3.0F, 0.0F, 0.0F, 0.0F),
                                &target));
    CHECK(evaluate(&fixture, &clear, enemy, target, 3.0F, &eligible));
    CHECK(eligible);
    CHECK(evaluate(&fixture, &blocked, enemy, target, 3.0F, &eligible));
    CHECK(!eligible);
    CHECK(evaluate(&fixture, &start_solid, enemy, target, 3.0F, &eligible));
    CHECK(!eligible);
    CHECK(evaluate(&fixture, &blocked, enemy, target, 2.0F, &eligible));
    CHECK(!eligible);
    fixture_destroy(&fixture);
    return true;
}

static bool test_optional_state_and_explicit_target(void)
{
    Fixture fixture;
    HTHCollisionWorld world = clear_world();
    HTHSpatialTransform enemy_transform = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform target_transform = transform(1.0F, 0.0F, 0.0F, 2.0F);
    HTHDynamicBody body = {{0.25F, 0.5F, 0.25F}, {0.0F, 0.0F, 0.0F}};
    HTHHealth zero_health = {0.0F, 100.0F};
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle current;
    bool eligible = false;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, enemy_transform, &enemy));
    CHECK(create_spatial_entity(&fixture, target_transform, &target));
    CHECK(evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(eligible);
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, enemy, zero_health));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target, zero_health));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, enemy, &body));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, target, &body));
    CHECK(evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(eligible);
    CHECK(hth_dynamic_body_set_velocity(
        fixture.bodies, fixture.entities, enemy,
        (HTHVec3){20.0F, -3.0F, 4.0F}));
    enemy_transform.yaw = -2.5F;
    target_transform.yaw = -1.0F;
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, enemy,
                                &enemy_transform));
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &target_transform));
    CHECK(hth_enemy_target_store_set(fixture.targets, fixture.entities,
                                     fixture.actors, fixture.enemies, enemy,
                                     target));
    CHECK(hth_enemy_target_store_get(fixture.targets, fixture.entities,
                                     fixture.actors, fixture.enemies, enemy,
                                     &current));
    CHECK(hth_entity_handle_equal(current, target));
    CHECK(evaluate(&fixture, &world, enemy, current, 1.0F, &eligible));
    CHECK(eligible);
    fixture_destroy(&fixture);
    return true;
}

static bool test_enemy_target_and_player_proxy_composition(void)
{
    Fixture fixture;
    HTHCollisionWorld world = clear_world();
    HTHPlayerTargetBridge bridge = {hth_entity_handle_invalid()};
    HTHPlayerBody player;
    const HTHEnemyRuntimeSpawnSpec runtime_spec = {
        {{0.0F, 0.0F, 0.0F}, 0.0F},
        {{0.25F, 0.5F, 0.25F}, {0.0F, 0.0F, 0.0F}},
        {100.0F, 100.0F}
    };
    HTHEntityHandle enemy_a;
    HTHEntityHandle enemy_b;
    HTHEntityHandle proxy;
    bool eligible = false;

    CHECK(fixture_create(&fixture));
    CHECK(hth_enemy_runtime_spawn(
        fixture.entities, fixture.actors, fixture.enemies, fixture.cadences,
        fixture.spatial,
        fixture.bodies, fixture.health, &runtime_spec, &enemy_a));
    CHECK(create_enemy(&fixture, transform(1.0F, 0.0F, 0.0F, 0.0F),
                       &enemy_b));
    CHECK(evaluate(&fixture, &world, enemy_a, enemy_b, 1.0F, &eligible));
    CHECK(eligible);
    CHECK(hth_player_body_init(&player, (HTHVec3){2.0F, -1.0F, 0.0F}));
    player.height = 2.0F;
    CHECK(hth_player_target_bridge_create(&bridge, fixture.entities,
                                          fixture.actors, fixture.spatial,
                                          fixture.bodies, fixture.health,
                                          &player,
                                          (HTHHealth){100.0F, 100.0F}));
    CHECK(hth_player_target_bridge_get_target(
        &bridge, fixture.entities, fixture.spatial, &proxy));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, proxy));
    CHECK(evaluate(&fixture, &world, enemy_a, proxy, 2.0F, &eligible));
    CHECK(eligible);
    CHECK(hth_player_target_bridge_destroy(&bridge, fixture.entities,
                                           fixture.actors, fixture.spatial,
                                           fixture.bodies, fixture.health));
    CHECK(hth_enemy_runtime_despawn(
        fixture.entities, fixture.actors, fixture.enemies, fixture.cadences,
        fixture.spatial,
        fixture.bodies, fixture.health, fixture.targets, enemy_a));
    fixture_destroy(&fixture);
    return true;
}

static bool test_remove_reattach_and_generation_safety(void)
{
    Fixture fixture;
    HTHCollisionWorld world = clear_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform near = transform(1.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle replacement;
    HTHEntityHandle target_replacement;
    bool eligible = true;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, origin, &enemy));
    CHECK(create_spatial_entity(&fixture, near, &target));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, enemy));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(!eligible);
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(eligible);
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, enemy));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(!eligible);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, enemy));
    CHECK(evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, enemy));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(!eligible);
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities, enemy,
                                   &origin));
    CHECK(evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, target));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(!eligible);
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities, target,
                                   &near));
    CHECK(evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));

    CHECK(hth_entity_registry_destroy_entity(fixture.entities, target));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(!eligible);
    CHECK(create_spatial_entity(&fixture, near, &target_replacement));
    CHECK(target_replacement.index == target.index);
    CHECK(target_replacement.generation != target.generation);
    CHECK(!evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(evaluate(&fixture, &world, enemy, target_replacement, 1.0F,
                   &eligible));
    CHECK(eligible);

    CHECK(hth_entity_registry_destroy_entity(fixture.entities, enemy));
    eligible = true;
    CHECK(!evaluate(&fixture, &world, enemy, target_replacement, 1.0F,
                    &eligible));
    CHECK(!eligible);
    CHECK(create_enemy(&fixture, origin, &replacement));
    CHECK(replacement.index == enemy.index);
    CHECK(replacement.generation != enemy.generation);
    CHECK(!evaluate(&fixture, &world, enemy, target, 1.0F, &eligible));
    CHECK(evaluate(&fixture, &world, replacement, target_replacement, 1.0F,
                   &eligible));
    CHECK(eligible);
    fixture_destroy(&fixture);
    return true;
}

static bool test_large_finite_determinism_and_purity(void)
{
    Fixture fixture;
    HTHCollisionWorld world = clear_world();
    HTHCollisionWorld world_before;
    HTHSpatialTransform enemy_before;
    HTHSpatialTransform target_before;
    HTHSpatialTransform enemy_after;
    HTHSpatialTransform target_after;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    size_t live_before;
    size_t index;
    bool eligible = false;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture,
                       transform(-FLT_MAX / 4.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial_entity(
        &fixture, transform(FLT_MAX / 4.0F, 0.0F, 0.0F, 0.0F), &target));
    live_before = hth_entity_registry_live_count(fixture.entities);
    world_before = world;
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &enemy_before));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &target_before));
    for (index = 0U; index < 128U; ++index) {
        CHECK(evaluate(&fixture, &world, enemy, target, FLT_MAX,
                       &eligible));
        CHECK(eligible);
    }
    CHECK(hth_entity_registry_live_count(fixture.entities) == live_before);
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, enemy));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, enemy));
    CHECK(!hth_enemy_target_store_has(fixture.targets, fixture.entities,
                                      fixture.actors, fixture.enemies,
                                      enemy));
    CHECK(memcmp(&world, &world_before, sizeof(world)) == 0);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &enemy_after));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &target_after));
    CHECK(memcmp(&enemy_before, &enemy_after, sizeof(enemy_before)) == 0);
    CHECK(memcmp(&target_before, &target_after, sizeof(target_before)) == 0);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_pointer_scalar_and_output_contract,
        test_enemy_and_target_semantics,
        test_self_and_range_geometry,
        test_los_semantics,
        test_optional_state_and_explicit_target,
        test_enemy_target_and_player_proxy_composition,
        test_remove_reattach_and_generation_safety,
        test_large_finite_determinism_and_purity
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy attack eligibility tests passed");
    return EXIT_SUCCESS;
}
