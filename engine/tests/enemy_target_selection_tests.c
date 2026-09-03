#include "enemy_target_selection.h"

#include "dynamic_body.h"
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
    HTHActorStore *actors;
    HTHEnemyStore *enemies;
    HTHEnemyTargetStore *targets;
    HTHSpatialStore *spatial;
    HTHHealthStore *health;
    HTHDynamicBodyStore *bodies;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->enemies = hth_enemy_store_create();
    fixture->targets = hth_enemy_target_store_create();
    fixture->spatial = hth_spatial_store_create();
    fixture->health = hth_health_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->targets != NULL &&
           fixture->spatial != NULL && fixture->health != NULL &&
           fixture->bodies != NULL;
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

static HTHSpatialTransform transform(float x, float y, float z)
{
    HTHSpatialTransform value = {{x, y, z}, 0.0F};

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

static bool create_candidate(Fixture *fixture, HTHSpatialTransform value,
                             HTHEntityHandle *out_candidate)
{
    return create_entity(fixture, out_candidate) &&
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_candidate, &value);
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

static bool select_target(Fixture *fixture,
                          const HTHCollisionWorld *world,
                          HTHEntityHandle enemy,
                          const HTHEntityHandle *candidates,
                          size_t candidate_count,
                          float radius,
                          HTHEntityHandle *out_selected)
{
    return hth_enemy_target_select(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, world, fixture->targets, enemy, candidates,
        candidate_count, hth_entity_handle_invalid(), radius, out_selected);
}

static bool select_target_excluding(
    Fixture *fixture, const HTHCollisionWorld *world,
    HTHEntityHandle enemy, const HTHEntityHandle *candidates,
    size_t candidate_count, HTHEntityHandle excluded_target, float radius,
    HTHEntityHandle *out_selected)
{
    return hth_enemy_target_select(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, world, fixture->targets, enemy, candidates,
        candidate_count, excluded_target, radius, out_selected);
}

static bool set_target(Fixture *fixture, HTHEntityHandle enemy,
                       HTHEntityHandle target)
{
    return hth_enemy_target_store_set(
        fixture->targets, fixture->entities, fixture->actors,
        fixture->enemies, enemy, target);
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

static bool handle_is_invalid(HTHEntityHandle handle)
{
    return hth_entity_handle_equal(handle, hth_entity_handle_invalid());
}

static bool test_structural_contracts_and_empty_set(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle current;
    HTHEntityHandle selected = {7U, 9U};

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F), &enemy));
    CHECK(create_candidate(&fixture, transform(1.0F, 0.0F, 0.0F),
                           &current));
    CHECK(set_target(&fixture, enemy, current));
    CHECK(select_target(&fixture, &clear, enemy, NULL, 0U, 2.0F,
                        &selected));
    CHECK(handle_is_invalid(selected));
    CHECK(target_equals(&fixture, enemy, current));

    selected = current;
    CHECK(!select_target(&fixture, &clear, enemy, NULL, 1U, 2.0F,
                         &selected));
    CHECK(handle_is_invalid(selected));
    CHECK(target_equals(&fixture, enemy, current));
    CHECK(!select_target(&fixture, &clear, enemy, &current, 1U, 2.0F,
                         NULL));
    CHECK(target_equals(&fixture, enemy, current));

#define CHECK_NULL_FAILURE(arguments)                                       \
    do {                                                                     \
        selected = current;                                                  \
        CHECK(!hth_enemy_target_select arguments);                           \
        CHECK(handle_is_invalid(selected));                                  \
        CHECK(target_equals(&fixture, enemy, current));                       \
    } while (0)

    CHECK_NULL_FAILURE((NULL, fixture.actors, fixture.enemies,
                        fixture.spatial, &clear, fixture.targets, enemy,
                        NULL, 0U, hth_entity_handle_invalid(), 2.0F,
                        &selected));
    CHECK_NULL_FAILURE((fixture.entities, NULL, fixture.enemies,
                        fixture.spatial, &clear, fixture.targets, enemy,
                        NULL, 0U, hth_entity_handle_invalid(), 2.0F,
                        &selected));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, NULL,
                        fixture.spatial, &clear, fixture.targets, enemy,
                        NULL, 0U, hth_entity_handle_invalid(), 2.0F,
                        &selected));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                        NULL, &clear, fixture.targets, enemy, NULL, 0U,
                        hth_entity_handle_invalid(), 2.0F, &selected));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                        fixture.spatial, NULL, fixture.targets, enemy, NULL,
                        0U, hth_entity_handle_invalid(), 2.0F, &selected));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                        fixture.spatial, &clear, NULL, enemy, NULL, 0U,
                        hth_entity_handle_invalid(), 2.0F, &selected));
