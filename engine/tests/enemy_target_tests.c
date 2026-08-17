#include "actor_spawn.h"
#include "enemy_target.h"

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

static bool create_entity(Fixture *fixture, HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity);
}

static bool create_enemy(Fixture *fixture, HTHEntityHandle *out_enemy)
{
    if (!create_entity(fixture, out_enemy) ||
        !hth_actor_store_attach(fixture->actors, fixture->entities,
                                *out_enemy)) {
        return false;
    }
    return hth_enemy_store_attach(fixture->enemies, fixture->entities,
                                  fixture->actors, *out_enemy);
}

static bool set_target(Fixture *fixture, HTHEntityHandle enemy,
                       HTHEntityHandle target)
{
    return hth_enemy_target_store_set(
        fixture->targets, fixture->entities, fixture->actors,
        fixture->enemies, enemy, target);
}

static bool has_target(Fixture *fixture, HTHEntityHandle enemy)
{
    return hth_enemy_target_store_has(
        fixture->targets, fixture->entities, fixture->actors,
        fixture->enemies, enemy);
}

static bool get_target(Fixture *fixture, HTHEntityHandle enemy,
                       HTHEntityHandle *out_target)
{
    return hth_enemy_target_store_get(
        fixture->targets, fixture->entities, fixture->actors,
        fixture->enemies, enemy, out_target);
}

static bool test_null_contracts(void)
{
    Fixture fixture;
    HTHEnemyTargetIterator iterator;
    HTHEnemyTargetPair pair = {{1U, 1U}, {2U, 1U}};
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle output = {4U, 5U};

    hth_enemy_target_store_destroy(NULL);
    hth_enemy_target_iterator_begin(NULL);
    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(create_entity(&fixture, &target));
    CHECK(!hth_enemy_target_store_set(NULL, fixture.entities,
                                      fixture.actors, fixture.enemies,
                                      enemy, target));
    CHECK(!hth_enemy_target_store_set(fixture.targets, NULL,
                                      fixture.actors, fixture.enemies,
                                      enemy, target));
    CHECK(!hth_enemy_target_store_set(fixture.targets, fixture.entities,
                                      NULL, fixture.enemies, enemy, target));
    CHECK(!hth_enemy_target_store_set(fixture.targets, fixture.entities,
                                      fixture.actors, NULL, enemy, target));
    CHECK(!hth_enemy_target_store_has(NULL, fixture.entities,
                                      fixture.actors, fixture.enemies,
                                      enemy));
    CHECK(!hth_enemy_target_store_get(NULL, fixture.entities,
                                      fixture.actors, fixture.enemies,
                                      enemy, &output));
    CHECK(hth_entity_handle_equal(output, hth_entity_handle_invalid()));
    CHECK(!hth_enemy_target_store_get(fixture.targets, fixture.entities,
                                      fixture.actors, fixture.enemies,
                                      enemy, NULL));
    CHECK(!hth_enemy_target_store_clear(NULL, fixture.entities, enemy));
    CHECK(!hth_enemy_target_store_clear(fixture.targets, NULL, enemy));
    hth_enemy_target_iterator_begin(&iterator);
    CHECK(!hth_enemy_target_iterator_next(
        NULL, fixture.entities, fixture.actors, fixture.enemies,
        &iterator, &pair));
    CHECK(hth_entity_handle_equal(pair.enemy, hth_entity_handle_invalid()));
    CHECK(hth_entity_handle_equal(pair.target, hth_entity_handle_invalid()));
    CHECK(!hth_enemy_target_iterator_next(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        NULL, &pair));
    CHECK(!hth_enemy_target_iterator_next(
        fixture.targets, fixture.entities, fixture.actors, fixture.enemies,
        &iterator, NULL));
    fixture_destroy(&fixture);
    return true;
}

