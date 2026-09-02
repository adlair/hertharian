#include "enemy_seek.h"

#include "dynamic_body.h"
#include "enemy_decision.h"
#include "enemy_target_selection.h"
#include "health.h"

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
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_health_store_destroy(fixture->health);
    hth_spatial_store_destroy(fixture->spatial);
    hth_enemy_target_store_destroy(fixture->targets);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static HTHSpatialTransform transform(float x, float y, float z, float yaw)
{
    HTHSpatialTransform value = {{x, y, z}, yaw};

    return value;
}

static bool create_entity(Fixture *fixture, HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity);
}

static bool create_spatial(Fixture *fixture, HTHSpatialTransform value,
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

static bool seek(Fixture *fixture, HTHEntityHandle enemy,
                 HTHEntityHandle target, HTHVec3 *out_direction)
{
    return hth_enemy_seek_compute(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, enemy, target, out_direction);
}

static bool vector_equal(HTHVec3 left, HTHVec3 right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

static bool vector_near(HTHVec3 actual, HTHVec3 expected)
{
    const float tolerance = 0.00001F;

    return fabsf(actual.x - expected.x) <= tolerance &&
           fabsf(actual.y - expected.y) <= tolerance &&
           fabsf(actual.z - expected.z) <= tolerance;
}

static bool vector_is_zero(HTHVec3 vector)
{
    const HTHVec3 zero = {0.0F, 0.0F, 0.0F};

    return vector_equal(vector, zero);
}

static bool test_arguments_and_canonical_output(void)
{
    Fixture fixture;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHVec3 direction;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial(&fixture, transform(1.0F, 0.0F, 0.0F, 0.0F),
                         &target));

#define CHECK_FAILURE(arguments)                                             \
    do {                                                                     \
        direction = (HTHVec3){99.0F, 99.0F, 99.0F};                          \
        CHECK(!hth_enemy_seek_compute arguments);                            \
        CHECK(vector_is_zero(direction));                                    \
    } while (0)

    CHECK_FAILURE((NULL, fixture.actors, fixture.enemies, fixture.spatial,
                   enemy, target, &direction));
    CHECK_FAILURE((fixture.entities, NULL, fixture.enemies, fixture.spatial,
                   enemy, target, &direction));
    CHECK_FAILURE((fixture.entities, fixture.actors, NULL, fixture.spatial,
                   enemy, target, &direction));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies, NULL,
                   enemy, target, &direction));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                   fixture.spatial, hth_entity_handle_invalid(), target,
                   &direction));
    CHECK_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                   fixture.spatial, enemy, hth_entity_handle_invalid(),
                   &direction));
#undef CHECK_FAILURE

    CHECK(!seek(&fixture, enemy, target, NULL));
    fixture_destroy(&fixture);
    return true;
}

static bool test_required_associations(void)
{
    Fixture fixture;
    HTHEntityHandle entity_only;
    HTHEntityHandle actor_only;
    HTHEntityHandle enemy_without_spatial;
    HTHEntityHandle valid_enemy;
    HTHEntityHandle target_without_spatial;
    HTHEntityHandle target;
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHVec3 direction;

    CHECK(fixture_create(&fixture));
    CHECK(create_spatial(&fixture, transform(1.0F, 0.0F, 0.0F, 0.0F),
                         &target));
    CHECK(create_entity(&fixture, &entity_only));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   entity_only, &origin));
    CHECK(!seek(&fixture, entity_only, target, &direction));
    CHECK(vector_is_zero(direction));

    CHECK(create_entity(&fixture, &actor_only));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 actor_only));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   actor_only, &origin));
    CHECK(!seek(&fixture, actor_only, target, &direction));

    CHECK(create_entity(&fixture, &enemy_without_spatial));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 enemy_without_spatial));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy_without_spatial));
    CHECK(!seek(&fixture, enemy_without_spatial, target, &direction));

    CHECK(create_enemy(&fixture, origin, &valid_enemy));
    CHECK(create_entity(&fixture, &target_without_spatial));
    CHECK(!seek(&fixture, valid_enemy, target_without_spatial, &direction));
    CHECK(vector_is_zero(direction));
    fixture_destroy(&fixture);
    return true;
}

