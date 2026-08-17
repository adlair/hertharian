#include "actor_spawn.h"
#include "enemy.h"

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
    fixture->spatial = hth_spatial_store_create();
    fixture->bodies = hth_dynamic_body_store_create();
    fixture->health = hth_health_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->spatial != NULL &&
           fixture->bodies != NULL && fixture->health != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_store_destroy(fixture->enemies);
    hth_health_store_destroy(fixture->health);
    hth_dynamic_body_store_destroy(fixture->bodies);
    hth_spatial_store_destroy(fixture->spatial);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static bool spawn_actor(Fixture *fixture, const HTHActorSpawnSpec *spec,
                        HTHEntityHandle *out_entity)
{
    return hth_actor_spawn(fixture->entities, fixture->actors,
                           fixture->spatial, fixture->bodies,
                           fixture->health, spec, out_entity);
}

static bool test_null_contracts(void)
{
    Fixture fixture;
    HTHEnemyIterator iterator;
    HTHEntityHandle entity;
    HTHEntityHandle output = {4U, 5U};

    hth_enemy_store_destroy(NULL);
    hth_enemy_iterator_begin(NULL);
    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, entity));
    CHECK(!hth_enemy_store_attach(NULL, fixture.entities, fixture.actors,
                                  entity));
    CHECK(!hth_enemy_store_attach(fixture.enemies, NULL, fixture.actors,
                                  entity));
    CHECK(!hth_enemy_store_attach(fixture.enemies, fixture.entities, NULL,
                                  entity));
    CHECK(!hth_enemy_store_has(NULL, fixture.entities, fixture.actors,
                               entity));
    CHECK(!hth_enemy_store_has(fixture.enemies, NULL, fixture.actors,
                               entity));
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities, NULL,
                               entity));
    CHECK(!hth_enemy_store_remove(NULL, fixture.entities, entity));
    CHECK(!hth_enemy_store_remove(fixture.enemies, NULL, entity));
    hth_enemy_iterator_begin(&iterator);
    CHECK(!hth_enemy_iterator_next(NULL, fixture.entities, fixture.actors,
                                   &iterator, &output));
    CHECK(hth_entity_handle_equal(output, hth_entity_handle_invalid()));
    CHECK(!hth_enemy_iterator_next(fixture.enemies, NULL, fixture.actors,
                                   &iterator, &output));
    CHECK(!hth_enemy_iterator_next(fixture.enemies, fixture.entities, NULL,
                                   &iterator, &output));
    CHECK(!hth_enemy_iterator_next(fixture.enemies, fixture.entities,
                                   fixture.actors, NULL, &output));
    CHECK(!hth_enemy_iterator_next(fixture.enemies, fixture.entities,
                                   fixture.actors, &iterator, NULL));
    fixture_destroy(&fixture);
    return true;
}

static bool test_basic_actor_requirements_and_duplicate(void)
{
    Fixture fixture;
    HTHEntityHandle entity;
    HTHEntityHandle dead;

    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity));
    CHECK(!hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                  fixture.actors, entity));
    CHECK(hth_entity_registry_is_alive(fixture.entities, entity));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, entity));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, entity));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, entity));
    CHECK(!hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                  fixture.actors, entity));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, entity));
    CHECK(hth_entity_registry_is_alive(fixture.entities, entity));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, entity));
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities,
                               fixture.actors, entity));
    CHECK(!hth_enemy_store_remove(fixture.enemies, fixture.entities, entity));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &dead));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, dead));
    CHECK(!hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                  fixture.actors, dead));
    fixture_destroy(&fixture);
    return true;
}

static bool test_actor_only_spawn_and_actor_visibility(void)
{
    Fixture fixture;
    const HTHActorSpawnSpec spec = {0};
    HTHEntityHandle entity;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_actor(&fixture, &spec, &entity));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, entity));
    CHECK(hth_entity_registry_is_alive(fixture.entities, entity));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, entity));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, entity));
    CHECK(!hth_spatial_store_has(fixture.spatial, fixture.entities, entity));
    CHECK(!hth_dynamic_body_has(fixture.bodies, fixture.entities, entity));
    CHECK(!hth_health_store_has(fixture.health, fixture.entities,
                                fixture.actors, entity));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, entity));
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities,
                               fixture.actors, entity));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, entity));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, entity));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, entity));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, entity));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, entity));
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities,
                               fixture.actors, entity));
    fixture_destroy(&fixture);
    return true;
}