static bool test_basic_lifecycle_and_non_actor_target(void)
{
    Fixture fixture;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle output;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(create_entity(&fixture, &target));
    CHECK(!hth_actor_store_has(fixture.actors, fixture.entities, target));
    CHECK(!hth_spatial_store_has(fixture.spatial, fixture.entities, target));
    CHECK(set_target(&fixture, enemy, target));
    CHECK(has_target(&fixture, enemy));
    CHECK(get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, target));
    CHECK(hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                       enemy));
    CHECK(!has_target(&fixture, enemy));
    output = target;
    CHECK(!get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, hth_entity_handle_invalid()));
    CHECK(!hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                        enemy));
    CHECK(hth_entity_registry_is_alive(fixture.entities, enemy));
    CHECK(hth_entity_registry_is_alive(fixture.entities, target));
    CHECK(hth_enemy_store_has(fixture.enemies, fixture.entities,
                              fixture.actors, enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_replacement_idempotence_transaction_and_self(void)
{
    Fixture fixture;
    HTHEntityHandle enemy;
    HTHEntityHandle first;
    HTHEntityHandle second;
    HTHEntityHandle stale;
    HTHEntityHandle output;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(create_entity(&fixture, &first));
    CHECK(create_entity(&fixture, &second));
    CHECK(set_target(&fixture, enemy, first));
    CHECK(set_target(&fixture, enemy, first));
    CHECK(get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, first));
    CHECK(set_target(&fixture, enemy, second));
    CHECK(get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, second));
    CHECK(create_entity(&fixture, &stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(!set_target(&fixture, enemy, stale));
    CHECK(get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, second));
    CHECK(set_target(&fixture, enemy, enemy));
    CHECK(get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_owner_requirements_and_enemy_visibility(void)
{
    Fixture fixture;
    HTHEntityHandle actor;
    HTHEntityHandle enemy;
    HTHEntityHandle target;
    HTHEntityHandle output;

    CHECK(fixture_create(&fixture));
    CHECK(create_entity(&fixture, &actor));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, actor));
    CHECK(create_entity(&fixture, &target));
    CHECK(!set_target(&fixture, actor, target));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(set_target(&fixture, enemy, target));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, enemy));
    CHECK(!has_target(&fixture, enemy));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, enemy));
    CHECK(has_target(&fixture, enemy));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, enemy));
    CHECK(!has_target(&fixture, enemy));
    CHECK(!get_target(&fixture, enemy, &output));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(has_target(&fixture, enemy));
    CHECK(get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, target));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities, enemy));
    CHECK(hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                       enemy));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(!has_target(&fixture, enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool test_owner_death_reuse_and_stale_clear(void)
{
    Fixture fixture;
    HTHEntityHandle stale_owner;
    HTHEntityHandle replacement;
    HTHEntityHandle first_target;
    HTHEntityHandle second_target;
    HTHEntityHandle output;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &stale_owner));
    CHECK(create_entity(&fixture, &first_target));
    CHECK(set_target(&fixture, stale_owner, first_target));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale_owner));
    CHECK(!has_target(&fixture, stale_owner));
    CHECK(!hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                        stale_owner));
    CHECK(create_entity(&fixture, &replacement));
    CHECK(replacement.index == stale_owner.index);
    CHECK(replacement.generation != stale_owner.generation);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 replacement));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, replacement));
    CHECK(!has_target(&fixture, replacement));
    CHECK(create_entity(&fixture, &second_target));
    CHECK(set_target(&fixture, replacement, second_target));
    CHECK(!hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                        stale_owner));
    CHECK(get_target(&fixture, replacement, &output));
    CHECK(hth_entity_handle_equal(output, second_target));
    fixture_destroy(&fixture);
    return true;
}

