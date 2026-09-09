#include "player_death_snapshot.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
    HTHHealthStore *health;
    HTHPlayerRoster roster;
} Fixture;

static size_t death_query_count;

bool __real_hth_player_death_is_dead(const HTHEntityRegistry *entities,
                                     const HTHActorStore *actors,
                                     const HTHHealthStore *health,
                                     HTHEntityHandle player,
                                     bool *out_dead);

bool __wrap_hth_player_death_is_dead(const HTHEntityRegistry *entities,
                                     const HTHActorStore *actors,
                                     const HTHHealthStore *health,
                                     HTHEntityHandle player,
                                     bool *out_dead)
{
    death_query_count++;
    return __real_hth_player_death_is_dead(entities, actors, health, player,
                                           out_dead);
}

static bool fixture_init(Fixture *fixture)
{
    memset(fixture, 0, sizeof(*fixture));
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->health = hth_health_store_create();
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->health != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_health_store_destroy(fixture->health);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static bool create_player(Fixture *fixture, float current,
                          HTHEntityHandle *out_entity,
                          HTHPlayerSlot *out_slot)
{
    const HTHHealth health = {current, 100.0F};

    return hth_entity_registry_create_entity(fixture->entities, out_entity) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_entity) &&
           hth_health_store_attach(fixture->health, fixture->entities,
                                   fixture->actors, *out_entity, health) &&
           hth_player_roster_register(&fixture->roster, fixture->entities,
                                      fixture->actors, *out_entity, out_slot);
}

static bool snapshot_is_empty(const HTHPlayerDeathSnapshot *snapshot)
{
    HTHPlayerSlot slot;

    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        const HTHPlayerDeathSnapshotEntry *entry = &snapshot->entries[slot];

        if (entry->entity.index != 0U || entry->entity.generation != 0U ||
            entry->present || entry->dead) {
            return false;
        }
    }
    return true;
}

static bool snapshots_equal(const HTHPlayerDeathSnapshot *left,
                            const HTHPlayerDeathSnapshot *right)
{
    HTHPlayerSlot slot;

    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        if (!hth_entity_handle_equal(left->entries[slot].entity,
                                     right->entries[slot].entity) ||
            left->entries[slot].present != right->entries[slot].present ||
            left->entries[slot].dead != right->entries[slot].dead) {
            return false;
        }
    }
    return true;
}

static void poison_snapshot(HTHPlayerDeathSnapshot *snapshot)
{
    memset(snapshot, 0xff, sizeof(*snapshot));
}

static bool query_matches(const HTHPlayerDeathSnapshot *snapshot,
                          HTHPlayerSlot slot,
                          HTHEntityHandle entity,
                          bool expected_dead)
{
    bool present = false;
    bool dead = !expected_dead;

    return hth_player_death_snapshot_query(snapshot, slot, entity,
                                            &present, &dead) &&
           present && dead == expected_dead;
}

