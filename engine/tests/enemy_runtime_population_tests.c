#include "enemy_runtime_population.h"

#include "actor_spawn.h"
#include "enemy_pursuit_runtime.h"
#include "enemy_target_selection.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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

static HTHEnemyRuntimeSpawnSpec spawn_spec(float offset)
{
    HTHEnemyRuntimeSpawnSpec spec = {
        {{1.0F + offset, 2.0F + offset, 3.0F + offset},
         0.25F + offset},
        {{0.5F + offset, 0.75F + offset, 1.0F + offset},
         {4.0F + offset, 5.0F + offset, 6.0F + offset}},
        {75.0F + offset, 100.0F + offset}
    };

    return spec;
}

static bool vec_equal(HTHVec3 left, HTHVec3 right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

static bool handle_invalid(HTHEntityHandle handle)
{
    return hth_entity_handle_equal(handle, hth_entity_handle_invalid());
}

static bool spawn_enemy(Fixture *fixture,
                        const HTHEnemyRuntimeSpawnSpec *spec,
                        HTHEntityHandle *out_enemy)
{
    return hth_enemy_runtime_spawn(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->cadences,
        fixture->spatial, fixture->bodies, fixture->health, spec, out_enemy);
}

static bool despawn_enemy(Fixture *fixture, HTHEntityHandle enemy)
{
    return hth_enemy_runtime_despawn(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->cadences,
        fixture->spatial, fixture->bodies, fixture->health, fixture->targets,
        enemy);
}

static bool runtime_matches(Fixture *fixture, HTHEntityHandle enemy,
                            const HTHEnemyRuntimeSpawnSpec *spec)
{
    HTHSpatialTransform transform;
    HTHDynamicBody body;
    HTHHealth health;
    HTHEntityHandle target;
    HTHEnemyAttackCadence *cadence;
    bool ready = false;

    return hth_entity_registry_is_alive(fixture->entities, enemy) &&
           hth_actor_store_has(fixture->actors, fixture->entities, enemy) &&
           hth_enemy_store_has(fixture->enemies, fixture->entities,
                               fixture->actors, enemy) &&
           hth_enemy_attack_cadence_store_has(
               fixture->cadences, fixture->entities, fixture->actors,
               fixture->enemies, enemy) &&
           hth_enemy_attack_cadence_store_get_mutable(
               fixture->cadences, fixture->entities, fixture->actors,
               fixture->enemies, enemy, &cadence) &&
           hth_enemy_attack_cadence_is_ready(cadence, &ready) && ready &&
           hth_spatial_store_get(fixture->spatial, fixture->entities, enemy,
                                 &transform) &&
           vec_equal(transform.position, spec->transform.position) &&
           transform.yaw == spec->transform.yaw &&
           hth_dynamic_body_get(fixture->bodies, fixture->entities, enemy,
                                &body) &&
           vec_equal(body.half_extents, spec->body.half_extents) &&
           vec_equal(body.velocity, spec->body.velocity) &&
           hth_health_store_get(fixture->health, fixture->entities,
                                fixture->actors, enemy, &health) &&
           health.current == spec->health.current &&
           health.maximum == spec->health.maximum &&
           !hth_enemy_target_store_get(
               fixture->targets, fixture->entities, fixture->actors,
               fixture->enemies, enemy, &target);
}

static bool composition_absent(const Fixture *fixture,
                               HTHEntityHandle enemy)
{
    return !hth_entity_registry_is_alive(fixture->entities, enemy) &&
           !hth_actor_store_has(fixture->actors, fixture->entities, enemy) &&
           !hth_enemy_store_has(fixture->enemies, fixture->entities,
                                fixture->actors, enemy) &&
           !hth_enemy_attack_cadence_store_has(
               fixture->cadences, fixture->entities, fixture->actors,
               fixture->enemies, enemy) &&
           !hth_spatial_store_has(fixture->spatial, fixture->entities,
                                  enemy) &&
           !hth_dynamic_body_has(fixture->bodies, fixture->entities, enemy) &&
           !hth_health_store_has(fixture->health, fixture->entities,
                                 fixture->actors, enemy);
}

static bool expect_invalid_spec(Fixture *fixture,
                                const HTHEnemyRuntimeSpawnSpec *spec,
                                HTHEntityHandle sentinel,
                                const HTHEnemyRuntimeSpawnSpec *sentinel_spec)
{
    const size_t live_before =
        hth_entity_registry_live_count(fixture->entities);
    HTHEntityHandle output = {7U, 9U};

    return !spawn_enemy(fixture, spec, &output) && handle_invalid(output) &&
           hth_entity_registry_live_count(fixture->entities) == live_before &&
           runtime_matches(fixture, sentinel, sentinel_spec);
}

static bool test_spawn_nulls_and_validation(void)
{
    Fixture fixture;
    HTHEnemyRuntimeSpawnSpec valid = spawn_spec(0.0F);
    HTHEntityHandle sentinel;
    HTHEntityHandle output = {7U, 9U};
    size_t live_before;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_enemy(&fixture, &valid, &sentinel));
    live_before = hth_entity_registry_live_count(fixture.entities);

#define CHECK_NULL_FAILURE(arguments)                                        \
    do {                                                                     \
        output = (HTHEntityHandle){7U, 9U};                                 \
        CHECK(!hth_enemy_runtime_spawn arguments);                           \
        CHECK(handle_invalid(output));                                       \
        CHECK(hth_entity_registry_live_count(fixture.entities) ==            \
              live_before);                                                  \
        CHECK(runtime_matches(&fixture, sentinel, &valid));                  \
    } while (0)

    CHECK_NULL_FAILURE((NULL, fixture.actors, fixture.enemies,
                        fixture.cadences,
                        fixture.spatial, fixture.bodies, fixture.health,
                        &valid, &output));
    CHECK_NULL_FAILURE((fixture.entities, NULL, fixture.enemies,
                        fixture.cadences,
                        fixture.spatial, fixture.bodies, fixture.health,
                        &valid, &output));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, NULL,
                        fixture.cadences,
                        fixture.spatial, fixture.bodies, fixture.health,
                        &valid, &output));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                        NULL, fixture.spatial, fixture.bodies, fixture.health,
                        &valid, &output));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                        fixture.cadences, NULL, fixture.bodies, fixture.health,
                        &valid, &output));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                        fixture.cadences, fixture.spatial, NULL,
                        fixture.health, &valid,
                        &output));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                        fixture.cadences, fixture.spatial, fixture.bodies,
                        NULL, &valid,
                        &output));
    CHECK_NULL_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                        fixture.cadences, fixture.spatial, fixture.bodies,
                        fixture.health, NULL, &output));
