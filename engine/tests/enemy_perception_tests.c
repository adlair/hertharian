#include "actor_spawn.h"
#include "enemy_perception.h"
#include "enemy_target.h"

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

static bool create_entity(Fixture *fixture, HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity);
}

static bool create_enemy(Fixture *fixture,
                         const HTHSpatialTransform *spatial_transform,
                         HTHEntityHandle *out_enemy)
{
    if (!create_entity(fixture, out_enemy) ||
        !hth_actor_store_attach(fixture->actors, fixture->entities,
                                *out_enemy) ||
        !hth_enemy_store_attach(fixture->enemies, fixture->entities,
                                fixture->actors, *out_enemy)) {
        return false;
    }
    return spatial_transform == NULL ||
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_enemy, spatial_transform);
}

static bool create_candidate(Fixture *fixture,
                             const HTHSpatialTransform *spatial_transform,
                             HTHEntityHandle *out_candidate)
{
    if (!create_entity(fixture, out_candidate)) {
        return false;
    }
    return spatial_transform == NULL ||
           hth_spatial_store_attach(fixture->spatial, fixture->entities,
                                    *out_candidate, spatial_transform);
}

static bool can_perceive(Fixture *fixture, HTHEntityHandle enemy,
                         HTHEntityHandle candidate, float radius)
{
    return hth_enemy_perception_can_perceive(
        fixture->entities, fixture->actors, fixture->enemies,
        fixture->spatial, enemy, candidate, radius);
}

static bool set_target(Fixture *fixture, HTHEntityHandle enemy,
                       HTHEntityHandle target)
{
    return hth_enemy_target_store_set(
        fixture->targets, fixture->entities, fixture->actors,
        fixture->enemies, enemy, target);
}

static bool get_target(Fixture *fixture, HTHEntityHandle enemy,
                       HTHEntityHandle *out_target)
{
    return hth_enemy_target_store_get(
        fixture->targets, fixture->entities, fixture->actors,
        fixture->enemies, enemy, out_target);
}

static bool test_null_and_radius_contracts(void)
{
    Fixture fixture;
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &origin, &enemy));
    CHECK(create_candidate(&fixture, &origin, &candidate));
    CHECK(!hth_enemy_perception_can_perceive(
        NULL, fixture.actors, fixture.enemies, fixture.spatial,
        enemy, candidate, 1.0F));
    CHECK(!hth_enemy_perception_can_perceive(
        fixture.entities, NULL, fixture.enemies, fixture.spatial,
        enemy, candidate, 1.0F));
    CHECK(!hth_enemy_perception_can_perceive(
        fixture.entities, fixture.actors, NULL, fixture.spatial,
        enemy, candidate, 1.0F));
    CHECK(!hth_enemy_perception_can_perceive(
        fixture.entities, fixture.actors, fixture.enemies, NULL,
        enemy, candidate, 1.0F));
    CHECK(!can_perceive(&fixture, enemy, candidate, -1.0F));
    CHECK(!can_perceive(&fixture, enemy, candidate, NAN));
    CHECK(!can_perceive(&fixture, enemy, candidate, INFINITY));
    CHECK(!can_perceive(&fixture, enemy, candidate, -INFINITY));
    CHECK(can_perceive(&fixture, enemy, candidate, 0.0F));
    fixture_destroy(&fixture);
    return true;
}

static bool test_geometry_boundaries_and_3d(void)
{
    Fixture fixture;
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 1.0F);
    HTHSpatialTransform point = transform(3.0F, 4.0F, 0.0F, -2.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &origin, &enemy));
    CHECK(create_candidate(&fixture, &point, &candidate));
    CHECK(can_perceive(&fixture, enemy, candidate, 5.0F));
    CHECK(!can_perceive(&fixture, enemy, candidate, 4.99F));
    point = transform(1.0F, 2.0F, 2.0F, 0.0F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities,
                                candidate, &point));
    CHECK(can_perceive(&fixture, enemy, candidate, 3.0F));
    point = transform(0.0F, 3.0F, 0.0F, 0.0F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities,
                                candidate, &point));
    CHECK(can_perceive(&fixture, enemy, candidate, 3.0F));
    CHECK(!can_perceive(&fixture, enemy, candidate, 2.99F));
    point = transform(-3.0F, -4.0F, 0.0F, 0.0F);
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities,
                                candidate, &point));
    CHECK(can_perceive(&fixture, enemy, candidate, 5.0F));
    fixture_destroy(&fixture);
    return true;
}

static bool test_non_actor_candidate_and_missing_spatial(void)
{
    Fixture fixture;
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform near = transform(1.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &origin, &enemy));
    CHECK(create_candidate(&fixture, &near, &candidate));
    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities, candidate));
    CHECK(can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   candidate));
    CHECK(!can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   candidate, &near));
    CHECK(can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, enemy));
    CHECK(!can_perceive(&fixture, enemy, candidate, 1.0F));
    fixture_destroy(&fixture);
    return true;
}