#undef CHECK_NULL_FAILURE

    fixture_destroy(&fixture);
    return true;
}

static bool test_invalid_radius_transactionality(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle current;
    HTHEntityHandle selected;
    const float invalid_radii[] = {-1.0F, NAN, INFINITY, -INFINITY};
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F), &enemy));
    CHECK(create_candidate(&fixture, transform(1.0F, 0.0F, 0.0F),
                           &current));
    CHECK(set_target(&fixture, enemy, current));
    for (index = 0U; index < sizeof(invalid_radii) /
                                  sizeof(invalid_radii[0]); ++index) {
        selected = current;
        CHECK(!select_target(&fixture, &clear, enemy, NULL, 0U,
                             invalid_radii[index], &selected));
        CHECK(handle_is_invalid(selected));
        CHECK(target_equals(&fixture, enemy, current));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_observer_requirements_and_generation(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F);
    HTHEntityHandle invalid = hth_entity_handle_invalid();
    HTHEntityHandle candidate;
    HTHEntityHandle actor_only;
    HTHEntityHandle no_spatial;
    HTHEntityHandle old_enemy;
    HTHEntityHandle new_enemy;
    HTHEntityHandle selected = {1U, 1U};

    CHECK(fixture_create(&fixture));
    CHECK(create_candidate(&fixture, transform(1.0F, 0.0F, 0.0F),
                           &candidate));
    CHECK(!select_target(&fixture, &clear, invalid, &candidate, 1U, 2.0F,
                         &selected));
    CHECK(handle_is_invalid(selected));
    CHECK(create_entity(&fixture, &actor_only));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 actor_only));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   actor_only, &origin));
    CHECK(!select_target(&fixture, &clear, actor_only, &candidate, 1U, 2.0F,
                         &selected));
    CHECK(create_entity(&fixture, &no_spatial));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 no_spatial));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, no_spatial));
    CHECK(!select_target(&fixture, &clear, no_spatial, &candidate, 1U, 2.0F,
                         &selected));
    CHECK(create_enemy(&fixture, origin, &old_enemy));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, old_enemy));
    CHECK(!select_target(&fixture, &clear, old_enemy, &candidate, 1U, 2.0F,
                         &selected));
    CHECK(create_enemy(&fixture, origin, &new_enemy));
    CHECK(new_enemy.index == old_enemy.index);
    CHECK(new_enemy.generation != old_enemy.generation);
    CHECK(!select_target(&fixture, &clear, old_enemy, &candidate, 1U, 2.0F,
                         &selected));
    CHECK(select_target(&fixture, &clear, new_enemy, &candidate, 1U, 2.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, candidate));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   new_enemy));
    CHECK(!select_target(&fixture, &clear, new_enemy, &candidate, 1U, 2.0F,
                         &selected));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   new_enemy, &origin));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities,
                                 new_enemy));
    CHECK(!select_target(&fixture, &clear, new_enemy, &candidate, 1U, 2.0F,
                         &selected));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, new_enemy));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 new_enemy));
    CHECK(!select_target(&fixture, &clear, new_enemy, &candidate, 1U, 2.0F,
                         &selected));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 new_enemy));
    CHECK(select_target(&fixture, &clear, new_enemy, &candidate, 1U, 2.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, candidate));
    fixture_destroy(&fixture);
    return true;
}