#undef CHECK_NULL_FAILURE

    CHECK(!hth_enemy_runtime_spawn(
        fixture.entities, fixture.actors, fixture.enemies, fixture.cadences,
        fixture.spatial, fixture.bodies, fixture.health, &valid, NULL));
    CHECK(hth_entity_registry_live_count(fixture.entities) == live_before);
    CHECK(runtime_matches(&fixture, sentinel, &valid));

    CHECK(despawn_enemy(&fixture, sentinel));
    fixture_destroy(&fixture);
    return true;
}

static bool test_spawn_payload_validation(void)
{
    Fixture fixture;
    HTHEnemyRuntimeSpawnSpec sentinel_spec = spawn_spec(0.0F);
    HTHEnemyRuntimeSpawnSpec invalid;
    HTHEntityHandle sentinel;
    const float nonfinite[] = {NAN, INFINITY, -INFINITY};
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_enemy(&fixture, &sentinel_spec, &sentinel));

    for (index = 0U; index < sizeof(nonfinite) / sizeof(nonfinite[0]);
         ++index) {
        invalid = spawn_spec(1.0F);
        invalid.transform.position.x = nonfinite[index];
        CHECK(expect_invalid_spec(&fixture, &invalid, sentinel,
                                  &sentinel_spec));
        invalid = spawn_spec(1.0F);
        invalid.transform.position.y = nonfinite[index];
        CHECK(expect_invalid_spec(&fixture, &invalid, sentinel,
                                  &sentinel_spec));
        invalid = spawn_spec(1.0F);
        invalid.transform.position.z = nonfinite[index];
        CHECK(expect_invalid_spec(&fixture, &invalid, sentinel,
                                  &sentinel_spec));
        invalid = spawn_spec(1.0F);
        invalid.transform.yaw = nonfinite[index];
        CHECK(expect_invalid_spec(&fixture, &invalid, sentinel,
                                  &sentinel_spec));
        invalid = spawn_spec(1.0F);
        invalid.body.half_extents.x = nonfinite[index];
        CHECK(expect_invalid_spec(&fixture, &invalid, sentinel,
                                  &sentinel_spec));
        invalid = spawn_spec(1.0F);
        invalid.body.velocity.z = nonfinite[index];
        CHECK(expect_invalid_spec(&fixture, &invalid, sentinel,
                                  &sentinel_spec));
    }

    invalid = spawn_spec(1.0F);
    invalid.body.half_extents.x = 0.0F;
    CHECK(expect_invalid_spec(&fixture, &invalid, sentinel, &sentinel_spec));
    invalid = spawn_spec(1.0F);
    invalid.body.half_extents.y = -1.0F;
    CHECK(expect_invalid_spec(&fixture, &invalid, sentinel, &sentinel_spec));
    invalid = spawn_spec(1.0F);
    invalid.body.half_extents.z = 0.0F;
    CHECK(expect_invalid_spec(&fixture, &invalid, sentinel, &sentinel_spec));

    invalid = spawn_spec(1.0F);
    invalid.health.maximum = 0.0F;
    CHECK(expect_invalid_spec(&fixture, &invalid, sentinel, &sentinel_spec));
    invalid = spawn_spec(1.0F);
    invalid.health.current = -1.0F;
    CHECK(expect_invalid_spec(&fixture, &invalid, sentinel, &sentinel_spec));
    invalid = spawn_spec(1.0F);
    invalid.health.current = invalid.health.maximum + 1.0F;
    CHECK(expect_invalid_spec(&fixture, &invalid, sentinel, &sentinel_spec));
    invalid = spawn_spec(1.0F);
    invalid.health.current = NAN;
    CHECK(expect_invalid_spec(&fixture, &invalid, sentinel, &sentinel_spec));
    invalid = spawn_spec(1.0F);
    invalid.health.maximum = INFINITY;
    CHECK(expect_invalid_spec(&fixture, &invalid, sentinel, &sentinel_spec));

    CHECK(despawn_enemy(&fixture, sentinel));
    fixture_destroy(&fixture);
    return true;
}

