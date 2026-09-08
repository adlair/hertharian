#include "player_revive_execution.h"

#include "player_death.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
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

typedef struct Fixture {
    HTHEntityRegistry *entities;
    HTHActorStore *actors;
    HTHHealthStore *health;
    HTHPlayerRoster roster;
    HTHEntityHandle reviver;
    HTHEntityHandle target;
    HTHPlayerSlot reviver_slot;
    HTHPlayerSlot target_slot;
    HTHPlayerDefeatState reviver_defeat;
    HTHPlayerDefeatState target_defeat;
    HTHPlayerReviveWindow target_window;
} Fixture;

typedef struct FixtureSnapshot {
    HTHPlayerRoster roster;
    HTHHealth reviver_health;
    HTHHealth target_health;
    HTHPlayerDefeatState reviver_defeat;
    HTHPlayerDefeatState target_defeat;
    HTHPlayerReviveWindow target_window;
} FixtureSnapshot;

static bool fixture_create_empty(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->health = hth_health_store_create();
    fixture->reviver_slot = HTH_PLAYER_SLOT_INVALID;
    fixture->target_slot = HTH_PLAYER_SLOT_INVALID;
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->health != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_health_store_destroy(fixture->health);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
    *fixture = (Fixture){0};
}

static bool add_player(Fixture *fixture, float current, bool with_health,
                       HTHEntityHandle *out_entity,
                       HTHPlayerSlot *out_slot)
{
    return hth_entity_registry_create_entity(fixture->entities,
                                              out_entity) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_entity) &&
           (!with_health ||
            hth_health_store_attach(
                fixture->health, fixture->entities, fixture->actors,
                *out_entity, (HTHHealth){current, 100.0F})) &&
           hth_player_roster_register(
               &fixture->roster, fixture->entities, fixture->actors,
               *out_entity, out_slot);
}

static bool fixture_create_pair(Fixture *fixture, float reviver_health,
                                bool reviver_has_health,
                                float target_health,
                                bool target_has_health,
                                bool window_active)
{
    if (!fixture_create_empty(fixture) ||
        !add_player(fixture, reviver_health, reviver_has_health,
                    &fixture->reviver, &fixture->reviver_slot) ||
        !add_player(fixture, target_health, target_has_health,
                    &fixture->target, &fixture->target_slot) ||
        (window_active &&
         !hth_player_revive_window_begin(&fixture->target_window, 5.0))) {
        fixture_destroy(fixture);
        return false;
    }
    return true;
}

static bool execute(Fixture *fixture, float revive_health,
                    bool *out_revived)
{
    return hth_player_revive_execute(
        &fixture->roster, fixture->entities, fixture->actors,
        fixture->health, fixture->reviver_slot,
        &fixture->reviver_defeat, fixture->target_slot,
        &fixture->target_defeat, &fixture->target_window, revive_health,
        out_revived);
}

static bool health_get(const Fixture *fixture, HTHEntityHandle entity,
                       HTHHealth *out_health)
{
    return hth_health_store_get(fixture->health, fixture->entities,
                                fixture->actors, entity, out_health);
}

static bool take_snapshot(const Fixture *fixture,
                          FixtureSnapshot *snapshot)
{
    snapshot->roster = fixture->roster;
    snapshot->reviver_defeat = fixture->reviver_defeat;
    snapshot->target_defeat = fixture->target_defeat;
    snapshot->target_window = fixture->target_window;
    return health_get(fixture, fixture->reviver,
                      &snapshot->reviver_health) &&
           health_get(fixture, fixture->target, &snapshot->target_health);
}

static bool snapshot_matches(const Fixture *fixture,
                             const FixtureSnapshot *snapshot)
{
    HTHHealth reviver_health;
    HTHHealth target_health;

    return health_get(fixture, fixture->reviver, &reviver_health) &&
           health_get(fixture, fixture->target, &target_health) &&
           memcmp(&fixture->roster, &snapshot->roster,
                  sizeof(fixture->roster)) == 0 &&
           reviver_health.current == snapshot->reviver_health.current &&
           reviver_health.maximum == snapshot->reviver_health.maximum &&
           target_health.current == snapshot->target_health.current &&
           target_health.maximum == snapshot->target_health.maximum &&
           fixture->reviver_defeat.defeated ==
               snapshot->reviver_defeat.defeated &&
           fixture->target_defeat.defeated ==
               snapshot->target_defeat.defeated &&
           fixture->target_window.active == snapshot->target_window.active &&
           fixture->target_window.remaining_seconds ==
               snapshot->target_window.remaining_seconds;
}