static bool test_zero_snapshot_and_query_contract(void)
{
    HTHPlayerDeathSnapshot snapshot = {0};
    const HTHEntityHandle first = {0U, 1U};
    const HTHEntityHandle second = {1U, 1U};
    const HTHEntityHandle invalid = hth_entity_handle_invalid();
    bool present = true;
    bool dead = true;
    size_t iteration;

    CHECK(snapshot_is_empty(&snapshot));
    CHECK(hth_player_death_snapshot_query(&snapshot, 0U, first,
                                           &present, &dead));
    CHECK(!present && !dead);

    snapshot.entries[2].entity = first;
    snapshot.entries[2].present = true;
    CHECK(query_matches(&snapshot, 2U, first, false));
    snapshot.entries[2].dead = true;
    CHECK(query_matches(&snapshot, 2U, first, true));
    present = true;
    dead = true;
    CHECK(hth_player_death_snapshot_query(&snapshot, 2U, second,
                                           &present, &dead));
    CHECK(!present && !dead);

    CHECK(!hth_player_death_snapshot_query(NULL, 0U, first,
                                            &present, &dead));
    CHECK(!present && !dead);
    dead = true;
    CHECK(!hth_player_death_snapshot_query(&snapshot, 0U, first,
                                            NULL, &dead));
    CHECK(!dead);
    present = true;
    CHECK(!hth_player_death_snapshot_query(&snapshot, 0U, first,
                                            &present, NULL));
    CHECK(!present);
    present = true;
    dead = true;
    CHECK(!hth_player_death_snapshot_query(
        &snapshot, HTH_PLAYER_SLOT_INVALID, first, &present, &dead));
    CHECK(!present && !dead);
    present = true;
    dead = true;
    CHECK(!hth_player_death_snapshot_query(
        &snapshot, HTH_MAX_PLAYERS + 1U, first, &present, &dead));
    CHECK(!present && !dead);
    present = true;
    dead = true;
    CHECK(!hth_player_death_snapshot_query(&snapshot, 0U, invalid,
                                            &present, &dead));
    CHECK(!present && !dead);
    present = true;
    dead = true;
    CHECK(!hth_player_death_snapshot_query(
        &snapshot, 0U, (HTHEntityHandle){0U, 0U}, &present, &dead));
    CHECK(!present && !dead);

    for (iteration = 0U; iteration < 128U; ++iteration) {
        CHECK(query_matches(&snapshot, 2U, first, true));
    }
    return true;
}

static bool test_build_nulls_and_empty_roster(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;

    CHECK(fixture_init(&fixture));
#define CHECK_EMPTY_FAILURE(call)                                            \
    do {                                                                     \
        poison_snapshot(&snapshot);                                          \
        CHECK(!(call));                                                      \
        CHECK(snapshot_is_empty(&snapshot));                                 \
    } while (0)
    CHECK_EMPTY_FAILURE(hth_player_death_snapshot_build(
        NULL, fixture.entities, fixture.actors, fixture.health, &snapshot));
    CHECK_EMPTY_FAILURE(hth_player_death_snapshot_build(
        &fixture.roster, NULL, fixture.actors, fixture.health, &snapshot));
    CHECK_EMPTY_FAILURE(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, NULL, fixture.health, &snapshot));
    CHECK_EMPTY_FAILURE(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, NULL, &snapshot));
#undef CHECK_EMPTY_FAILURE
    CHECK(!hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        NULL));

    death_query_count = 0U;
    poison_snapshot(&snapshot);
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(snapshot_is_empty(&snapshot));
    CHECK(death_query_count == 0U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_one_alive_and_one_dead_player(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;
    HTHEntityHandle player;
    HTHPlayerSlot slot;

    CHECK(fixture_init(&fixture));
    CHECK(create_player(&fixture, 25.0F, &player, &slot));
    CHECK(slot == 0U);
    death_query_count = 0U;
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(death_query_count == 1U);
    CHECK(query_matches(&snapshot, slot, player, false));

    CHECK(hth_player_roster_unregister(&fixture.roster, slot));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, player));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, player));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, player));
    CHECK(create_player(&fixture, 0.0F, &player, &slot));
    death_query_count = 0U;
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(death_query_count == 1U);
    CHECK(query_matches(&snapshot, slot, player, true));
    fixture_destroy(&fixture);
    return true;
}

static bool test_four_players_and_sparse_slots(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;
    HTHEntityHandle players[HTH_MAX_PLAYERS];
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    const float current[HTH_MAX_PLAYERS] = {10.0F, 0.0F, 20.0F, 0.0F};
    HTHPlayerSlot slot;
    bool present = true;
    bool dead = true;

    CHECK(fixture_init(&fixture));
    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        CHECK(create_player(&fixture, current[slot], &players[slot],
                            &slots[slot]));
        CHECK(slots[slot] == slot);
    }
    death_query_count = 0U;
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(death_query_count == 4U);
    CHECK(query_matches(&snapshot, 0U, players[0], false));
    CHECK(query_matches(&snapshot, 1U, players[1], true));
    CHECK(query_matches(&snapshot, 2U, players[2], false));
    CHECK(query_matches(&snapshot, 3U, players[3], true));

    CHECK(hth_player_roster_unregister(&fixture.roster, 1U));
    death_query_count = 0U;
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(death_query_count == 3U);
    CHECK(snapshot.entries[0].present);
    CHECK(!snapshot.entries[1].present);
    CHECK(snapshot.entries[2].present);
    CHECK(snapshot.entries[3].present);
    CHECK(hth_player_death_snapshot_query(&snapshot, 1U, players[1],
                                           &present, &dead));
    CHECK(!present && !dead);
    fixture_destroy(&fixture);
    return true;
}