static bool test_target_death_reuse_and_explicit_replacement(void)
{
    Fixture fixture;
    HTHEntityHandle enemy;
    HTHEntityHandle stale_target;
    HTHEntityHandle replacement;
    HTHEntityHandle output;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &enemy));
    CHECK(create_entity(&fixture, &stale_target));
    CHECK(set_target(&fixture, enemy, stale_target));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale_target));
    CHECK(!has_target(&fixture, enemy));
    CHECK(!get_target(&fixture, enemy, &output));
    CHECK(create_entity(&fixture, &replacement));
    CHECK(replacement.index == stale_target.index);
    CHECK(replacement.generation != stale_target.generation);
    CHECK(!has_target(&fixture, enemy));
    CHECK(set_target(&fixture, enemy, replacement));
    CHECK(get_target(&fixture, enemy, &output));
    CHECK(hth_entity_handle_equal(output, replacement));
    fixture_destroy(&fixture);
    return true;
}

static HTHActorSpawnSpec full_spec(void)
{
    HTHActorSpawnSpec spec = {0};

    spec.has_spatial = true;
    spec.transform = (HTHSpatialTransform){{1.0F, 2.0F, 3.0F}, 0.5F};
    spec.has_body = true;
    spec.body = (HTHDynamicBody){{0.5F, 0.5F, 0.5F},
                                 {1.0F, 0.0F, -1.0F}};
    spec.has_health = true;
    spec.health = (HTHHealth){0.0F, 100.0F};
    return spec;
}

static bool test_component_independence(void)
{
    Fixture fixture;
    HTHActorSpawnSpec spec = full_spec();
    HTHEntityHandle enemy;
    HTHEntityHandle target;

    CHECK(fixture_create(&fixture));
    CHECK(hth_actor_spawn(fixture.entities, fixture.actors, fixture.spatial,
                          fixture.bodies, fixture.health, &spec, &enemy));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, enemy));
    CHECK(create_entity(&fixture, &target));
    CHECK(hth_spatial_store_attach(fixture.spatial, fixture.entities, target,
                                   &spec.transform));
    CHECK(set_target(&fixture, enemy, target));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, enemy));
    CHECK(has_target(&fixture, enemy));
    CHECK(hth_dynamic_body_remove(fixture.bodies, fixture.entities, enemy));
    CHECK(has_target(&fixture, enemy));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, enemy));
    CHECK(has_target(&fixture, enemy));
    CHECK(hth_spatial_store_remove(fixture.spatial, fixture.entities, target));
    CHECK(has_target(&fixture, enemy));
    fixture_destroy(&fixture);
    return true;
}

static bool iterator_matches(Fixture *fixture,
                             const HTHEnemyTargetPair *expected,
                             size_t expected_count)
{
    HTHEnemyTargetIterator iterator;
    HTHEnemyTargetPair pair;
    size_t count = 0U;

    hth_enemy_target_iterator_begin(&iterator);
    while (hth_enemy_target_iterator_next(
               fixture->targets, fixture->entities, fixture->actors,
               fixture->enemies, &iterator, &pair)) {
        if (count >= expected_count ||
            !hth_entity_handle_equal(pair.enemy, expected[count].enemy) ||
            !hth_entity_handle_equal(pair.target, expected[count].target)) {
            return false;
        }
        count++;
    }
    return count == expected_count &&
           hth_entity_handle_equal(pair.enemy,
                                   hth_entity_handle_invalid()) &&
           hth_entity_handle_equal(pair.target,
                                   hth_entity_handle_invalid());
}

static bool test_iterator_filtering_and_order(void)
{
    Fixture fixture;
    HTHEntityHandle owners[6];
    HTHEntityHandle targets[6];
    HTHEntityHandle target_replacement;
    HTHEnemyTargetPair expected[4];
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(iterator_matches(&fixture, NULL, 0U));
    for (index = 0U; index < 6U; ++index) {
        CHECK(create_enemy(&fixture, &owners[index]));
        CHECK(create_entity(&fixture, &targets[index]));
        CHECK(set_target(&fixture, owners[index], targets[index]));
    }
    CHECK(hth_enemy_target_store_clear(fixture.targets, fixture.entities,
                                       owners[1]));
    CHECK(hth_enemy_store_remove(fixture.enemies, fixture.entities,
                                 owners[2]));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, targets[3]));
    CHECK(create_entity(&fixture, &target_replacement));
    CHECK(target_replacement.index == targets[3].index);
    CHECK(target_replacement.generation != targets[3].generation);
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, owners[4]));
    expected[0] = (HTHEnemyTargetPair){owners[0], targets[0]};
    expected[1] = (HTHEnemyTargetPair){owners[5], targets[5]};
    CHECK(iterator_matches(&fixture, expected, 2U));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, owners[2]));
    expected[1] = (HTHEnemyTargetPair){owners[2], targets[2]};
    expected[2] = (HTHEnemyTargetPair){owners[5], targets[5]};
    CHECK(iterator_matches(&fixture, expected, 3U));
    fixture_destroy(&fixture);
    return true;
}