static bool test_exact_spawn_and_zero_health(void)
{
    Fixture fixture;
    HTHEnemyRuntimeSpawnSpec first_spec = spawn_spec(3.5F);
    HTHEnemyRuntimeSpawnSpec zero_health_spec = spawn_spec(0.0F);
    HTHEntityHandle first;
    HTHEntityHandle zero_health;

    zero_health_spec.health.current = 0.0F;
    CHECK(fixture_create(&fixture));
    CHECK(spawn_enemy(&fixture, &first_spec, &first));
    CHECK(runtime_matches(&fixture, first, &first_spec));
    CHECK(spawn_enemy(&fixture, &zero_health_spec, &zero_health));
    CHECK(runtime_matches(&fixture, zero_health, &zero_health_spec));
    CHECK(despawn_enemy(&fixture, first));
    CHECK(composition_absent(&fixture, first));
    CHECK(despawn_enemy(&fixture, zero_health));
    CHECK(composition_absent(&fixture, zero_health));
    fixture_destroy(&fixture);
    return true;
}

static bool test_growth_and_cleanup(void)
{
    enum { ENEMY_COUNT = 130 };
    Fixture fixture;
    HTHEnemyRuntimeSpawnSpec specs[ENEMY_COUNT];
    HTHEntityHandle enemies[ENEMY_COUNT];
    const size_t initial_live_count = 0U;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_live_count(fixture.entities) ==
          initial_live_count);
    for (index = 0U; index < ENEMY_COUNT; ++index) {
        specs[index] = spawn_spec((float)index * 0.01F);
        CHECK(spawn_enemy(&fixture, &specs[index], &enemies[index]));
        CHECK(runtime_matches(&fixture, enemies[index], &specs[index]));
    }
    CHECK(hth_entity_registry_live_count(fixture.entities) == ENEMY_COUNT);
    for (index = 0U; index < ENEMY_COUNT; ++index) {
        CHECK(despawn_enemy(&fixture, enemies[index]));
        CHECK(composition_absent(&fixture, enemies[index]));
    }
    CHECK(hth_entity_registry_live_count(fixture.entities) ==
          initial_live_count);
    fixture_destroy(&fixture);
    return true;
}