static bool test_remove_and_reattach(void)
{
    Fixture fixture;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHSpatialTransform enemy_value = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform target_value = transform(2.0F, 0.0F, 0.0F, 0.0F);
    HTHVec3 direction;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, enemy_value, &enemy));
    CHECK(create_spatial(&fixture, target_value, &target));

    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, enemy));
    CHECK(!seek(&fixture, enemy, target, &direction));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, enemy));
    CHECK(seek(&fixture, enemy, target, &direction));

    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, enemy));
    CHECK(!seek(&fixture, enemy, target, &direction));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(seek(&fixture, enemy, target, &direction));

    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, enemy));
    CHECK(!seek(&fixture, enemy, target, &direction));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities, enemy,
                                   &enemy_value));
    CHECK(seek(&fixture, enemy, target, &direction));

    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, target));
    CHECK(!seek(&fixture, enemy, target, &direction));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities, target,
                                   &target_value));
    CHECK(seek(&fixture, enemy, target, &direction));
    CHECK(vector_equal(direction, (HTHVec3){1.0F, 0.0F, 0.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_generation_safety(void)
{
    Fixture fixture;
    HTHEntityHandle stale_enemy;
    HTHEntityHandle replacement_enemy;
    HTHEntityHandle stale_target;
    HTHEntityHandle replacement_target;
    HTHVec3 direction;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &stale_enemy));
    CHECK(create_spatial(&fixture, transform(1.0F, 0.0F, 0.0F, 0.0F),
                         &stale_target));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale_enemy));
    CHECK(create_enemy(&fixture, transform(0.0F, 1.0F, 0.0F, 0.0F),
                       &replacement_enemy));
    CHECK(replacement_enemy.index == stale_enemy.index);
    CHECK(replacement_enemy.generation != stale_enemy.generation);
    CHECK(!seek(&fixture, stale_enemy, stale_target, &direction));
    CHECK(vector_is_zero(direction));

    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale_target));
    CHECK(create_spatial(&fixture, transform(2.0F, 1.0F, 0.0F, 0.0F),
                         &replacement_target));
    CHECK(replacement_target.index == stale_target.index);
    CHECK(replacement_target.generation != stale_target.generation);
    CHECK(!seek(&fixture, replacement_enemy, stale_target, &direction));
    CHECK(vector_is_zero(direction));
    CHECK(seek(&fixture, replacement_enemy, replacement_target, &direction));
    CHECK(vector_equal(direction, (HTHVec3){1.0F, 0.0F, 0.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_zero_and_axis_directions(void)
{
    Fixture fixture;
    const HTHSpatialTransform positions[] = {
        {{10.0F, 0.0F, 0.0F}, 0.0F},
        {{-10.0F, 0.0F, 0.0F}, 0.0F},
        {{0.0F, 10.0F, 0.0F}, 0.0F},
        {{0.0F, -10.0F, 0.0F}, 0.0F},
        {{0.0F, 0.0F, 10.0F}, 0.0F},
        {{0.0F, 0.0F, -10.0F}, 0.0F}
    };
    const HTHVec3 expected[] = {
        {1.0F, 0.0F, 0.0F}, {-1.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F}, {0.0F, -1.0F, 0.0F},
        {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, -1.0F}
    };
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle colocated;
    HTHVec3 direction;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(seek(&fixture, enemy, enemy, &direction));
    CHECK(vector_is_zero(direction));
    CHECK(isfinite(direction.x) && isfinite(direction.y) &&
          isfinite(direction.z));
    CHECK(create_spatial(&fixture, transform(0.0F, 0.0F, 0.0F, 3.0F),
                         &colocated));
    CHECK(seek(&fixture, enemy, colocated, &direction));
    CHECK(vector_is_zero(direction));

    CHECK(create_spatial(&fixture, positions[0], &target));
    for (index = 0U; index < sizeof(positions) / sizeof(positions[0]);
         ++index) {
        CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities,
                                    target, &positions[index]));
        CHECK(seek(&fixture, enemy, target, &direction));
        CHECK(vector_equal(direction, expected[index]));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_diagonals_and_geometric_invariants(void)
{
    Fixture fixture;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle reverse_enemy;
    HTHSpatialTransform enemy_value;
    HTHSpatialTransform target_value;
    HTHVec3 base;
    HTHVec3 direction;
    HTHVec3 reverse;
    float length;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 1.0F),
                       &enemy));
    CHECK(create_spatial(&fixture, transform(3.0F, 4.0F, 0.0F, -1.0F),
                         &target));
    CHECK(seek(&fixture, enemy, target, &base));
    CHECK(vector_near(base, (HTHVec3){0.6F, 0.8F, 0.0F}));
    length = sqrtf(base.x * base.x + base.y * base.y + base.z * base.z);
    CHECK(fabsf(length - 1.0F) <= 0.00001F);

    target_value = transform(1.0F, 2.0F, 2.0F, 7.0F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &target_value));
    CHECK(seek(&fixture, enemy, target, &direction));
    CHECK(vector_near(direction,
                      (HTHVec3){1.0F / 3.0F, 2.0F / 3.0F,
                                2.0F / 3.0F}));

    enemy_value = transform(10.0F, -7.0F, 4.0F, 9.0F);
    target_value = transform(13.0F, -3.0F, 4.0F, -9.0F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, enemy,
                                &enemy_value));
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &target_value));
    CHECK(seek(&fixture, enemy, target, &direction));
    CHECK(vector_near(direction, base));

    target_value = transform(16.0F, 1.0F, 4.0F, 0.0F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &target_value));
    CHECK(seek(&fixture, enemy, target, &direction));
    CHECK(vector_near(direction, base));

    CHECK(create_enemy(&fixture, target_value, &reverse_enemy));
    CHECK(seek(&fixture, reverse_enemy, enemy, &reverse));
    CHECK(vector_near(direction,
                      (HTHVec3){-reverse.x, -reverse.y, -reverse.z}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_large_coordinates_and_dynamic_positions(void)
{
    Fixture fixture;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHSpatialTransform moved;
    HTHVec3 direction;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(-3.0E38F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial(&fixture,
                         transform(3.0E38F, 0.0F, 0.0F, 0.0F), &target));
    CHECK(seek(&fixture, enemy, target, &direction));
    CHECK(vector_equal(direction, (HTHVec3){1.0F, 0.0F, 0.0F}));
    CHECK(isfinite(direction.x) && isfinite(direction.y) &&
          isfinite(direction.z));

    moved = transform(-3.0E38F, 2.0F, 0.0F, 0.0F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, target,
                                &moved));
    CHECK(seek(&fixture, enemy, target, &direction));
    CHECK(vector_equal(direction, (HTHVec3){0.0F, 1.0F, 0.0F}));
    moved = transform(-3.0E38F, 2.0F, 2.0F, 0.0F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, enemy,
                                &moved));
    CHECK(seek(&fixture, enemy, target, &direction));
    CHECK(vector_equal(direction, (HTHVec3){0.0F, 0.0F, -1.0F}));
    fixture_destroy(&fixture);
    return true;
}

