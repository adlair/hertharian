#include "player_revive_eligibility.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
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

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->health = hth_health_store_create();
    if (fixture->entities == NULL || fixture->actors == NULL ||
        fixture->health == NULL) {
        hth_health_store_destroy(fixture->health);
        hth_actor_store_destroy(fixture->actors);
        hth_entity_registry_destroy(fixture->entities);
        *fixture = (Fixture){0};
        return false;
    }
    fixture->reviver_slot = HTH_PLAYER_SLOT_INVALID;
    fixture->target_slot = HTH_PLAYER_SLOT_INVALID;
    return true;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_health_store_destroy(fixture->health);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
    *fixture = (Fixture){0};
}

static bool add_player(Fixture *fixture, HTHHealth health,
                       bool attach_health, HTHEntityHandle *out_entity,
                       HTHPlayerSlot *out_slot)
{
    return hth_entity_registry_create_entity(fixture->entities,
                                              out_entity) &&
           hth_actor_store_attach(fixture->actors, fixture->entities,
                                  *out_entity) &&
           (!attach_health ||
            hth_health_store_attach(fixture->health, fixture->entities,
                                    fixture->actors, *out_entity, health)) &&
           hth_player_roster_register(&fixture->roster, fixture->entities,
                                      fixture->actors, *out_entity,
                                      out_slot);
}

static bool fixture_create_pair(Fixture *fixture, float reviver_current,
                                bool reviver_has_health,
                                float target_current,
                                bool target_has_health,
                                bool window_active)
{
    if (!fixture_create(fixture) ||
        !add_player(fixture, (HTHHealth){reviver_current, 100.0F},
                    reviver_has_health, &fixture->reviver,
                    &fixture->reviver_slot) ||
        !add_player(fixture, (HTHHealth){target_current, 100.0F},
                    target_has_health, &fixture->target,
                    &fixture->target_slot)) {
        fixture_destroy(fixture);
        return false;
    }
    if (window_active &&
        !hth_player_revive_window_begin(&fixture->target_window, 5.0)) {
        fixture_destroy(fixture);
        return false;
    }
    return true;
}

static bool evaluate(const Fixture *fixture, bool *out_eligible)
{
    return hth_player_revive_eligibility_evaluate(
        &fixture->roster, fixture->entities, fixture->actors,
        fixture->health, fixture->reviver_slot,
        &fixture->reviver_defeat, fixture->target_slot,
        &fixture->target_defeat, &fixture->target_window, out_eligible);
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

static bool set_health_current(Fixture *fixture, HTHEntityHandle entity,
                               float desired)
{
    HTHHealth current;

    if (!hth_health_store_get(fixture->health, fixture->entities,
                              fixture->actors, entity, &current)) {
        return false;
    }
    if (desired < current.current) {
        HTHDamageResult result;

        return hth_health_store_apply_damage(
            fixture->health, fixture->entities, fixture->actors, entity,
            current.current - desired, &result);
    }
    if (desired > current.current) {
        HTHHealingResult result;

        return hth_health_store_apply_healing(
            fixture->health, fixture->entities, fixture->actors, entity,
            desired - current.current, &result);
    }
    return true;
}

static bool expect_policy(const Fixture *fixture, bool expected)
{
    bool eligible = !expected;

    return evaluate(fixture, &eligible) && eligible == expected;
}

static bool test_positive_and_null_dependencies(void)
{
    Fixture fixture;
    bool eligible;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    CHECK(expect_policy(&fixture, true));

#define CHECK_NULL_FAILURE(call)                                             \
    do {                                                                     \
        eligible = true;                                                     \
        CHECK(!(call));                                                      \
        CHECK(!eligible);                                                    \
    } while (0)

    CHECK_NULL_FAILURE(hth_player_revive_eligibility_evaluate(
        NULL, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, &eligible));
    CHECK_NULL_FAILURE(hth_player_revive_eligibility_evaluate(
        &fixture.roster, NULL, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, &eligible));
    CHECK_NULL_FAILURE(hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, NULL, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, &eligible));
    CHECK_NULL_FAILURE(hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, NULL,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, &eligible));
    CHECK_NULL_FAILURE(hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, NULL, fixture.target_slot,
        &fixture.target_defeat, &fixture.target_window, &eligible));
    CHECK_NULL_FAILURE(hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, NULL, &fixture.target_window, &eligible));
    CHECK_NULL_FAILURE(hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat, NULL, &eligible));
    CHECK(!hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, NULL));