static bool test_despawn_validation_and_optional_components(void)
{
    Fixture fixture;
    HTHEnemyRuntimeSpawnSpec spec = spawn_spec(0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle bodyless;
    HTHEntityHandle healthless;
    HTHEntityHandle spatialless;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_enemy(&fixture, &spec, &enemy));
#define CHECK_DESPAWN_FAILURE(arguments)                                     \
    do {                                                                     \
        CHECK(!hth_enemy_runtime_despawn arguments);                         \
        CHECK(runtime_matches(&fixture, enemy, &spec));                      \
    } while (0)
    CHECK_DESPAWN_FAILURE((NULL, fixture.actors, fixture.enemies,
                           fixture.cadences,
                           fixture.spatial, fixture.bodies, fixture.health,
                           fixture.targets, enemy));
    CHECK_DESPAWN_FAILURE((fixture.entities, NULL, fixture.enemies,
                           fixture.cadences,
                           fixture.spatial, fixture.bodies, fixture.health,
                           fixture.targets, enemy));
    CHECK_DESPAWN_FAILURE((fixture.entities, fixture.actors, NULL,
                           fixture.cadences,
                           fixture.spatial, fixture.bodies, fixture.health,
                           fixture.targets, enemy));
    CHECK_DESPAWN_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                           NULL, fixture.spatial, fixture.bodies,
                           fixture.health, fixture.targets, enemy));
    CHECK_DESPAWN_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                           fixture.cadences, NULL, fixture.bodies,
                           fixture.health,
                           fixture.targets, enemy));
    CHECK_DESPAWN_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                           fixture.cadences, fixture.spatial, NULL,
                           fixture.health,
                           fixture.targets, enemy));
    CHECK_DESPAWN_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                           fixture.cadences, fixture.spatial, fixture.bodies,
                           NULL,
                           fixture.targets, enemy));
    CHECK_DESPAWN_FAILURE((fixture.entities, fixture.actors, fixture.enemies,
                           fixture.cadences, fixture.spatial, fixture.bodies,
                           fixture.health,
                           NULL, enemy));
#undef CHECK_DESPAWN_FAILURE
    CHECK(despawn_enemy(&fixture, enemy));
    CHECK(!despawn_enemy(&fixture, enemy));

    CHECK(spawn_enemy(&fixture, &spec, &bodyless));
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities,
                                  bodyless));
    CHECK(despawn_enemy(&fixture, bodyless));
    CHECK(composition_absent(&fixture, bodyless));

    CHECK(spawn_enemy(&fixture, &spec, &healthless));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, healthless));
    CHECK(despawn_enemy(&fixture, healthless));
    CHECK(composition_absent(&fixture, healthless));

    CHECK(spawn_enemy(&fixture, &spec, &spatialless));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   spatialless));
    CHECK(despawn_enemy(&fixture, spatialless));
    CHECK(composition_absent(&fixture, spatialless));
    fixture_destroy(&fixture);
    return true;
}

static bool test_missing_ownership_is_not_cascaded(void)
{
    Fixture fixture;
    HTHEnemyRuntimeSpawnSpec spec = spawn_spec(0.0F);
    HTHEntityHandle missing_enemy;
    HTHEntityHandle missing_actor;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_enemy(&fixture, &spec, &missing_enemy));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities,
                                 missing_enemy));
    CHECK(!despawn_enemy(&fixture, missing_enemy));
    CHECK(hth_entity_registry_is_alive(fixture.entities, missing_enemy));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                              missing_enemy));
    CHECK(hth_actor_despawn(fixture.entities, fixture.actors,
                            fixture.spatial, fixture.bodies, fixture.health,
                            missing_enemy));

    CHECK(spawn_enemy(&fixture, &spec, &missing_actor));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 missing_actor));
    CHECK(!despawn_enemy(&fixture, missing_actor));
    CHECK(hth_entity_registry_is_alive(fixture.entities, missing_actor));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities,
                                 missing_actor));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 missing_actor));
    CHECK(hth_actor_despawn(fixture.entities, fixture.actors,
                            fixture.spatial, fixture.bodies, fixture.health,
                            missing_actor));
    fixture_destroy(&fixture);
    return true;
}

static bool test_target_cleanup_generation_and_isolation(void)
{
    Fixture fixture;
    HTHEnemyRuntimeSpawnSpec first_spec = spawn_spec(0.0F);
    HTHEnemyRuntimeSpawnSpec second_spec = spawn_spec(2.0F);
    HTHEnemyRuntimeSpawnSpec third_spec = spawn_spec(4.0F);
    HTHEntityHandle first;
    HTHEntityHandle second;
    HTHEntityHandle third;
    HTHEntityHandle replacement;
    HTHEntityHandle target;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_enemy(&fixture, &first_spec, &first));
    CHECK(spawn_enemy(&fixture, &second_spec, &second));
    CHECK(spawn_enemy(&fixture, &third_spec, &third));
    CHECK(hth_enemy_target_store_set(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        first, second));
    CHECK(hth_enemy_target_store_set(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        third, second));
    CHECK(despawn_enemy(&fixture, first));
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        first, &target));
    CHECK(runtime_matches(&fixture, second, &second_spec));
    CHECK(runtime_matches(&fixture, third, &third_spec) == false);
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        third, &target));
    CHECK(hth_entity_handle_equal(target, second));

    CHECK(despawn_enemy(&fixture, second));
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        third, &target));
    CHECK(spawn_enemy(&fixture, &first_spec, &replacement));
    CHECK(replacement.index == second.index);
    CHECK(replacement.generation != second.generation);
    CHECK(!hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        third, &target));
    CHECK(!despawn_enemy(&fixture, second));
    CHECK(runtime_matches(&fixture, replacement, &first_spec));
    CHECK(despawn_enemy(&fixture, replacement));
    CHECK(despawn_enemy(&fixture, third));
    fixture_destroy(&fixture);
    return true;
}