static bool test_null_dependencies_and_amount_validation(void)
{
    Fixture fixture;
    FixtureSnapshot snapshot;
    const float invalid_amounts[] = {
        0.0F, -0.0F, -1.0F, NAN, INFINITY, -INFINITY
    };
    size_t index;
    bool revived;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    CHECK(take_snapshot(&fixture, &snapshot));

#define CHECK_FAILURE(call)                                                  \
    do {                                                                     \
        revived = true;                                                      \
        CHECK(!(call));                                                      \
        CHECK(!revived);                                                     \
        CHECK(snapshot_matches(&fixture, &snapshot));                        \
    } while (0)

    CHECK_FAILURE(hth_player_revive_execute(
        NULL, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, 25.0F, &revived));
    CHECK_FAILURE(hth_player_revive_execute(
        &fixture.roster, NULL, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, 25.0F, &revived));
    CHECK_FAILURE(hth_player_revive_execute(
        &fixture.roster, fixture.entities, NULL, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, 25.0F, &revived));
    CHECK_FAILURE(hth_player_revive_execute(
        &fixture.roster, fixture.entities, fixture.actors, NULL,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, 25.0F, &revived));
    CHECK_FAILURE(hth_player_revive_execute(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, NULL, fixture.target_slot,
        &fixture.target_defeat, &fixture.target_window, 25.0F, &revived));
    CHECK_FAILURE(hth_player_revive_execute(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, NULL, &fixture.target_window, 25.0F,
        &revived));
    CHECK_FAILURE(hth_player_revive_execute(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat, NULL, 25.0F,
        &revived));
    CHECK(!hth_player_revive_execute(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, 25.0F, NULL));
    CHECK(snapshot_matches(&fixture, &snapshot));

    for (index = 0U;
         index < sizeof(invalid_amounts) / sizeof(invalid_amounts[0]);
         ++index) {
        CHECK_FAILURE(execute(&fixture, invalid_amounts[index], &revived));
    }
    revived = true;
    CHECK(!hth_player_revive_execute(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.reviver_slot, &fixture.reviver_defeat,
        &fixture.target_window, NAN, &revived));
    CHECK(!revived && snapshot_matches(&fixture, &snapshot));
#undef CHECK_FAILURE

    fixture_destroy(&fixture);
    return true;
}

static bool test_slot_and_roster_failures(void)
{
    Fixture fixture;
    HTHPlayerRoster malformed;
    HTHPlayerRoster roster_before;
    bool revived = true;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    CHECK(!hth_player_revive_execute(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        HTH_PLAYER_SLOT_INVALID, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, 25.0F, &revived));
    CHECK(!revived);
    revived = true;
    CHECK(!hth_player_revive_execute(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        HTH_PLAYER_SLOT_INVALID, &fixture.target_defeat,
        &fixture.target_window, 25.0F, &revived));
    CHECK(!revived);

    CHECK(hth_player_roster_unregister(&fixture.roster,
                                       fixture.target_slot));
    roster_before = fixture.roster;
    revived = true;
    CHECK(!execute(&fixture, 25.0F, &revived));
    CHECK(!revived && memcmp(&fixture.roster, &roster_before,
                             sizeof(roster_before)) == 0);
    CHECK(hth_player_roster_register(
        &fixture.roster, fixture.entities, fixture.actors,
        fixture.target, &fixture.target_slot));

    malformed = fixture.roster;
    malformed.entries[2] = malformed.entries[0];
    roster_before = malformed;
    revived = true;
    CHECK(!hth_player_revive_execute(
        &malformed, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, 25.0F, &revived));
    CHECK(!revived && memcmp(&malformed, &roster_before,
                             sizeof(malformed)) == 0);
    fixture_destroy(&fixture);
    return true;
}

static bool current_player_failure(bool reviver, int failure_kind)
{
    Fixture fixture;
    HTHEntityHandle entity;
    HTHPlayerRoster roster_before;
    HTHPlayerReviveWindow window_before;
    bool revived = true;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    entity = reviver ? fixture.reviver : fixture.target;
    if (failure_kind == 0) {
        CHECK(hth_entity_registry_destroy_entity(fixture.entities, entity));
    } else if (failure_kind == 1) {
        CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                     entity));
    } else {
        CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                      fixture.actors, entity));
    }
    roster_before = fixture.roster;
    window_before = fixture.target_window;
    CHECK(!execute(&fixture, 25.0F, &revived));
    CHECK(!revived && memcmp(&fixture.roster, &roster_before,
                             sizeof(roster_before)) == 0 &&
          fixture.target_window.active == window_before.active &&
          fixture.target_window.remaining_seconds ==
              window_before.remaining_seconds);
    fixture_destroy(&fixture);
    return true;
}

static bool test_current_player_and_window_failures(void)
{
    Fixture fixture;
    HTHPlayerReviveWindow malformed;
    bool revived = true;

    CHECK(current_player_failure(true, 0));
    CHECK(current_player_failure(false, 0));
    CHECK(current_player_failure(true, 1));
    CHECK(current_player_failure(false, 1));
    CHECK(current_player_failure(true, 2));
    CHECK(current_player_failure(false, 2));

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    malformed = (HTHPlayerReviveWindow){0.0, true};
    fixture.target_window = malformed;
    CHECK(!execute(&fixture, 25.0F, &revived));
    CHECK(!revived && fixture.target_window.active == malformed.active &&
          fixture.target_window.remaining_seconds ==
              malformed.remaining_seconds);
    fixture_destroy(&fixture);
    return true;
}