#undef CHECK_NULL_FAILURE

    fixture_destroy(&fixture);
    return true;
}

static bool test_slot_roster_and_generic_actor_validation(void)
{
    Fixture fixture;
    HTHPlayerRoster empty = {0};
    HTHPlayerRoster malformed;
    HTHPlayerRoster malformed_snapshot;
    HTHEntityHandle generic_actor;
    HTHPlayerSlot found = 0U;
    bool eligible = true;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    CHECK(!hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        HTH_PLAYER_SLOT_INVALID, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, &eligible));
    CHECK(!eligible);
    CHECK(hth_player_roster_unregister(&fixture.roster,
                                       fixture.target_slot));
    eligible = true;
    CHECK(!hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, &eligible));
    CHECK(!eligible);
    CHECK(hth_player_roster_register(
        &fixture.roster, fixture.entities, fixture.actors, fixture.target,
        &fixture.target_slot));
    eligible = true;
    CHECK(!hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        HTH_PLAYER_SLOT_INVALID, &fixture.target_defeat,
        &fixture.target_window, &eligible));
    CHECK(!eligible);
    eligible = true;
    CHECK(!hth_player_revive_eligibility_evaluate(
        &empty, fixture.entities, fixture.actors, fixture.health,
        0U, &fixture.reviver_defeat, 1U, &fixture.target_defeat,
        &fixture.target_window, &eligible));
    CHECK(!eligible);

    malformed = fixture.roster;
    malformed.entries[2] = malformed.entries[0];
    malformed_snapshot = malformed;
    eligible = true;
    CHECK(!hth_player_revive_eligibility_evaluate(
        &malformed, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.target_slot, &fixture.target_defeat,
        &fixture.target_window, &eligible));
    CHECK(!eligible);
    CHECK(roster_equal(&malformed, &malformed_snapshot));

    CHECK(hth_entity_registry_create_entity(fixture.entities,
                                             &generic_actor));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 generic_actor));
    CHECK(!hth_player_roster_find_entity(
        &fixture.roster, fixture.entities, fixture.actors,
        generic_actor, &found));
    CHECK(found == HTH_PLAYER_SLOT_INVALID);
    CHECK(hth_player_roster_count(&fixture.roster) == 2U);
    fixture_destroy(&fixture);
    return true;
}

static bool stale_case(bool stale_reviver)
{
    Fixture fixture;
    HTHEntityHandle entity;
    bool eligible = true;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    entity = stale_reviver ? fixture.reviver : fixture.target;
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, entity));
    CHECK(!evaluate(&fixture, &eligible));
    CHECK(!eligible && hth_player_roster_count(&fixture.roster) == 2U);
    fixture_destroy(&fixture);
    return true;
}

static bool actor_removed_case(bool remove_reviver)
{
    Fixture fixture;
    HTHEntityHandle entity;
    bool eligible = true;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    entity = remove_reviver ? fixture.reviver : fixture.target;
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, entity));
    CHECK(!evaluate(&fixture, &eligible));
    CHECK(!eligible && hth_player_roster_count(&fixture.roster) == 2U);
    fixture_destroy(&fixture);
    return true;
}