static bool test_invalid_candidates_self_and_generation_reuse(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle current;
    HTHEntityHandle no_spatial;
    HTHEntityHandle old_candidate;
    HTHEntityHandle new_candidate;
    HTHEntityHandle other;
    HTHEntityHandle selected = {8U, 8U};
    HTHEntityHandle skipped[4];
    HTHEntityHandle mixed[3];

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F), &enemy));
    CHECK(create_candidate(&fixture, transform(4.0F, 0.0F, 0.0F),
                           &current));
    CHECK(set_target(&fixture, enemy, current));
    CHECK(create_entity(&fixture, &no_spatial));
    CHECK(create_candidate(&fixture, transform(2.0F, 0.0F, 0.0F),
                           &old_candidate));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities,
                                              old_candidate));
    skipped[0] = hth_entity_handle_invalid();
    skipped[1] = no_spatial;
    skipped[2] = old_candidate;
    skipped[3] = enemy;
    CHECK(select_target(&fixture, &clear, enemy, skipped, 4U, 10.0F,
                        &selected));
    CHECK(handle_is_invalid(selected));
    CHECK(target_equals(&fixture, enemy, current));

    CHECK(create_candidate(&fixture, transform(2.0F, 0.0F, 0.0F),
                           &new_candidate));
    CHECK(new_candidate.index == old_candidate.index);
    CHECK(new_candidate.generation != old_candidate.generation);
    CHECK(create_candidate(&fixture, transform(3.0F, 0.0F, 0.0F), &other));
    mixed[0] = enemy;
    mixed[1] = old_candidate;
    mixed[2] = new_candidate;
    CHECK(select_target(&fixture, &clear, enemy, mixed, 3U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, new_candidate));
    CHECK(target_equals(&fixture, enemy, new_candidate));
    fixture_destroy(&fixture);
    return true;
}

static bool test_duplicates_nearest_and_order_independence(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle far;
    HTHEntityHandle near;
    HTHEntityHandle middle;
    HTHEntityHandle selected;
    HTHEntityHandle ordered[5];
    HTHEntityHandle reversed[5];
    HTHEntityHandle duplicates[3];
    HTHEntityHandle snapshot[5];

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F), &enemy));
    CHECK(create_candidate(&fixture, transform(5.0F, 0.0F, 0.0F), &far));
    CHECK(create_candidate(&fixture, transform(1.0F, 1.0F, 1.0F), &near));
    CHECK(create_candidate(&fixture, transform(0.0F, 0.0F, 3.0F), &middle));
    ordered[0] = far;
    ordered[1] = near;
    ordered[2] = middle;
    ordered[3] = far;
    ordered[4] = near;
    reversed[0] = near;
    reversed[1] = far;
    reversed[2] = middle;
    reversed[3] = near;
    reversed[4] = far;
    memcpy(snapshot, ordered, sizeof(ordered));
    CHECK(select_target(&fixture, &clear, enemy, ordered, 5U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, near));
    CHECK(memcmp(snapshot, ordered, sizeof(ordered)) == 0);
    CHECK(select_target(&fixture, &clear, enemy, reversed, 5U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, near));
    duplicates[0] = middle;
    duplicates[1] = middle;
    duplicates[2] = middle;
    CHECK(select_target(&fixture, &clear, enemy, duplicates, 3U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, middle));
    fixture_destroy(&fixture);
    return true;
}

static bool test_exact_tie_break_and_three_dimensions(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle lower;
    HTHEntityHandle higher;
    HTHEntityHandle vertical_far;
    HTHEntityHandle spatial_near;
    HTHEntityHandle selected;
    HTHEntityHandle tied[2];
    HTHEntityHandle three_d[2];

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F), &enemy));
    CHECK(create_candidate(&fixture, transform(3.0F, 4.0F, 0.0F), &lower));
    CHECK(create_candidate(&fixture, transform(-3.0F, -4.0F, 0.0F),
                           &higher));
    CHECK(lower.index < higher.index);
    tied[0] = higher;
    tied[1] = lower;
    CHECK(select_target(&fixture, &clear, enemy, tied, 2U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, lower));

    CHECK(create_candidate(&fixture, transform(3.0F, 4.0F, 0.0F),
                           &vertical_far));
    CHECK(create_candidate(&fixture, transform(4.0F, 0.0F, 0.0F),
                           &spatial_near));
    three_d[0] = vertical_far;
    three_d[1] = spatial_near;
    CHECK(select_target(&fixture, &clear, enemy, three_d, 2U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, spatial_near));
    fixture_destroy(&fixture);
    return true;
}