static bool test_role_independence_determinism_and_purity(void)
{
    Fixture fixture;
    const HTHHealth zero_health = {0.0F, 100.0F};
    const HTHDynamicBody body = {{0.5F, 0.5F, 1.0F}, {8.0F, 7.0F, 6.0F}};
    HTHDynamicBody body_after;
    HTHSpatialTransform enemy_before = transform(0.0F, 0.0F, 0.0F, -8.0F);
    HTHSpatialTransform target_before = transform(3.0F, 4.0F, 0.0F, 9.0F);
    HTHSpatialTransform after;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHVec3 expected = {0.6F, 0.8F, 0.0F};
    HTHVec3 direction;
    size_t live_count;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, enemy_before, &enemy));
    CHECK(create_spatial(&fixture, target_before, &target));
    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(seek(&fixture, enemy, target, &direction));
    CHECK(vector_near(direction, expected));

    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, target));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, target));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, enemy, zero_health));
    CHECK(hth_health_store_attach(fixture.health, fixture.entities,
                                  fixture.actors, target, zero_health));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, enemy, &body));
    CHECK(hth_dynamic_body_attach(fixture.bodies, fixture.entities,
                                  fixture.spatial, target, &body));
    live_count = hth_entity_registry_live_count(fixture.entities);
    for (index = 0U; index < 128U; ++index) {
        CHECK(seek(&fixture, enemy, target, &direction));
        CHECK(vector_near(direction, expected));
    }
    CHECK(hth_entity_registry_live_count(fixture.entities) == live_count);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, enemy,
                                &after));
    CHECK(memcmp(&after, &enemy_before, sizeof(after)) == 0);
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, target,
                                &after));
    CHECK(memcmp(&after, &target_before, sizeof(after)) == 0);
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, enemy,
                               &body_after));
    CHECK(memcmp(&body_after, &body, sizeof(body_after)) == 0);
    fixture_destroy(&fixture);
    return true;
}