static bool test_transactional_failures(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;
    HTHEntityHandle players[3];
    HTHPlayerSlot slot;

    CHECK(fixture_init(&fixture));
    CHECK(create_player(&fixture, 10.0F, &players[0], &slot));
    CHECK(create_player(&fixture, 10.0F, &players[1], &slot));
    CHECK(create_player(&fixture, 10.0F, &players[2], &slot));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, players[2]));
    poison_snapshot(&snapshot);
    death_query_count = 0U;
    CHECK(!hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(death_query_count == 3U);
    CHECK(snapshot_is_empty(&snapshot));
    fixture_destroy(&fixture);

    CHECK(fixture_init(&fixture));
    CHECK(create_player(&fixture, 10.0F, &players[0], &slot));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 players[0]));
    poison_snapshot(&snapshot);
    death_query_count = 0U;
    CHECK(!hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(death_query_count == 0U);
    CHECK(snapshot_is_empty(&snapshot));
    fixture_destroy(&fixture);

    CHECK(fixture_init(&fixture));
    CHECK(create_player(&fixture, 10.0F, &players[0], &slot));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, players[0]));
    poison_snapshot(&snapshot);
    death_query_count = 0U;
    CHECK(!hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(death_query_count == 0U);
    CHECK(snapshot_is_empty(&snapshot));
    fixture_destroy(&fixture);
    return true;
}

static bool test_malformed_roster(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;
    HTHEntityHandle player;
    HTHPlayerSlot slot;

    CHECK(fixture_init(&fixture));
    CHECK(create_player(&fixture, 10.0F, &player, &slot));
    fixture.roster.entries[1] = fixture.roster.entries[0];
    poison_snapshot(&snapshot);
    death_query_count = 0U;
    CHECK(!hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(death_query_count == 0U);
    CHECK(snapshot_is_empty(&snapshot));

    hth_player_roster_reset(&fixture.roster);
    fixture.roster.entries[0].entity = (HTHEntityHandle){UINT32_MAX, 1U};
    fixture.roster.entries[0].occupied = true;
    poison_snapshot(&snapshot);
    CHECK(!hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &snapshot));
    CHECK(snapshot_is_empty(&snapshot));
    fixture_destroy(&fixture);
    return true;
}

static bool test_health_mutation_and_rebuild(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot alive_snapshot;
    HTHPlayerDeathSnapshot dead_snapshot;
    HTHPlayerDeathSnapshot healed_snapshot;
    HTHEntityHandle player;
    HTHPlayerSlot slot;
    HTHDamageResult damage;
    HTHHealingResult healing;

    CHECK(fixture_init(&fixture));
    CHECK(create_player(&fixture, 20.0F, &player, &slot));
    death_query_count = 0U;
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &alive_snapshot));
    CHECK(death_query_count == 1U);
    CHECK(hth_health_store_apply_damage(
        fixture.health, fixture.entities, fixture.actors, player, 20.0F,
        &damage));
    CHECK(query_matches(&alive_snapshot, slot, player, false));
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &dead_snapshot));
    CHECK(query_matches(&dead_snapshot, slot, player, true));

    CHECK(hth_health_store_apply_healing(
        fixture.health, fixture.entities, fixture.actors, player, 5.0F,
        &healing));
    CHECK(query_matches(&dead_snapshot, slot, player, true));
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &healed_snapshot));
    CHECK(query_matches(&healed_snapshot, slot, player, false));
    fixture_destroy(&fixture);
    return true;
}

