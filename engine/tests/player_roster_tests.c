#include "player_roster.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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

typedef struct Fixture {
    HTHEntityRegistry *entities;
    HTHActorStore *actors;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    if (fixture->entities == NULL || fixture->actors == NULL) {
        hth_actor_store_destroy(fixture->actors);
        hth_entity_registry_destroy(fixture->entities);
        fixture->entities = NULL;
        fixture->actors = NULL;
        return false;
    }
    return true;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
    fixture->actors = NULL;
    fixture->entities = NULL;
}

static bool create_actor(Fixture *fixture, HTHEntityHandle *out_entity)
{
    return hth_entity_registry_create_entity(fixture->entities, out_entity) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_entity);
}

static bool handle_is_invalid(HTHEntityHandle entity)
{
    return hth_entity_handle_equal(entity, hth_entity_handle_invalid());
}

static bool roster_equal(const HTHPlayerRoster *left,
                         const HTHPlayerRoster *right)
{
    HTHPlayerSlot slot;

    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        if (left->entries[slot].occupied != right->entries[slot].occupied ||
            !hth_entity_handle_equal(left->entries[slot].entity,
                                     right->entries[slot].entity)) {
            return false;
        }
    }
    return true;
}

static bool test_zero_initialization_reset_and_invalid_inputs(void)
{
    HTHPlayerRoster roster = {0};
    HTHPlayerRoster snapshot = roster;
    Fixture fixture;
    HTHEntityHandle output = {3U, 7U};
    HTHEntityHandle entity;
    HTHPlayerSlot slot = 0U;
    HTHPlayerSlot index;

    CHECK(fixture_create(&fixture));
    CHECK(hth_player_roster_count(NULL) == 0U);
    CHECK(hth_player_roster_count(&roster) == 0U);
    CHECK(!hth_player_roster_unregister(&roster, 0U));
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        output = (HTHEntityHandle){3U, 7U};
        CHECK(!hth_player_roster_get_slot(
            &roster, fixture.entities, fixture.actors, index, &output));
        CHECK(handle_is_invalid(output));
    }
    CHECK(!hth_player_roster_get_slot(
        &roster, fixture.entities, fixture.actors, HTH_MAX_PLAYERS, &output));
    CHECK(handle_is_invalid(output));
    output = (HTHEntityHandle){3U, 7U};
    CHECK(!hth_player_roster_get_slot(
        NULL, fixture.entities, fixture.actors, 0U, &output));
    CHECK(handle_is_invalid(output));
    output = (HTHEntityHandle){3U, 7U};
    CHECK(!hth_player_roster_get_slot(
        &roster, NULL, fixture.actors, 0U, &output));
    CHECK(handle_is_invalid(output));
    output = (HTHEntityHandle){3U, 7U};
    CHECK(!hth_player_roster_get_slot(
        &roster, fixture.entities, NULL, 0U, &output));
    CHECK(handle_is_invalid(output));
    CHECK(!hth_player_roster_get_slot(
        &roster, fixture.entities, fixture.actors, 0U, NULL));

    CHECK(create_actor(&fixture, &entity));
    CHECK(!hth_player_roster_find_entity(
        NULL, fixture.entities, fixture.actors, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    slot = 0U;
    CHECK(!hth_player_roster_find_entity(
        &roster, NULL, fixture.actors, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    slot = 0U;
    CHECK(!hth_player_roster_find_entity(
        &roster, fixture.entities, NULL, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(!hth_player_roster_find_entity(
        &roster, fixture.entities, fixture.actors, entity, NULL));
    CHECK(roster_equal(&roster, &snapshot));

    hth_player_roster_reset(NULL);
    hth_player_roster_reset(&roster);
    CHECK(hth_player_roster_count(&roster) == 0U);
    CHECK(roster_equal(&roster, &snapshot));
    fixture_destroy(&fixture);
    return true;
}

static bool test_registration_role_and_transactionality(void)
{
    HTHPlayerRoster roster = {0};
    HTHPlayerRoster snapshot;
    Fixture fixture;
    HTHEntityHandle entity;
    HTHEntityHandle stale;
    HTHEntityHandle observed;
    HTHPlayerSlot slot = 0U;
    HTHPlayerSlot found = HTH_PLAYER_SLOT_INVALID;

    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &entity));
    snapshot = roster;
    CHECK(!hth_player_roster_register(
        &roster, fixture.entities, fixture.actors, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(roster_equal(&roster, &snapshot));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, entity));

    CHECK(!hth_player_roster_register(
        NULL, fixture.entities, fixture.actors, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    slot = 0U;
    CHECK(!hth_player_roster_register(
        &roster, NULL, fixture.actors, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    slot = 0U;
    CHECK(!hth_player_roster_register(
        &roster, fixture.entities, NULL, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(!hth_player_roster_register(
        &roster, fixture.entities, fixture.actors, entity, NULL));
    CHECK(roster_equal(&roster, &snapshot));

    CHECK(hth_player_roster_register(
        &roster, fixture.entities, fixture.actors, entity, &slot));
    CHECK(slot == 0U && hth_player_roster_count(&roster) == 1U);
    CHECK(hth_player_roster_get_slot(
        &roster, fixture.entities, fixture.actors, 0U, &observed));
    CHECK(hth_entity_handle_equal(observed, entity));
    CHECK(hth_player_roster_find_entity(
        &roster, fixture.entities, fixture.actors, entity, &found));
    CHECK(found == 0U);

    snapshot = roster;
    slot = 0U;
    CHECK(!hth_player_roster_register(
        &roster, fixture.entities, fixture.actors, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(roster_equal(&roster, &snapshot));

    CHECK(create_actor(&fixture, &stale));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    slot = 0U;
    CHECK(!hth_player_roster_register(
        &roster, fixture.entities, fixture.actors, stale, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(roster_equal(&roster, &snapshot));
    fixture_destroy(&fixture);
    return true;
}

static bool test_capacity_sparse_slots_and_reuse(void)
{
    HTHPlayerRoster roster = {0};
    HTHPlayerRoster full_snapshot;
    Fixture fixture;
    HTHEntityHandle entities[HTH_MAX_PLAYERS + 1U];
    HTHEntityHandle observed;
    HTHPlayerSlot slot;
    HTHPlayerSlot index;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < HTH_MAX_PLAYERS + 1U; ++index) {
        CHECK(create_actor(&fixture, &entities[index]));
    }
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        slot = HTH_PLAYER_SLOT_INVALID;
        CHECK(hth_player_roster_register(
            &roster, fixture.entities, fixture.actors, entities[index],
            &slot));
        CHECK(slot == index);
        CHECK(hth_player_roster_count(&roster) == index + 1U);
    }
    full_snapshot = roster;
    slot = 0U;
    CHECK(!hth_player_roster_register(
        &roster, fixture.entities, fixture.actors,
        entities[HTH_MAX_PLAYERS], &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(roster_equal(&roster, &full_snapshot));

    CHECK(hth_player_roster_unregister(&roster, 1U));
    CHECK(hth_player_roster_count(&roster) == HTH_MAX_PLAYERS - 1U);
    CHECK(!hth_player_roster_get_slot(
        &roster, fixture.entities, fixture.actors, 1U, &observed));
    CHECK(handle_is_invalid(observed));
    CHECK(hth_player_roster_get_slot(
        &roster, fixture.entities, fixture.actors, 2U, &observed));
    CHECK(hth_entity_handle_equal(observed, entities[2]));
    CHECK(hth_player_roster_get_slot(
        &roster, fixture.entities, fixture.actors, 3U, &observed));
    CHECK(hth_entity_handle_equal(observed, entities[3]));

    CHECK(hth_player_roster_register(
        &roster, fixture.entities, fixture.actors,
        entities[HTH_MAX_PLAYERS], &slot));
    CHECK(slot == 1U);
    CHECK(hth_player_roster_count(&roster) == HTH_MAX_PLAYERS);
    CHECK(!hth_player_roster_unregister(&roster, HTH_MAX_PLAYERS));
    CHECK(!hth_player_roster_unregister(NULL, 0U));
    CHECK(!hth_player_roster_unregister(&roster, HTH_MAX_PLAYERS));
    CHECK(roster.entries[0].occupied);
    fixture_destroy(&fixture);
    return true;
}

static bool test_stale_actor_loss_and_generation_reuse(void)
{
    HTHPlayerRoster roster = {0};
    Fixture fixture;
    HTHEntityHandle actor_lost;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHEntityHandle observed = {3U, 7U};
    HTHPlayerSlot slot;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &actor_lost));
    CHECK(hth_player_roster_register(
        &roster, fixture.entities, fixture.actors, actor_lost, &slot));
    CHECK(hth_actor_store_remove(
        fixture.actors, fixture.entities, actor_lost));
    CHECK(hth_player_roster_count(&roster) == 1U);
    CHECK(!hth_player_roster_get_slot(
        &roster, fixture.entities, fixture.actors, slot, &observed));
    CHECK(handle_is_invalid(observed));
    slot = 0U;
    CHECK(!hth_player_roster_find_entity(
        &roster, fixture.entities, fixture.actors, actor_lost, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(hth_player_roster_unregister(&roster, 0U));

    CHECK(create_actor(&fixture, &stale));
    CHECK(hth_player_roster_register(
        &roster, fixture.entities, fixture.actors, stale, &slot));
    CHECK(slot == 0U);
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(replacement.index == stale.index);
    CHECK(replacement.generation != stale.generation);
    CHECK(hth_actor_store_attach(
        fixture.actors, fixture.entities, replacement));
    CHECK(hth_player_roster_count(&roster) == 1U);
    CHECK(!hth_player_roster_get_slot(
        &roster, fixture.entities, fixture.actors, 0U, &observed));
    CHECK(handle_is_invalid(observed));
    slot = 0U;
    CHECK(!hth_player_roster_find_entity(
        &roster, fixture.entities, fixture.actors, replacement, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(hth_player_roster_unregister(&roster, 0U));
    CHECK(hth_player_roster_count(&roster) == 0U);
    CHECK(hth_player_roster_register(
        &roster, fixture.entities, fixture.actors, replacement, &slot));
    CHECK(slot == 0U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_reset_copy_and_multiple_rosters(void)
{
    HTHPlayerRoster original = {0};
    HTHPlayerRoster copy;
    HTHPlayerRoster rosters[HTH_MAX_PLAYERS] = {0};
    Fixture fixture;
    HTHEntityHandle entities[HTH_MAX_PLAYERS];
    HTHPlayerSlot slot;
    HTHPlayerSlot index;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        CHECK(create_actor(&fixture, &entities[index]));
    }
    CHECK(hth_player_roster_register(
        &original, fixture.entities, fixture.actors, entities[0], &slot));
    CHECK(hth_player_roster_register(
        &original, fixture.entities, fixture.actors, entities[1], &slot));
    copy = original;
    CHECK(hth_player_roster_unregister(&copy, 0U));
    CHECK(hth_player_roster_count(&copy) == 1U);
    CHECK(hth_player_roster_count(&original) == 2U);
    hth_player_roster_reset(&copy);
    CHECK(hth_player_roster_count(&copy) == 0U);
    CHECK(hth_player_roster_count(&original) == 2U);

    hth_player_roster_reset(&original);
    CHECK(hth_player_roster_count(&original) == 0U);
    CHECK(hth_entity_registry_is_alive(fixture.entities, entities[0]));
    CHECK(hth_actor_store_has(
        fixture.actors, fixture.entities, entities[0]));
    CHECK(hth_entity_registry_is_alive(fixture.entities, entities[1]));
    CHECK(hth_actor_store_has(
        fixture.actors, fixture.entities, entities[1]));

    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        CHECK(hth_player_roster_register(
            &rosters[index], fixture.entities, fixture.actors,
            entities[index], &slot));
        CHECK(slot == 0U);
    }
    CHECK(hth_player_roster_unregister(&rosters[2], 0U));
    CHECK(hth_player_roster_count(&rosters[2]) == 0U);
    CHECK(hth_player_roster_count(&rosters[0]) == 1U);
    CHECK(hth_player_roster_count(&rosters[1]) == 1U);
    CHECK(hth_player_roster_count(&rosters[3]) == 1U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_malformed_state_validation_and_recovery(void)
{
    HTHPlayerRoster duplicate = {0};
    HTHPlayerRoster impossible = {0};
    HTHPlayerRoster snapshot;
    Fixture fixture;
    HTHEntityHandle entity;
    HTHEntityHandle other;
    HTHEntityHandle observed = {3U, 7U};
    HTHPlayerSlot slot = 0U;

    CHECK(fixture_create(&fixture));
    CHECK(create_actor(&fixture, &entity));
    CHECK(create_actor(&fixture, &other));
    duplicate.entries[0].entity = entity;
    duplicate.entries[0].occupied = true;
    duplicate.entries[1].entity = entity;
    duplicate.entries[1].occupied = true;
    snapshot = duplicate;
    CHECK(!hth_player_roster_register(
        &duplicate, fixture.entities, fixture.actors, other, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(roster_equal(&duplicate, &snapshot));
    CHECK(!hth_player_roster_get_slot(
        &duplicate, fixture.entities, fixture.actors, 0U, &observed));
    CHECK(handle_is_invalid(observed));
    slot = 0U;
    CHECK(!hth_player_roster_find_entity(
        &duplicate, fixture.entities, fixture.actors, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    hth_player_roster_reset(&duplicate);
    CHECK(hth_player_roster_count(&duplicate) == 0U);

    impossible.entries[0].entity = (HTHEntityHandle){UINT32_MAX, 1U};
    impossible.entries[0].occupied = true;
    snapshot = impossible;
    slot = 0U;
    CHECK(!hth_player_roster_register(
        &impossible, fixture.entities, fixture.actors, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(roster_equal(&impossible, &snapshot));
    CHECK(!hth_player_roster_get_slot(
        &impossible, fixture.entities, fixture.actors, 0U, &observed));
    CHECK(handle_is_invalid(observed));
    CHECK(!hth_player_roster_find_entity(
        &impossible, fixture.entities, fixture.actors, entity, &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    hth_player_roster_reset(&impossible);
    CHECK(hth_player_roster_count(&impossible) == 0U);

    impossible.entries[0].entity = (HTHEntityHandle){0U, 0U};
    impossible.entries[0].occupied = true;
    CHECK(!hth_player_roster_get_slot(
        &impossible, fixture.entities, fixture.actors, 0U, &observed));
    hth_player_roster_reset(&impossible);
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_zero_initialization_reset_and_invalid_inputs,
        test_registration_role_and_transactionality,
        test_capacity_sparse_slots_and_reuse,
        test_stale_actor_loss_and_generation_reuse,
        test_reset_copy_and_multiple_rosters,
        test_malformed_state_validation_and_recovery
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("player role roster tests passed");
    return EXIT_SUCCESS;
}
