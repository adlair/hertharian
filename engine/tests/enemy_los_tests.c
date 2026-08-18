#include "enemy_los.h"

#include "dynamic_body.h"
#include "enemy_perception.h"
#include "enemy_target.h"
#include "health.h"

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
    HTHEnemyTargetStore *targets;
    HTHSpatialStore *spatial;
    HTHDynamicBodyStore *bodies;
    HTHHealthStore *health;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->enemies = hth_enemy_store_create();
    fixture->targets = hth_enemy_target_store_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->targets != NULL &&
           fixture->spatial != NULL && fixture->bodies != NULL &&
           fixture->health != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_target_store_destroy(fixture->targets);
    hth_health_store_destroy(fixture->health);
    hth_enemy_store_destroy(fixture->enemies);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static HTHSpatialTransform transform(float x, float y, float z, float yaw)
{
    HTHSpatialTransform value = {{x, y, z}, yaw};

    return value;
}

static HTHCollisionWorld empty_world(void)
{
    HTHCollisionWorld world = {0};

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

static bool create_candidate(Fixture *fixture,
                             HTHSpatialTransform value,
                             HTHEntityHandle *out_candidate)
{
    return create_entity(fixture, out_candidate) &&
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_candidate, &value);
}

static bool create_enemy(Fixture *fixture,
                         HTHSpatialTransform value,
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

static bool has_los(Fixture *fixture,
                    const HTHCollisionWorld *world,
                    HTHEntityHandle enemy,
                    HTHEntityHandle candidate)
{
    return hth_enemy_los_has_line_of_sight(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, world, enemy, candidate);
}

static bool set_target(Fixture *fixture, HTHEntityHandle enemy,
                       HTHEntityHandle candidate)
{
    return hth_enemy_target_store_set(
        fixture->targets, fixture->entities, fixture->actors,
        fixture->enemies, enemy, candidate);
}

static bool target_equals(Fixture *fixture, HTHEntityHandle enemy,
                          HTHEntityHandle expected)
{
    HTHEntityHandle actual;

    return hth_enemy_target_store_get(
               fixture->targets, fixture->entities, fixture->actors,
               fixture->enemies, enemy, &actual) &&
           hth_entity_handle_equal(actual, expected);
}

static bool test_null_and_trace_failure_contracts(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHCollisionWorld invalid = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_candidate(&fixture, transform(2.0F, 0.0F, 0.0F, 0.0F),
                           &candidate));
    CHECK(!hth_enemy_los_has_line_of_sight(
        NULL, fixture.actors, fixture.enemies, fixture.spatial, &clear,
        enemy, candidate));
    CHECK(!hth_enemy_los_has_line_of_sight(
        fixture.entities, NULL, fixture.enemies, fixture.spatial, &clear,
        enemy, candidate));
    CHECK(!hth_enemy_los_has_line_of_sight(
        fixture.entities, fixture.actors, NULL, fixture.spatial, &clear,
        enemy, candidate));
    CHECK(!hth_enemy_los_has_line_of_sight(
        fixture.entities, fixture.actors, fixture.enemies, NULL, &clear,
        enemy, candidate));
    CHECK(!has_los(&fixture, NULL, enemy, candidate));
    invalid.obstacle_count = HTH_COLLISION_WORLD_MAX_OBSTACLES + 1U;
    CHECK(!has_los(&fixture, &invalid, enemy, candidate));
    fixture_destroy(&fixture);
    return true;
}