static bool test_enemy_visibility_death_and_self(void)
{
    Fixture fixture;
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &origin, &enemy));
    CHECK(create_candidate(&fixture, &origin, &candidate));
    CHECK(can_perceive(&fixture, enemy, enemy, 0.0F));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, enemy));
    CHECK(!can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, candidate));
    CHECK(!can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, enemy));
    CHECK(!can_perceive(&fixture, enemy, enemy, 0.0F));
    fixture_destroy(&fixture);
    return true;
}

static bool test_yaw_independence_and_purity(void)
{
    Fixture fixture;
    HTHSpatialTransform observer = transform(0.0F, 0.0F, 0.0F, -3.0F);
    HTHSpatialTransform candidate_transform =
        transform(2.0F, 0.0F, 0.0F, 1.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &observer, &enemy));
    CHECK(create_candidate(&fixture, &candidate_transform, &candidate));
    for (index = 0U; index < 10U; ++index) {
        CHECK(can_perceive(&fixture, enemy, candidate, 2.0F));
    }
    observer.yaw = 2.75F;
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities, enemy,
                                &observer));
    CHECK(can_perceive(&fixture, enemy, candidate, 2.0F));
    CHECK(!hth_enemy_target_store_has(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_target_relation_independence(void)
{
    Fixture fixture;
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform near = transform(1.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform far = transform(100.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate_a;
    HTHEntityHandle target_b;
    HTHEntityHandle output;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &origin, &enemy));
    CHECK(create_candidate(&fixture, &near, &candidate_a));
    CHECK(create_candidate(&fixture, &far, &target_b));
    CHECK(can_perceive(&fixture, enemy, candidate_a, 1.0F));
    CHECK(!hth_enemy_target_store_has(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy));
    CHECK(set_target(&fixture, enemy, target_b));
    CHECK(can_perceive(&fixture, enemy, candidate_a, 1.0F));
    CHECK(get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, target_b));
    CHECK(!can_perceive(&fixture, enemy, target_b, 1.0F));
    CHECK(get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, target_b));
    fixture_destroy(&fixture);
    return true;
}

static HTHActorSpawnSpec full_spec(void)
{
    HTHActorSpawnSpec spec = {0};

    spec.has_spatial = true;
    spec.transform = transform(0.0F, 0.0F, 0.0F, 0.0F);
    spec.has_body = true;
    spec.body = (HTHDynamicBody){{0.5F, 0.5F, 0.5F},
                                 {0.0F, 0.0F, 0.0F}};
    spec.has_health = true;
    spec.health = (HTHHealth){0.0F, 100.0F};
    return spec;
}

static bool test_health_body_and_relation_spatial_independence(void)
{
    Fixture fixture;
    HTHActorSpawnSpec spec = full_spec();
    HTHSpatialTransform near = transform(1.0F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(hth_actor_spawn(fixture.entities, fixture.actors, fixture.spatial,
                          fixture.bodies, fixture.health, &spec, &enemy));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(create_candidate(&fixture, &near, &candidate));
    CHECK(set_target(&fixture, enemy, candidate));
    CHECK(can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, enemy));
    CHECK(can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities, enemy));
    CHECK(can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities,
                                   candidate));
    CHECK(hth_enemy_target_store_has(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy));
    CHECK(!can_perceive(&fixture, enemy, candidate, 1.0F));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities,
                                   candidate, &near));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, enemy));
    CHECK(hth_enemy_target_store_has(
        fixture.targets, fixture.entities, fixture.actors,
        fixture.enemies, enemy));
    CHECK(!can_perceive(&fixture, enemy, candidate, 1.0F));
    fixture_destroy(&fixture);
    return true;
}

static bool test_extreme_finite_coordinates(void)
{
    Fixture fixture;
    HTHSpatialTransform origin = transform(0.0F, 0.0F, 0.0F, 0.0F);
    HTHSpatialTransform extreme = transform(3.0e30F, 0.0F, 0.0F, 0.0F);
    HTHEntityHandle enemy;
    HTHEntityHandle candidate;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &origin, &enemy));
    CHECK(create_candidate(&fixture, &extreme, &candidate));
    CHECK(!can_perceive(&fixture, enemy, candidate, 2.0e30F));
    CHECK(can_perceive(&fixture, enemy, candidate, 3.0e30F));
    extreme.position.x = -3.0e30F;
    CHECK(hth_spatial_store_set(fixture.spatial, fixture.entities,
                                candidate, &extreme));
    CHECK(can_perceive(&fixture, enemy, candidate, 3.0e30F));
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_null_and_radius_contracts,
        test_geometry_boundaries_and_3d,
        test_non_actor_candidate_and_missing_spatial,
        test_enemy_visibility_death_and_self,
        test_yaw_independence_and_purity,
        test_target_relation_independence,
        test_health_body_and_relation_spatial_independence,
        test_extreme_finite_coordinates
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy perception tests passed");
    return EXIT_SUCCESS;
}