static bool expect_noop(Fixture *fixture)
{
    FixtureSnapshot snapshot;
    bool revived = true;

    return take_snapshot(fixture, &snapshot) &&
           execute(fixture, 25.0F, &revived) && !revived &&
           snapshot_matches(fixture, &snapshot);
}

static bool test_policy_noops_and_determinism(void)
{
    Fixture fixture;
    FixtureSnapshot snapshot;
    size_t index;
    bool revived;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    CHECK(take_snapshot(&fixture, &snapshot));
    revived = true;
    CHECK(hth_player_revive_execute(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.reviver_slot, &fixture.reviver_defeat,
        &fixture.target_window, 25.0F, &revived));
    CHECK(!revived && snapshot_matches(&fixture, &snapshot));
    fixture_destroy(&fixture);

    CHECK(fixture_create_pair(&fixture, 0.0F, true, 0.0F, true, true));
    CHECK(expect_noop(&fixture));
    fixture_destroy(&fixture);

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    CHECK(hth_player_defeat_mark(&fixture.reviver_defeat));
    CHECK(expect_noop(&fixture));
    fixture_destroy(&fixture);

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 100.0F, true, true));
    CHECK(expect_noop(&fixture));
    fixture_destroy(&fixture);

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    CHECK(hth_player_defeat_mark(&fixture.target_defeat));
    CHECK(expect_noop(&fixture));
    fixture_destroy(&fixture);

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, false));
    CHECK(take_snapshot(&fixture, &snapshot));
    for (index = 0U; index < 128U; ++index) {
        revived = true;
        CHECK(execute(&fixture, 25.0F, &revived));
        CHECK(!revived && snapshot_matches(&fixture, &snapshot));
    }
    fixture_destroy(&fixture);
    return true;
}

static bool test_success_and_repeat(void)
{
    Fixture fixture;
    HTHPlayerRoster roster_before;
    HTHHealth reviver_before;
    HTHHealth reviver_after;
    HTHHealth target_health;
    double remaining;
    bool active;
    bool dead;
    bool revived;

    CHECK(fixture_create_pair(&fixture, 80.0F, true, 0.0F, true, true));
    roster_before = fixture.roster;
    CHECK(health_get(&fixture, fixture.reviver, &reviver_before));
    CHECK(execute(&fixture, 25.0F, &revived));
    CHECK(revived);
    CHECK(health_get(&fixture, fixture.target, &target_health));
    CHECK(target_health.current == 25.0F &&
          target_health.maximum == 100.0F);
    CHECK(hth_player_death_is_dead(
        fixture.entities, fixture.actors, fixture.health,
        fixture.target, &dead));
    CHECK(!dead);
    CHECK(hth_player_revive_window_query(
        &fixture.target_window, &active, &remaining));
    CHECK(!active && remaining == 0.0);
    CHECK(!fixture.reviver_defeat.defeated &&
          !fixture.target_defeat.defeated);
    CHECK(memcmp(&fixture.roster, &roster_before,
                 sizeof(roster_before)) == 0);
    CHECK(health_get(&fixture, fixture.reviver, &reviver_after));
    CHECK(reviver_before.current == reviver_after.current &&
          reviver_before.maximum == reviver_after.maximum);
    CHECK(hth_entity_registry_is_alive(fixture.entities,
                                       fixture.reviver));
    CHECK(hth_entity_registry_is_alive(fixture.entities, fixture.target));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                              fixture.reviver));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                              fixture.target));

    revived = true;
    CHECK(execute(&fixture, 25.0F, &revived));
    CHECK(!revived);
    CHECK(health_get(&fixture, fixture.target, &target_health));
    CHECK(target_health.current == 25.0F);
    fixture_destroy(&fixture);
    return true;
}

static bool success_with_amount(float amount, float expected_health)
{
    Fixture fixture;
    HTHHealth target_health;
    double remaining;
    bool active;
    bool dead;
    bool revived = false;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    CHECK(execute(&fixture, amount, &revived));
    CHECK(revived);
    CHECK(health_get(&fixture, fixture.target, &target_health));
    CHECK(target_health.current == expected_health);
    CHECK(hth_player_death_is_dead(
        fixture.entities, fixture.actors, fixture.health,
        fixture.target, &dead));
    CHECK(!dead);
    CHECK(hth_player_revive_window_query(
        &fixture.target_window, &active, &remaining));
    CHECK(!active && remaining == 0.0);
    fixture_destroy(&fixture);
    return true;
}

static bool test_small_positive_and_overheal(void)
{
    CHECK(success_with_amount(FLT_MIN, FLT_MIN));
    CHECK(success_with_amount(1000.0F, 100.0F));
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_null_dependencies_and_amount_validation,
        test_slot_and_roster_failures,
        test_current_player_and_window_failures,
        test_policy_noops_and_determinism,
        test_success_and_repeat,
        test_small_positive_and_overheal
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("player revive execution tests passed");
    return EXIT_SUCCESS;
}