static bool test_independent_store_sets(void)
{
    Fixture first;
    Fixture second;
    HTHEntityHandle first_enemy;
    HTHEntityHandle first_target;
    HTHEntityHandle second_enemy;
    HTHEntityHandle second_target;
    HTHVec3 direction;

    CHECK(fixture_create(&first));
    CHECK(fixture_create(&second));
    CHECK(create_enemy(&first, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &first_enemy));
    CHECK(create_spatial(&first, transform(1.0F, 0.0F, 0.0F, 0.0F),
                         &first_target));
    CHECK(create_enemy(&second, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &second_enemy));
    CHECK(create_spatial(&second, transform(0.0F, 1.0F, 0.0F, 0.0F),
                         &second_target));
    CHECK(hth_entity_handle_equal(first_enemy, second_enemy));
    CHECK(hth_entity_handle_equal(first_target, second_target));
    CHECK(seek(&first, first_enemy, first_target, &direction));
    CHECK(vector_equal(direction, (HTHVec3){1.0F, 0.0F, 0.0F}));
    CHECK(seek(&second, second_enemy, second_target, &direction));
    CHECK(vector_equal(direction, (HTHVec3){0.0F, 1.0F, 0.0F}));
    fixture_destroy(&second);
    fixture_destroy(&first);
    return true;
}

static bool test_decision_and_selection_composition(void)
{
    Fixture fixture;
    HTHCollisionWorld world = {0};
    HTHEntityHandle enemy;
    HTHEntityHandle candidate_a;
    HTHEntityHandle candidate_b;
    HTHEntityHandle candidates[2];
    HTHEntityHandle selected;
    HTHEntityHandle current;
    HTHEnemyIntent intent;
    HTHVec3 direction;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, transform(0.0F, 0.0F, 0.0F, 0.0F),
                       &enemy));
    CHECK(create_spatial(&fixture, transform(3.0F, 4.0F, 0.0F, 0.0F),
                         &candidate_a));
    CHECK(create_spatial(&fixture, transform(0.0F, 9.0F, 0.0F, 0.0F),
                         &candidate_b));
    CHECK(hth_enemy_target_store_set(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, candidate_a));
    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &world, enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_PURSUE);
    CHECK(seek(&fixture, enemy, intent.target, &direction));
    CHECK(vector_near(direction, (HTHVec3){0.6F, 0.8F, 0.0F}));

    candidates[0] = candidate_b;
    candidates[1] = candidate_a;
    CHECK(hth_enemy_target_select(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &world, fixture.targets, enemy, candidates, 2U, 10.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, candidate_a));
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        enemy, &current));
    CHECK(hth_enemy_decision_evaluate(
        fixture.entities, fixture.actors, fixture.enemies, fixture.targets,
        fixture.spatial, &world, enemy, 10.0F, &intent));
    CHECK(intent.kind == HTH_ENEMY_INTENT_PURSUE);
    CHECK(hth_entity_handle_equal(intent.target, current));
    CHECK(seek(&fixture, enemy, current, &direction));
    CHECK(vector_near(direction, (HTHVec3){0.6F, 0.8F, 0.0F}));
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_arguments_and_canonical_output,
        test_required_associations,
        test_remove_and_reattach,
        test_generation_safety,
        test_zero_and_axis_directions,
        test_diagonals_and_geometric_invariants,
        test_large_coordinates_and_dynamic_positions,
        test_role_independence_determinism_and_purity,
        test_independent_store_sets,
        test_decision_and_selection_composition
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy seek tests passed");
    return EXIT_SUCCESS;
}