static bool test_entity_death_reuse_and_stale_safety(void)
{
    Fixture fixture;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;

    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &stale));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, stale));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities,
                               fixture.actors, stale));
    CHECK(!hth_enemy_store_remove(fixture.enemies, fixture.entities, stale));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(replacement.index == stale.index);
    CHECK(replacement.generation != stale.generation);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 replacement));
    CHECK(!hth_enemy_store_has(fixture.enemies, fixture.entities,
                               fixture.actors, replacement));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, replacement));
    CHECK(!hth_enemy_store_remove(fixture.enemies, fixture.entities, stale));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, replacement));
    fixture_destroy(&fixture);
    return true;
}

static bool transform_equal(HTHSpatialTransform left,
                            HTHSpatialTransform right)
{
    return left.position.x == right.position.x &&
           left.position.y == right.position.y &&
           left.position.z == right.position.z && left.yaw == right.yaw;
}

static bool body_equal(HTHDynamicBody left, HTHDynamicBody right)
{
    return left.half_extents.x == right.half_extents.x &&
           left.half_extents.y == right.half_extents.y &&
           left.half_extents.z == right.half_extents.z &&
           left.velocity.x == right.velocity.x &&
           left.velocity.y == right.velocity.y &&
           left.velocity.z == right.velocity.z;
}

static bool health_equal(HTHHealth left, HTHHealth right)
{
    return left.current == right.current && left.maximum == right.maximum;
}

static HTHActorSpawnSpec full_spec(float health_current)
{
    HTHActorSpawnSpec spec = {0};

    spec.has_spatial = true;
    spec.transform = (HTHSpatialTransform){{2.0F, 3.0F, 4.0F}, 0.75F};
    spec.has_body = true;
    spec.body = (HTHDynamicBody){{0.5F, 1.0F, 0.5F},
                                 {1.0F, 2.0F, 3.0F}};
    spec.has_health = true;
    spec.health = (HTHHealth){health_current, 100.0F};
    return spec;
}

static bool test_full_composition_remove_independence(void)
{
    Fixture fixture;
    HTHActorSpawnSpec spec = full_spec(60.0F);
    HTHSpatialTransform transform;
    HTHDynamicBody body;
    HTHHealth health;
    HTHEntityHandle entity;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_actor(&fixture, &spec, &entity));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, entity));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, entity));
    CHECK(hth_entity_registry_is_alive(fixture.entities, entity));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, entity));
    CHECK(hth_spatial_store_get(fixture.spatial, fixture.entities, entity,
                                &transform));
    CHECK(transform_equal(transform, spec.transform));
    CHECK(hth_dynamic_body_get(fixture.bodies, fixture.entities, entity,
                               &body));
    CHECK(body_equal(body, spec.body));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, entity, &health));
    CHECK(health_equal(health, spec.health));
    fixture_destroy(&fixture);
    return true;
}

static bool test_optional_component_removal_and_zero_health(void)
{
    Fixture fixture;
    HTHActorSpawnSpec spec = full_spec(0.0F);
    HTHEntityHandle entity;

    CHECK(fixture_create(&fixture));
    CHECK(spawn_actor(&fixture, &spec, &entity));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, entity));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, entity));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, entity));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, entity));
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities, entity));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, entity));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, entity));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, entity));
    fixture_destroy(&fixture);
    return true;
}

static bool iterator_contains_only(Fixture *fixture,
                                   const HTHEntityHandle *expected,
                                   size_t expected_count)
{
    HTHEnemyIterator iterator;
    HTHEntityHandle entity;
    size_t count = 0U;

    hth_enemy_iterator_begin(&iterator);
    while (hth_enemy_iterator_next(fixture->enemies, fixture->entities,
                                   fixture->actors, &iterator, &entity)) {
        if (count >= expected_count ||
            !hth_entity_handle_equal(entity, expected[count])) {
            return false;
        }
        count++;
    }
    return count == expected_count &&
           hth_entity_handle_equal(entity, hth_entity_handle_invalid());
}

static bool test_iterator_filtering_order_and_reattach(void)
{
    Fixture fixture;
    HTHEntityHandle handles[5];
    HTHEntityHandle expected[3];
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(iterator_contains_only(&fixture, NULL, 0U));
    for (index = 0U; index < 5U; ++index) {
        CHECK(hth_entity_registry_create_entity(fixture.entities,
                                                &handles[index]));
        CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                     handles[index]));
        if (index != 1U) {
            CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                         fixture.actors, handles[index]));
        }
    }
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, handles[2]));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 handles[3]));
    expected[0] = handles[0];
    expected[1] = handles[4];
    CHECK(iterator_contains_only(&fixture, expected, 2U));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 handles[3]));
    expected[1] = handles[3];
    expected[2] = handles[4];
    CHECK(iterator_contains_only(&fixture, expected, 3U));
    fixture_destroy(&fixture);
    return true;
}