static bool test_independent_store_sets(void)
{
    Fixture first_fixture;
    Fixture second_fixture;
    HTHEnemyRuntimeSpawnSpec spec = spawn_spec(0.0F);
    HTHEntityHandle first;
    HTHEntityHandle second;

    CHECK(fixture_create(&first_fixture));
    CHECK(fixture_create(&second_fixture));
    CHECK(spawn_enemy(&first_fixture, &spec, &first));
    CHECK(spawn_enemy(&second_fixture, &spec, &second));
    CHECK(hth_entity_handle_equal(first, second));
    CHECK(despawn_enemy(&first_fixture, first));
    CHECK(runtime_matches(&second_fixture, second, &spec));
    CHECK(despawn_enemy(&second_fixture, second));
    fixture_destroy(&second_fixture);
    fixture_destroy(&first_fixture);
    return true;
}

static HTHCollisionWorld clear_world(void)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = (HTHAABB){{100.0F, -10.0F, -10.0F},
                                   {101.0F, 10.0F, 10.0F}};
    world.obstacle_count = 1U;
    return world;
}

static bool test_selection_and_pursuit_integration(void)
{
    Fixture fixture;
    HTHCollisionWorld world = clear_world();
    HTHEnemyRuntimeSpawnSpec first_spec = spawn_spec(0.0F);
    HTHEnemyRuntimeSpawnSpec second_spec = spawn_spec(0.0F);
    HTHEntityHandle first;
    HTHEntityHandle second;
    HTHEntityHandle ordinary;
    HTHEntityHandle selected;
    HTHEntityHandle candidates[1];
    HTHSpatialTransform ordinary_transform =
        {{3.0F, 2.0F, 3.0F}, 0.0F};
    HTHSpatialTransform moved;

    first_spec.transform.position = (HTHVec3){0.0F, 0.0F, 0.0F};
    first_spec.body.velocity = (HTHVec3){0.0F, 0.0F, 0.0F};
    second_spec.transform.position = (HTHVec3){4.0F, 0.0F, 0.0F};
    second_spec.body.velocity = (HTHVec3){0.0F, 0.0F, 0.0F};

    CHECK(fixture_create(&fixture));
    CHECK(spawn_enemy(&fixture, &first_spec, &first));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &ordinary));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   ordinary, &ordinary_transform));
    candidates[0] = ordinary;
    CHECK(hth_enemy_target_select(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &world, fixture.targets, first, candidates, 1U,
        NULL, 0U, 10.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, ordinary));
    CHECK(hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                       first));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   ordinary));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, ordinary));

    CHECK(spawn_enemy(&fixture, &second_spec, &second));
    candidates[0] = second;
    CHECK(hth_enemy_target_select(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        &world, fixture.targets, first, candidates, 1U,
        NULL, 0U, 10.0F, &selected));
    CHECK(hth_entity_handle_equal(selected, second));
    CHECK(hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                       first));

    CHECK(hth_enemy_pursuit_runtime_step(
        fixture.entities, fixture.actors, fixture.enemies, fixture.spatial,
        fixture.bodies, fixture.health, fixture.targets, fixture.cadences,
        &world, candidates, 1U, NULL, 0U, 10.0F, 0.0F,
        2.0F, 0.0F, 1.0, 0.5));
    CHECK(hth_enemy_target_store_get(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        first, &selected));
    CHECK(hth_entity_handle_equal(selected, second));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, first,
                                &moved));
    CHECK(vec_equal(moved.position, (HTHVec3){1.0F, 0.0F, 0.0F}));
    CHECK(despawn_enemy(&fixture, first));
    CHECK(despawn_enemy(&fixture, second));
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_spawn_nulls_and_validation,
        test_spawn_payload_validation,
        test_exact_spawn_and_zero_health,
        test_growth_and_cleanup,
        test_despawn_validation_and_optional_components,
        test_missing_ownership_is_not_cascaded,
        test_target_cleanup_generation_and_isolation,
        test_independent_store_sets,
        test_selection_and_pursuit_integration
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy runtime population tests passed");
    return EXIT_SUCCESS;
}