static bool test_clear_blocked_and_finite_segment_semantics(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHCollisionWorld wall = world_with_box(
        (HTHAABB){{4.0F, -1.0F, -1.0F}, {5.0F, 1.0F, 1.0F}});
    HTHCollisionWorld endpoint = world_with_box(
        (HTHAABB){{10.0F, -1.0F, -1.0F}, {11.0F, 1.0F, 1.0F}});
    HTHCollisionWorld behind = world_with_box(
        (HTHAABB){{11.0F, -1.0F, -1.0F}, {12.0F, 1.0F, 1.0F}});
    HTHCollisionWorld off_axis = world_with_box(
        (HTHAABB){{4.0F, 2.0F, -1.0F}, {5.0F, 3.0F, 1.0F}});
    HTHCollisionWorld start_solid = world_with_box(
        (HTHAABB){{-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}});
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_candidate(&fixture, transform(10.0F, 0.0F, 0.0F, 0.0F),
                           &candidate));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    CHECK(!has_los(&fixture, &wall, enemy, candidate));
    CHECK(!has_los(&fixture, &endpoint, enemy, candidate));
    CHECK(has_los(&fixture, &behind, enemy, candidate));
    CHECK(has_los(&fixture, &off_axis, enemy, candidate));
    CHECK(!has_los(&fixture, &start_solid, enemy, candidate));
    fixture_destroy(&fixture);
    return true;
}

static bool test_zero_length_self_and_non_actor_candidate(void)
{
    Fixture fixture;
    HTHCollisionWorld occupied = world_with_box(
        (HTHAABB){{-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}});
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, origin, &enemy));
    CHECK(create_candidate(&fixture, origin, &candidate));
    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities, candidate));
    CHECK(has_los(&fixture, &occupied, enemy, candidate));
    CHECK(has_los(&fixture, &occupied, enemy, enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_semantic_requirements_remove_and_reattach(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHSpatialTransform observer = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform point = transform(2.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;
    HTHEntityHandle actor_only;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, observer, &enemy));
    CHECK(create_candidate(&fixture, point, &candidate));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   candidate));
    CHECK(!has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   candidate, &point));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, enemy));
    CHECK(!has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   enemy, &observer));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, enemy));
    CHECK(!has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    CHECK(create_entity(&fixture, &actor_only));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 actor_only));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   actor_only, &observer));
    CHECK(!has_los(&fixture, &clear, actor_only, candidate));
    fixture_destroy(&fixture);
    return true;
}

static bool test_dead_and_generation_reuse(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHSpatialTransform observer = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform point = transform(2.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle old_enemy;
    HTHEntityHandle new_enemy;
    HTHEntityHandle old_candidate;
    HTHEntityHandle new_candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, observer, &old_enemy));
    CHECK(create_candidate(&fixture, point, &old_candidate));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities,
                                              old_candidate));
    CHECK(!has_los(&fixture, &clear, old_enemy, old_candidate));
    CHECK(create_candidate(&fixture, point, &new_candidate));
    CHECK(new_candidate.index == old_candidate.index);
    CHECK(new_candidate.generation != old_candidate.generation);
    CHECK(!has_los(&fixture, &clear, old_enemy, old_candidate));
    CHECK(has_los(&fixture, &clear, old_enemy, new_candidate));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, old_enemy));
    CHECK(!has_los(&fixture, &clear, old_enemy, new_candidate));
    CHECK(create_enemy(&fixture, observer, &new_enemy));
    CHECK(new_enemy.index == old_enemy.index);
    CHECK(new_enemy.generation != old_enemy.generation);
    CHECK(!has_los(&fixture, &clear, old_enemy, new_candidate));
    CHECK(has_los(&fixture, &clear, new_enemy, new_candidate));
    fixture_destroy(&fixture);
    return true;
}

static bool test_candidate_actor_health_body_and_yaw_independence(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHSpatialTransform observer = transform(0.0F, 0.0F, 0.0F, -2.5F);
    HTHSpatialTransform point = transform(2.0F, 0.0F, 0.0F, 1.0F);
    HTHDynamicBody body = {{0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 0.0F}};
    HTHHealth zero_health = {0.0F, 100.0F};
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, observer, &enemy));
    CHECK(create_candidate(&fixture, point, &candidate));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 candidate));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, enemy, zero_health));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, candidate, zero_health));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, enemy, &body));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, candidate, &body));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities, enemy));
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities,
                                  candidate));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, enemy));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, candidate));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 candidate));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    observer.yaw = 2.75F;
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, enemy,
                                &observer));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    fixture_destroy(&fixture);
    return true;
}