static bool test_iterator_generation_reuse(void)
{
    Fixture fixture;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHEntityHandle expected[1];

    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &stale));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, stale));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 replacement));
    CHECK(iterator_contains_only(&fixture, NULL, 0U));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, replacement));
    expected[0] = replacement;
    CHECK(iterator_contains_only(&fixture, expected, 1U));
    fixture_destroy(&fixture);
    return true;
}

static bool test_growth_mass_removal_and_holes(void)
{
    enum { ENTITY_COUNT = 130 };
    Fixture fixture;
    HTHEntityHandle handles[ENTITY_COUNT];
    HTHEnemyIterator iterator;
    HTHEntityHandle entity;
    size_t expected_index = 0U;
    size_t index;
    size_t iterated = 0U;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_entity_registry_create_entity(fixture.entities,
                                                &handles[index]));
        CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                     handles[index]));
        CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                     fixture.actors, handles[index]));
    }
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, handles[0]));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, handles[64]));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, handles[129]));
    for (index = 0U; index < ENTITY_COUNT; index += 3U) {
        CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities,
                                     handles[index]));
    }
    for (index = 0U; index < ENTITY_COUNT; ++index) {
        CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                                  fixture.actors, handles[index]) ==
              (index % 3U != 0U));
    }
    hth_enemy_iterator_begin(&iterator);
    while (hth_enemy_iterator_next(fixture.enemies, fixture.entities,
                                   fixture.actors, &iterator, &entity)) {
        while (expected_index < ENTITY_COUNT && expected_index % 3U == 0U) {
            expected_index++;
        }
        CHECK(expected_index < ENTITY_COUNT);
        CHECK(hth_entity_handle_equal(entity, handles[expected_index]));
        expected_index++;
        iterated++;
    }
    CHECK(iterated == 86U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_multiple_store_sets(void)
{
    Fixture first;
    Fixture second;
    HTHEntityHandle first_entity;
    HTHEntityHandle second_entity;

    CHECK(fixture_create(&first));
    CHECK(fixture_create(&second));
    CHECK(hth_entity_registry_create_entity(first.entities, &first_entity));
    CHECK(hth_entity_registry_create_entity(second.entities, &second_entity));
    CHECK(hth_entity_handle_equal(first_entity, second_entity));
    CHECK(hth_actor_store_attach(first.actors, first.entities, first_entity));
    CHECK(hth_actor_store_attach(second.actors, second.entities,
                                 second_entity));
    CHECK(hth_enemy_store_attach(first.enemies, first.entities, first.actors,
                                 first_entity));
    CHECK(hth_enemy_store_has(first.enemies, first.entities, first.actors,
                              first_entity));
    CHECK(!hth_enemy_store_has(second.enemies, second.entities,
                               second.actors, second_entity));
    CHECK(hth_enemy_store_attach(second.enemies, second.entities,
                                 second.actors, second_entity));
    CHECK(hth_enemy_store_remove(first.enemies, first.entities,
                                 first_entity));
    CHECK(hth_enemy_store_has(second.enemies, second.entities,
                              second.actors, second_entity));
    fixture_destroy(&second);
    fixture_destroy(&first);
    return true;
}

static bool test_destroy_store_with_live_associations(void)
{
    Fixture fixture;
    HTHEntityHandle handles[70];
    size_t index;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < 70U; ++index) {
        CHECK(hth_entity_registry_create_entity(fixture.entities,
                                                &handles[index]));
        CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                     handles[index]));
        CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                     fixture.actors, handles[index]));
    }
    hth_enemy_store_destroy(fixture.enemies);
    fixture.enemies = NULL;
    for (index = 0U; index < 70U; ++index) {
        CHECK(hth_entity_registry_is_alive(fixture.entities, handles[index]));
        CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                                  handles[index]));
    }
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_null_contracts,
        test_basic_actor_requirements_and_duplicate,
        test_actor_only_spawn_and_actor_visibility,
        test_entity_death_reuse_and_stale_safety,
        test_full_composition_remove_independence,
        test_optional_component_removal_and_zero_health,
        test_iterator_filtering_order_and_reattach,
        test_iterator_generation_reuse,
        test_growth_mass_removal_and_holes,
        test_multiple_store_sets,
        test_destroy_store_with_live_associations
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy tests passed");
    return EXIT_SUCCESS;
}