static bool missing_health_case(bool missing_reviver)
{
    Fixture fixture;
    bool eligible = true;

    CHECK(fixture_create_pair(
        &fixture, 100.0F, !missing_reviver, 0.0F, missing_reviver, true));
    CHECK(!evaluate(&fixture, &eligible));
    CHECK(!eligible && hth_player_roster_count(&fixture.roster) == 2U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_current_player_and_generation_validation(void)
{
    Fixture fixture;
    HTHEntityHandle stale_target;
    HTHEntityHandle replacement;
    HTHPlayerSlot found = 0U;
    bool eligible = true;

    CHECK(stale_case(true));
    CHECK(stale_case(false));
    CHECK(actor_removed_case(true));
    CHECK(actor_removed_case(false));
    CHECK(missing_health_case(true));
    CHECK(missing_health_case(false));

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    stale_target = fixture.target;
    CHECK(hth_health_store_remove(fixture.health, fixture.entities,
                                  fixture.actors, stale_target));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities,
                                 stale_target));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities,
                                              stale_target));
    CHECK(hth_entity_registry_create_entity(fixture.entities,
                                             &replacement));
    CHECK(replacement.index == stale_target.index);
    CHECK(replacement.generation != stale_target.generation);
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities,
                                 replacement));
    CHECK(hth_health_store_attach(
        fixture.health, fixture.entities, fixture.actors, replacement,
        (HTHHealth){0.0F, 100.0F}));
    CHECK(!evaluate(&fixture, &eligible));
    CHECK(!eligible);
    CHECK(!hth_player_roster_find_entity(
        &fixture.roster, fixture.entities, fixture.actors,
        replacement, &found));
    CHECK(found == HTH_PLAYER_SLOT_INVALID);
    fixture_destroy(&fixture);
    return true;
}

static bool test_malformed_window_and_validation_first(void)
{
    Fixture fixture;
    const HTHPlayerReviveWindow malformed[] = {
        {0.0, true},
        {-1.0, true},
        {INFINITY, true},
        {-INFINITY, true},
        {1.0, false},
        {-1.0, false},
        {NAN, true},
        {NAN, false},
        {INFINITY, false},
        {-INFINITY, false}
    };
    HTHPlayerReviveWindow valid_window;
    size_t index;
    bool eligible;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    valid_window = fixture.target_window;
    for (index = 0U; index < sizeof(malformed) / sizeof(malformed[0]);
         ++index) {
        fixture.target_window = malformed[index];
        eligible = true;
        CHECK(!evaluate(&fixture, &eligible));
        CHECK(!eligible);
    }

    fixture.target_window = valid_window;
    eligible = true;
    CHECK(hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.reviver_slot, &fixture.reviver_defeat,
        &fixture.target_window, &eligible));
    CHECK(!eligible);

    fixture.target_window = malformed[0];
    eligible = true;
    CHECK(!hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.reviver_slot, &fixture.reviver_defeat,
        &fixture.target_window, &eligible));
    CHECK(!eligible);
    CHECK(hth_player_defeat_mark(&fixture.reviver_defeat));
    eligible = true;
    CHECK(!evaluate(&fixture, &eligible));
    CHECK(!eligible);
    fixture.target_window = valid_window;
    CHECK(set_health_current(&fixture, fixture.target, 100.0F));
    eligible = true;
    CHECK(!hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, NULL, fixture.target_slot,
        &fixture.target_defeat, &fixture.target_window, &eligible));
    CHECK(!eligible);
    fixture_destroy(&fixture);

    CHECK(fixture_create_pair(&fixture, 100.0F, false,
                              0.0F, true, true));
    eligible = true;
    CHECK(!hth_player_revive_eligibility_evaluate(
        &fixture.roster, fixture.entities, fixture.actors, fixture.health,
        fixture.reviver_slot, &fixture.reviver_defeat,
        fixture.reviver_slot, &fixture.reviver_defeat,
        &fixture.target_window, &eligible));
    CHECK(!eligible);
    fixture_destroy(&fixture);
    return true;
}