static bool test_target_preservation_and_candidate_independence(void)
{
    Fixture fixture;
    HTHCollisionWorld wall = world_with_box(
        (HTHAABB){{4.0F, -1.0F, -1.0F}, {5.0F, 1.0F, 1.0F}});
    HTHCollisionWorld clear = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle candidate_a;
    HTHEntityHandle candidate_b;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_candidate(&fixture, transform(10.0F, 0.0F, 0.0F, 0.0F),
                           &candidate_a));
    CHECK(create_candidate(&fixture, transform(2.0F, 0.0F, 0.0F, 0.0F),
                           &candidate_b));
    CHECK(!hth_enemy_target_store_has(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy));
    CHECK(has_los(&fixture, &clear, enemy, candidate_b));
    CHECK(!hth_enemy_target_store_has(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy));
    CHECK(set_target(&fixture, enemy, candidate_a));
    CHECK(!has_los(&fixture, &wall, enemy, candidate_a));
    CHECK(target_equals(&fixture, enemy, candidate_a));
    CHECK(has_los(&fixture, &clear, enemy, candidate_b));
    CHECK(target_equals(&fixture, enemy, candidate_a));
    fixture_destroy(&fixture);
    return true;
}

static bool test_perception_orthogonality(void)
{
    Fixture fixture;
    HTHCollisionWorld wall = world_with_box(
        (HTHAABB){{1.0F, -1.0F, -1.0F}, {1.5F, 1.0F, 1.0F}});
    HTHCollisionWorld clear = empty_world();
    HTHSpatialTransform near = transform(2.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform far = transform(10.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_candidate(&fixture, near, &candidate));
    CHECK(hth_enemy_perception_can_perceive(
        fixture.entities, fixture.actors, fixture.enemies,
        fixture.spatial, enemy, candidate, 3.0F));
    CHECK(!has_los(&fixture, &wall, enemy, candidate));
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities,
                                candidate, &far));
    CHECK(!hth_enemy_perception_can_perceive(
        fixture.entities, fixture.actors, fixture.enemies,
        fixture.spatial, enemy, candidate, 3.0F));
    CHECK(has_los(&fixture, &clear, enemy, candidate));
    fixture_destroy(&fixture);
    return true;
}

static bool test_repeated_purity_and_independent_worlds(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHCollisionWorld blocked = world_with_box(
        (HTHAABB){{1.0F, -1.0F, -1.0F}, {2.0F, 1.0F, 1.0F}});
    HTHCollisionWorld before = blocked;
    HTHSpatialTransform observer_before;
    HTHSpatialTransform candidate_before;
    HTHSpatialTransform observer_after;
    HTHSpatialTransform candidate_after;
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_candidate(&fixture, transform(3.0F, 0.0F, 0.0F, 0.0F),
                           &candidate));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &observer_before));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, candidate,
                                &candidate_before));
    for (index = 0U; index < 10U; ++index) {
        CHECK(has_los(&fixture, &clear, enemy, candidate));
        CHECK(!has_los(&fixture, &blocked, enemy, candidate));
    }
    CHECK(memcmp(&blocked, &before, sizeof(blocked)) == 0);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &observer_after));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, candidate,
                                &candidate_after));
    CHECK(memcmp(&observer_before, &observer_after,
                 sizeof(observer_before)) == 0);
    CHECK(memcmp(&candidate_before, &candidate_after,
                 sizeof(candidate_before)) == 0);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_null_and_trace_failure_contracts,
        test_clear_blocked_and_finite_segment_semantics,
        test_zero_length_self_and_non_actor_candidate,
        test_semantic_requirements_remove_and_reattach,
        test_dead_and_generation_reuse,
        test_candidate_actor_health_body_and_yaw_independence,
        test_target_preservation_and_candidate_independence,
        test_perception_orthogonality,
        test_repeated_purity_and_independent_worlds
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy LOS tests passed");
    return EXIT_SUCCESS;
}