static bool test_perception_los_and_zero_radius(void)
{
    Fixture fixture;
    HTHCollisionWorld wall = world_with_box(
        (HTHAABB){{0.75F, -0.25F, -0.25F}, {1.25F, 0.25F, 0.25F}});
    HTHCollisionWorld clear = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle blocked_near;
    HTHEntityHandle visible_far;
    HTHEntityHandle outside;
    HTHEntityHandle colocated;
    HTHEntityHandle selected;
    HTHEntityHandle candidates[2];

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F), &enemy));
    CHECK(create_candidate(&fixture, transform(2.0F, 0.0F, 0.0F),
                           &blocked_near));
    CHECK(create_candidate(&fixture, transform(0.0F, 4.0F, 0.0F),
                           &visible_far));
    CHECK(create_candidate(&fixture, transform(8.0F, 0.0F, 0.0F), &outside));
    candidates[0] = blocked_near;
    candidates[1] = visible_far;
    CHECK(select_target(&fixture, &wall, enemy, candidates, 2U, 5.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, visible_far));
    CHECK(select_target(&fixture, &clear, enemy, &outside, 1U, 5.0F,
                        &selected));
    CHECK(handle_is_invalid(selected));
    CHECK(create_candidate(&fixture, transform(0.0F, 0.0F, 0.0F),
                           &colocated));
    CHECK(select_target(&fixture, &wall, enemy, &colocated, 1U, 0.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, colocated));
    CHECK(select_target(&fixture, &clear, enemy, &blocked_near, 1U, 0.0F,
                        &selected));
    CHECK(handle_is_invalid(selected));
    fixture_destroy(&fixture);
    return true;
}

static bool test_candidate_roles_health_and_body_independence(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHHealth zero_health = {0.0F, 100.0F};
    HTHDynamicBody body = {{0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 0.0F}};
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;
    HTHEntityHandle other_enemy;
    HTHEntityHandle selected;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F), &enemy));
    CHECK(create_candidate(&fixture, transform(1.0F, 0.0F, 0.0F),
                           &candidate));
    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities, candidate));
    CHECK(select_target(&fixture, &clear, enemy, &candidate, 1U, 5.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, candidate));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, candidate));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, candidate, zero_health));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, enemy, zero_health));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, candidate, &body));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, enemy, &body));
    CHECK(select_target(&fixture, &clear, enemy, &candidate, 1U, 5.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, candidate));
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities,
                                  candidate));
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities, enemy));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, candidate));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, candidate));
    CHECK(select_target(&fixture, &clear, enemy, &candidate, 1U, 5.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, candidate));
    CHECK(create_enemy(&fixture, transform(2.0F, 0.0F, 0.0F), &other_enemy));
    CHECK(select_target(&fixture, &clear, enemy, &other_enemy, 1U, 5.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, other_enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_current_target_policies(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHCollisionWorld wall = world_with_box(
        (HTHAABB){{1.0F, -1.0F, -1.0F}, {2.0F, 1.0F, 1.0F}});
    HTHEntityHandle enemy;
    HTHEntityHandle current;
    HTHEntityHandle better;
    HTHEntityHandle outside;
    HTHEntityHandle selected;
    HTHEntityHandle candidates[2];

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F), &enemy));
    CHECK(create_candidate(&fixture, transform(5.0F, 0.0F, 0.0F),
                           &current));
    CHECK(create_candidate(&fixture, transform(2.0F, 3.0F, 0.0F), &better));
    CHECK(create_candidate(&fixture, transform(20.0F, 0.0F, 0.0F),
                           &outside));
    CHECK(set_target(&fixture, enemy, current));
    CHECK(select_target(&fixture, &clear, enemy, &better, 1U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, better));
    CHECK(target_equals(&fixture, enemy, better));
    CHECK(set_target(&fixture, enemy, current));
    candidates[0] = current;
    candidates[1] = better;
    CHECK(select_target(&fixture, &clear, enemy, candidates, 2U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, better));
    CHECK(target_equals(&fixture, enemy, better));
    CHECK(select_target(&fixture, &clear, enemy, candidates, 2U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, better));
    CHECK(target_equals(&fixture, enemy, better));
    CHECK(select_target(&fixture, &clear, enemy, &current, 1U, 10.0F,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, current));
    CHECK(target_equals(&fixture, enemy, current));
    CHECK(select_target(&fixture, &clear, enemy, &outside, 1U, 10.0F,
                        &selected));
    CHECK(handle_is_invalid(selected));
    CHECK(target_equals(&fixture, enemy, current));
    CHECK(select_target(&fixture, &wall, enemy, &current, 1U, 10.0F,
                        &selected));
    CHECK(handle_is_invalid(selected));
    CHECK(target_equals(&fixture, enemy, current));
    fixture_destroy(&fixture);
    return true;
}

