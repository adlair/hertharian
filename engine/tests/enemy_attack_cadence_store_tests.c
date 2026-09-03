#include "enemy_attack_cadence_store.h"

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
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->enemies = hth_enemy_store_create();
    fixture->cadences = hth_enemy_attack_cadence_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->enemies != NULL && fixture->cadences != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_enemy_attack_cadence_store_destroy(fixture->cadences);
    hth_enemy_store_destroy(fixture->enemies);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static bool create_enemy(Fixture *fixture, HTHEntityHandle *out_enemy)
{
    return hth_entity_registry_create_entity(fixture->entities, out_enemy) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_enemy) &&
           hth_enemy_store_attach(fixture->enemies, fixture->entities,
                                  fixture->actors, *out_enemy);
}

static bool destroy_enemy(Fixture *fixture, HTHEntityHandle enemy)
{
    return hth_enemy_store_remove(fixture->enemies, fixture->entities,
                                  enemy) &&
           hth_actor_store_remove(fixture->actors, fixture->entities,
                                  enemy) &&
           hth_entity_registry_destroy_entity(fixture->entities, enemy);
}

static bool test_contracts_and_lifecycle(void)
{
    Fixture fixture;
    HTHEntityHandle entity;
    HTHEnemyAttackCadence *cadence = (HTHEnemyAttackCadence *)1;
    bool ready = false;

    hth_enemy_attack_cadence_store_destroy(NULL);
    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity));
    CHECK(!hth_enemy_attack_cadence_store_attach(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        entity));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, entity));
    CHECK(hth_enemy_store_attach(fixture.enemies, fixture.entities,
                                 fixture.actors, entity));
    CHECK(!hth_enemy_attack_cadence_store_has(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        entity));
    CHECK(hth_enemy_attack_cadence_store_attach(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        entity));
    CHECK(!hth_enemy_attack_cadence_store_attach(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        entity));
    CHECK(hth_enemy_attack_cadence_store_get_mutable(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        entity, &cadence));
    CHECK(cadence != NULL && cadence->remaining_seconds == 0.0);
    CHECK(hth_enemy_attack_cadence_is_ready(cadence, &ready) && ready);
    CHECK(!hth_enemy_attack_cadence_store_get_mutable(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        entity, NULL));
    CHECK(hth_enemy_attack_cadence_store_remove(
        fixture.cadences, fixture.entities, entity));
    CHECK(!hth_enemy_attack_cadence_store_remove(
        fixture.cadences, fixture.entities, entity));
    CHECK(!hth_enemy_attack_cadence_store_has(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        entity));
    fixture_destroy(&fixture);
    return true;
}

static bool test_generation_reuse_is_fresh(void)
{
    Fixture fixture;
    HTHEntityHandle old_enemy;
    HTHEntityHandle new_enemy;
    HTHEnemyAttackCadence *cadence;

    CHECK(fixture_create(&fixture));
    CHECK(create_enemy(&fixture, &old_enemy));
    CHECK(hth_enemy_attack_cadence_store_attach(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        old_enemy));
    CHECK(hth_enemy_attack_cadence_store_get_mutable(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        old_enemy, &cadence));
    CHECK(hth_enemy_attack_cadence_commit(cadence, 4.0));
    CHECK(hth_enemy_attack_cadence_store_remove(
        fixture.cadences, fixture.entities, old_enemy));
    CHECK(destroy_enemy(&fixture, old_enemy));
    CHECK(create_enemy(&fixture, &new_enemy));
    CHECK(new_enemy.index == old_enemy.index);
    CHECK(new_enemy.generation != old_enemy.generation);
    CHECK(!hth_enemy_attack_cadence_store_has(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        old_enemy));
    CHECK(!hth_enemy_attack_cadence_store_has(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        new_enemy));
    CHECK(hth_enemy_attack_cadence_store_attach(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        new_enemy));
    CHECK(hth_enemy_attack_cadence_store_get_mutable(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        new_enemy, &cadence));
    CHECK(cadence->remaining_seconds == 0.0);
    fixture_destroy(&fixture);
    return true;
}

static bool test_growth_and_independence(void)
{
    Fixture fixture;
    HTHEntityHandle enemies[130];
    HTHEnemyAttackCadence *cadence;
    size_t index;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < 130U; ++index) {
        CHECK(create_enemy(&fixture, &enemies[index]));
        CHECK(hth_enemy_attack_cadence_store_attach(
            fixture.cadences, fixture.entities, fixture.actors,
            fixture.enemies, enemies[index]));
    }
    CHECK(hth_enemy_attack_cadence_store_get_mutable(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        enemies[0], &cadence));
    CHECK(hth_enemy_attack_cadence_commit(cadence, 2.0));
    for (index = 1U; index < 130U; ++index) {
        CHECK(hth_enemy_attack_cadence_store_get_mutable(
            fixture.cadences, fixture.entities, fixture.actors,
            fixture.enemies, enemies[index], &cadence));
        CHECK(cadence->remaining_seconds == 0.0);
    }
    CHECK(hth_enemy_attack_cadence_store_get_mutable(
        fixture.cadences, fixture.entities, fixture.actors, fixture.enemies,
        enemies[0], &cadence));
    CHECK(cadence->remaining_seconds == 2.0);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    if (!test_contracts_and_lifecycle() ||
        !test_generation_reuse_is_fresh() ||
        !test_growth_and_independence()) {
        return EXIT_FAILURE;
    }
    puts("enemy attack cadence store tests passed");
    return EXIT_SUCCESS;
}