static bool test_growth_preservation(void)
{
    enum { RELATION_COUNT = 130 };
    Fixture fixture;
    HTHEntityHandle owners[RELATION_COUNT];
    HTHEntityHandle output;
    HTHEnemyTargetIterator iterator;
    HTHEnemyTargetPair pair;
    size_t index;
    size_t count = 0U;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < RELATION_COUNT; ++index) {
        CHECK(create_enemy(&fixture, &owners[index]));
        CHECK(set_target(&fixture, owners[index], owners[index]));
    }
    CHECK(get_target(&fixture, owners[0], &output));
    CHECK(hth_entity_handle_equal(output, owners[0]));
    CHECK(get_target(&fixture, owners[64], &output));
    CHECK(hth_entity_handle_equal(output, owners[64]));
    CHECK(get_target(&fixture, owners[129], &output));
    CHECK(hth_entity_handle_equal(output, owners[129]));
    hth_enemy_target_iterator_begin(&iterator);
    while (hth_enemy_target_iterator_next(
               fixture.targets, fixture.entities, fixture.actors,
               fixture.enemies, &iterator, &pair)) {
        CHECK(count < RELATION_COUNT);
        CHECK(hth_entity_handle_equal(pair.enemy, owners[count]));
        CHECK(hth_entity_handle_equal(pair.target, owners[count]));
        count++;
    }
    CHECK(count == RELATION_COUNT);
    fixture_destroy(&fixture);
    return true;
}

static bool test_multiple_stores_and_destroy_independence(void)
{
    Fixture first;
    Fixture second;
    HTHEntityHandle first_enemy;
    HTHEntityHandle first_target;
    HTHEntityHandle second_enemy;
    HTHEntityHandle second_target;

    CHECK(fixture_create(&first));
    CHECK(fixture_create(&second));
    CHECK(create_enemy(&first, &first_enemy));
    CHECK(create_entity(&first, &first_target));
    CHECK(create_enemy(&second, &second_enemy));
    CHECK(create_entity(&second, &second_target));
    CHECK(hth_entity_handle_equal(first_enemy, second_enemy));
    CHECK(set_target(&first, first_enemy, first_target));
    CHECK(!has_target(&second, second_enemy));
    CHECK(set_target(&second, second_enemy, second_target));
    hth_enemy_target_store_destroy(first.targets);
    first.targets = NULL;
    CHECK(hth_entity_registry_is_alive(first.entities, first_enemy));
    CHECK(hth_entity_registry_is_alive(first.entities, first_target));
    CHECK(hth_enemy_store_has(first.enemies, first.entities, first.actors,
                              first_enemy));
    CHECK(has_target(&second, second_enemy));
    fixture_destroy(&second);
    fixture_destroy(&first);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_null_contracts,
        test_basic_lifecycle_and_non_actor_target,
        test_replacement_idempotence_transaction_and_self,
        test_owner_requirements_and_enemy_visibility,
        test_owner_death_reuse_and_stale_clear,
        test_target_death_reuse_and_explicit_replacement,
        test_component_independence,
        test_iterator_filtering_and_order,
        test_growth_preservation,
        test_multiple_stores_and_destroy_independence
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("enemy target tests passed");
    return EXIT_SUCCESS;
}