static bool test_large_coordinates_and_independent_store_sets(void)
{
    Fixture first;
    Fixture second;
    HTHCollisionWorld clear = empty_world();
    const float large = FLT_MAX * 0.75F;
    HTHEntityHandle enemy_a;
    HTHEntityHandle farther_a;
    HTHEntityHandle nearer_a;
    HTHEntityHandle enemy_b;
    HTHEntityHandle target_b;
    HTHEntityHandle selected;
    HTHEntityHandle candidates[2];

    CHECK(fixture_create(&first));
    CHECK(fixture_create(&second));
    CHECK(create_enemy(&first, transform(large, large, large), &enemy_a));
    CHECK(create_candidate(&first,
                           transform(-FLT_MAX * 0.20F, large, large),
                           &farther_a));
    CHECK(create_candidate(&first,
                           transform(-FLT_MAX * 0.10F, large, large),
                           &nearer_a));
    CHECK(create_enemy(&second, transform(0.0F, 0.0F, 0.0F), &enemy_b));
    CHECK(create_candidate(&second, transform(1.0F, 0.0F, 0.0F),
                           &target_b));
    CHECK(set_target(&second, enemy_b, target_b));
    candidates[0] = farther_a;
    candidates[1] = nearer_a;
    CHECK(select_target(&first, &clear, enemy_a, candidates, 2U, FLT_MAX,
                        &selected));
    CHECK(hth_entity_handle_equal(selected, nearer_a));
    CHECK(target_equals(&first, enemy_a, nearer_a));
    CHECK(target_equals(&second, enemy_b, target_b));
    CHECK(target_equals(&first, enemy_a, nearer_a));
    fixture_destroy(&second);
    fixture_destroy(&first);
    return true;
}

static bool test_excluded_target_policy(void)
{
    Fixture fixture;
    HTHCollisionWorld clear = empty_world();
    HTHEntityHandle enemy;
    HTHEntityHandle excluded;
    HTHEntityHandle alternative;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHEntityHandle selected;
    HTHEntityHandle candidates[3];

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F), &enemy));
    CHECK(create_candidate(&fixture, transform(1.0F, 0.0F, 0.0F),
                           &excluded));
    CHECK(create_candidate(&fixture, transform(2.0F, 0.0F, 0.0F),
                           &alternative));
    candidates[0] = excluded;
    candidates[1] = excluded;
    CHECK(select_target_excluding(&fixture, &clear, enemy, candidates, 2U,
                                  excluded, 10.0F, &selected));
    CHECK(handle_is_invalid(selected));

    CHECK(select_target_excluding(
        &fixture, &clear, enemy, candidates, 2U,
        hth_entity_handle_invalid(), 10.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, excluded));
    CHECK(hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                       enemy));
    candidates[2] = alternative;
    CHECK(select_target_excluding(&fixture, &clear, enemy, candidates, 3U,
                                  excluded, 10.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, alternative));

    CHECK(create_candidate(&fixture, transform(3.0F, 0.0F, 0.0F), &stale));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(create_candidate(&fixture, transform(0.5F, 0.0F, 0.0F),
                           &replacement));
    CHECK(replacement.index == stale.index &&
          replacement.generation != stale.generation);
    CHECK(hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                       enemy));
    CHECK(select_target_excluding(&fixture, &clear, enemy, &replacement, 1U,
                                  stale, 10.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, replacement));

    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_structural_contracts_and_empty_set,
        test_invalid_radius_transactionality,
        test_observer_requirements_and_generation,
        test_invalid_candidates_self_and_generation_reuse,
        test_duplicates_nearest_and_order_independence,
        test_exact_tie_break_and_three_dimensions,
        test_perception_los_and_zero_radius,
        test_candidate_roles_health_and_body_independence,
        test_current_target_policies,
        test_large_coordinates_and_independent_store_sets,
        test_excluded_target_policy
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy target selection tests passed");
    return EXIT_SUCCESS;
}