static bool test_policy_matrix(void)
{
    Fixture fixture;
    bool expired;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    CHECK(expect_policy(&fixture, true));

    CHECK(set_health_current(&fixture, fixture.reviver, 0.0F));
    CHECK(expect_policy(&fixture, false));
    CHECK(hth_player_defeat_mark(&fixture.reviver_defeat));
    CHECK(expect_policy(&fixture, false));
    CHECK(set_health_current(&fixture, fixture.reviver, 100.0F));
    CHECK(expect_policy(&fixture, false));
    hth_player_defeat_reset(&fixture.reviver_defeat);

    CHECK(set_health_current(&fixture, fixture.target, 100.0F));
    CHECK(expect_policy(&fixture, false));
    hth_player_revive_window_reset(&fixture.target_window);
    CHECK(expect_policy(&fixture, false));

    CHECK(set_health_current(&fixture, fixture.target, 0.0F));
    CHECK(expect_policy(&fixture, false));
    CHECK(hth_player_defeat_mark(&fixture.target_defeat));
    CHECK(expect_policy(&fixture, false));
    CHECK(hth_player_revive_window_begin(&fixture.target_window, 2.0));
    CHECK(expect_policy(&fixture, false));

    hth_player_defeat_reset(&fixture.target_defeat);
    CHECK(set_health_current(&fixture, fixture.reviver, 0.0F));
    CHECK(expect_policy(&fixture, false));
    CHECK(set_health_current(&fixture, fixture.reviver, 100.0F));
    CHECK(expect_policy(&fixture, true));

    CHECK(hth_player_revive_window_advance(
        &fixture.target_window, 2.0, &expired));
    CHECK(expired && expect_policy(&fixture, false));
    CHECK(hth_player_revive_window_begin(&fixture.target_window, 0.25));
    CHECK(expect_policy(&fixture, true));
    hth_player_revive_window_reset(&fixture.target_window);
    CHECK(hth_player_revive_window_begin(&fixture.target_window, 30.0));
    CHECK(expect_policy(&fixture, true));
    fixture_destroy(&fixture);
    return true;
}

static bool test_purity_and_determinism(void)
{
    Fixture fixture;
    HTHPlayerRoster roster_snapshot;
    HTHPlayerDefeatState reviver_defeat_snapshot;
    HTHPlayerDefeatState target_defeat_snapshot;
    HTHPlayerReviveWindow window_snapshot;
    HTHHealth reviver_health_before;
    HTHHealth target_health_before;
    HTHHealth reviver_health_after;
    HTHHealth target_health_after;
    size_t index;

    CHECK(fixture_create_pair(&fixture, 100.0F, true, 0.0F, true, true));
    roster_snapshot = fixture.roster;
    reviver_defeat_snapshot = fixture.reviver_defeat;
    target_defeat_snapshot = fixture.target_defeat;
    window_snapshot = fixture.target_window;
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, fixture.reviver,
                               &reviver_health_before));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, fixture.target,
                               &target_health_before));
    for (index = 0U; index < 128U; ++index) {
        CHECK(expect_policy(&fixture, true));
    }
    CHECK(roster_equal(&fixture.roster, &roster_snapshot));
    CHECK(fixture.reviver_defeat.defeated ==
          reviver_defeat_snapshot.defeated);
    CHECK(fixture.target_defeat.defeated == target_defeat_snapshot.defeated);
    CHECK(fixture.target_window.active == window_snapshot.active);
    CHECK(fixture.target_window.remaining_seconds ==
          window_snapshot.remaining_seconds);
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, fixture.reviver,
                               &reviver_health_after));
    CHECK(hth_health_store_get(fixture.health, fixture.entities,
                               fixture.actors, fixture.target,
                               &target_health_after));
    CHECK(reviver_health_before.current == reviver_health_after.current);
    CHECK(reviver_health_before.maximum == reviver_health_after.maximum);
    CHECK(target_health_before.current == target_health_after.current);
    CHECK(target_health_before.maximum == target_health_after.maximum);
    CHECK(hth_entity_registry_is_alive(fixture.entities, fixture.reviver));
    CHECK(hth_entity_registry_is_alive(fixture.entities, fixture.target));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                              fixture.reviver));
    CHECK(hth_actor_store_has(fixture.actors, fixture.entities,
                              fixture.target));

    CHECK(hth_player_defeat_mark(&fixture.reviver_defeat));
    for (index = 0U; index < 128U; ++index) {
        CHECK(expect_policy(&fixture, false));
    }
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_positive_and_null_dependencies,
        test_slot_roster_and_generic_actor_validation,
        test_current_player_and_generation_validation,
        test_malformed_window_and_validation_first,
        test_policy_matrix,
        test_purity_and_determinism
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("player revive eligibility tests passed");
    return EXIT_SUCCESS;
}