static bool test_population_mutation_slot_and_generation_reuse(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot empty_snapshot;
    HTHPlayerDeathSnapshot old_snapshot;
    HTHPlayerDeathSnapshot new_snapshot;
    HTHEntityHandle first;
    HTHEntityHandle replacement;
    HTHPlayerSlot first_slot;
    HTHPlayerSlot replacement_slot;
    bool present = true;
    bool dead = true;

    CHECK(fixture_init(&fixture));
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &empty_snapshot));
    CHECK(create_player(&fixture, 0.0F, &first, &first_slot));
    CHECK(hth_player_death_snapshot_query(
        &empty_snapshot, first_slot, first, &present, &dead));
    CHECK(!present && !dead);
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &old_snapshot));
    CHECK(query_matches(&old_snapshot, first_slot, first, true));

    CHECK(hth_player_roster_unregister(&fixture.roster, first_slot));
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, first));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, first));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, first));
    CHECK(create_player(&fixture, 50.0F, &replacement, &replacement_slot));
    CHECK(replacement_slot == first_slot);
    CHECK(replacement.index == first.index);
    CHECK(replacement.generation != first.generation);
    present = true;
    dead = true;
    CHECK(hth_player_death_snapshot_query(
        &old_snapshot, replacement_slot, replacement, &present, &dead));
    CHECK(!present && !dead);
    CHECK(query_matches(&old_snapshot, first_slot, first, true));
    CHECK(hth_player_death_snapshot_build(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        &new_snapshot));
    CHECK(query_matches(&new_snapshot, replacement_slot, replacement, false));
    fixture_destroy(&fixture);
    return true;
}

static bool test_copy_determinism_and_source_immutability(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot first_snapshot;
    HTHPlayerDeathSnapshot copy;
    HTHPlayerDeathSnapshot rebuilt;
    HTHPlayerRoster roster_before;
    HTHEntityHandle player;
    HTHEntityHandle observed;
    HTHPlayerSlot slot;
    HTHHealth health_before;
    HTHHealth health_after;
    HTHDamageResult damage;
    size_t iteration;

    CHECK(fixture_init(&fixture));
    CHECK(create_player(&fixture, 75.0F, &player, &slot));
    roster_before = fixture.roster;
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, player, &health_before));
    death_query_count = 0U;
    for (iteration = 0U; iteration < 128U; ++iteration) {
        CHECK(hth_player_death_snapshot_build(
            &fixture.roster, fixture.entities, fixture.actors,
            fixture.health, &rebuilt));
        if (iteration == 0U) {
            first_snapshot = rebuilt;
        } else {
            CHECK(snapshots_equal(&first_snapshot, &rebuilt));
        }
    }
    CHECK(death_query_count == 128U);
    CHECK(memcmp(&fixture.roster, &roster_before,
                 sizeof(fixture.roster)) == 0);
    CHECK(hth_player_roster_get_slot(&fixture.roster, fixture.entities,
                                     fixture.actors, slot, &observed));
    CHECK(hth_entity_handle_equal(observed, player));
    CHECK(hth_entity_registry_is_alive(fixture.entities, player));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities, player));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, player, &health_after));
    CHECK(health_before.current == health_after.current);
    CHECK(health_before.maximum == health_after.maximum);

    copy = first_snapshot;
    CHECK(hth_health_store_apply_damage(
        fixture.health, fixture.entities, fixture.actors, player, 75.0F,
        &damage));
    CHECK(snapshots_equal(&copy, &first_snapshot));
    CHECK(query_matches(&copy, slot, player, false));
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_zero_snapshot_and_query_contract,
        test_build_nulls_and_empty_roster,
        test_one_alive_and_one_dead_player,
        test_four_players_and_sparse_slots,
        test_transactional_failures,
        test_malformed_roster,
        test_health_mutation_and_rebuild,
        test_population_mutation_slot_and_generation_reuse,
        test_copy_determinism_and_source_immutability,
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return 1;
        }
    }
    puts("player death snapshot tests passed");
    return 0;
}
