#include "player_lifecycle_runtime.h"

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

typedef struct Fixture {
    HTHEntityRegistry *entities;
    HTHActorStore *actors;
    HTHPlayerLifecycleRuntime runtime;
    HTHEntityHandle players[HTH_MAX_PLAYERS + 1U];
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    size_t player_count;
} Fixture;

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    return fixture->entities != NULL && fixture->actors != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
}

static bool add_player(Fixture *fixture, HTHPlayerSlot *out_slot)
{
    HTHEntityHandle entity;
    HTHPlayerSlot slot;

    if (fixture->player_count >= HTH_MAX_PLAYERS + 1U ||
        !hth_entity_registry_create_entity(fixture->entities, &entity) ||
        !hth_actor_store_attach(fixture->actors, fixture->entities, entity) ||
        !hth_player_lifecycle_runtime_register(
            &fixture->runtime, fixture->entities, fixture->actors, entity,
            &slot)) {
        return false;
    }
    fixture->players[fixture->player_count++] = entity;
    if (slot < HTH_MAX_PLAYERS) {
        fixture->slots[slot] = slot;
    }
    if (out_slot != NULL) {
        *out_slot = slot;
    }
    return true;
}

static bool query_defeat(const Fixture *fixture, HTHPlayerSlot slot,
                         bool expected)
{
    const HTHPlayerDefeatState *defeat;
    bool defeated = !expected;

    return hth_player_lifecycle_runtime_get_defeat(
               &fixture->runtime, fixture->entities, fixture->actors,
               slot, &defeat) &&
           hth_player_defeat_is_defeated(defeat, &defeated) &&
           defeated == expected;
}

static bool query_window(const Fixture *fixture, HTHPlayerSlot slot,
                         bool expected_active, double expected_remaining)
{
    const HTHPlayerReviveWindow *window;
    double remaining = -1.0;
    bool active = !expected_active;

    return hth_player_lifecycle_runtime_get_revive_window(
               &fixture->runtime, fixture->entities, fixture->actors,
               slot, &window) &&
           hth_player_revive_window_query(window, &active, &remaining) &&
           active == expected_active && remaining == expected_remaining;
}

static bool step(Fixture *fixture, HTHPlayerSlot slot, bool dead,
                 double delta_seconds, const double *duration_seconds)
{
    return hth_player_lifecycle_runtime_step_player(
        &fixture->runtime, fixture->entities, fixture->actors, slot, dead,
        delta_seconds, duration_seconds);
}

static bool test_zero_reset_and_accessors(void)
{
    HTHPlayerLifecycleRuntime runtime = {0};
    const HTHPlayerDefeatState *defeat = (const HTHPlayerDefeatState *)1;
    const HTHPlayerReviveWindow *window =
        (const HTHPlayerReviveWindow *)1;
    HTHPlayerSlot slot;

    CHECK(hth_player_lifecycle_runtime_get_roster(&runtime) ==
          &runtime.roster);
    CHECK(hth_player_lifecycle_runtime_get_roster(NULL) == NULL);
    CHECK(hth_player_roster_count(&runtime.roster) == 0U);
    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        bool defeated = true;
        bool active = true;
        double remaining = -1.0;

        CHECK(hth_player_defeat_is_defeated(
            &runtime.defeat[slot], &defeated));
        CHECK(!defeated && !runtime.was_dead[slot]);
        CHECK(hth_player_revive_window_query(
            &runtime.revive_windows[slot], &active, &remaining));
        CHECK(!active && remaining == 0.0);
    }
    CHECK(!hth_player_lifecycle_runtime_get_defeat(
        &runtime, NULL, NULL, 0U, &defeat));
    CHECK(defeat == NULL);
    CHECK(!hth_player_lifecycle_runtime_get_revive_window(
        &runtime, NULL, NULL, 0U, &window));
    CHECK(window == NULL);
    hth_player_lifecycle_runtime_reset(NULL);
    hth_player_lifecycle_runtime_reset(&runtime);
    CHECK(hth_player_roster_count(&runtime.roster) == 0U);
    return true;
}

static bool test_register_unregister_reset_and_reuse(void)
{
    Fixture fixture;
    HTHEntityHandle replacement;
    HTHPlayerSlot slot;
    HTHPlayerSlot reused;

    CHECK(fixture_create(&fixture));
    slot = 0U;
    CHECK(!hth_player_lifecycle_runtime_register(
        NULL, fixture.entities, fixture.actors,
        hth_entity_handle_invalid(), &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(add_player(&fixture, &slot));
    CHECK(slot == 0U && query_defeat(&fixture, slot, false));
    CHECK(query_window(&fixture, slot, false, 0.0));
    CHECK(!fixture.runtime.was_dead[slot]);

    CHECK(hth_player_defeat_mark(&fixture.runtime.defeat[slot]));
    CHECK(hth_player_revive_window_begin(
        &fixture.runtime.revive_windows[slot], 3.0));
    fixture.runtime.was_dead[slot] = true;
    CHECK(hth_player_lifecycle_runtime_unregister(&fixture.runtime, slot));
    CHECK(hth_player_roster_count(&fixture.runtime.roster) == 0U);
    CHECK(!fixture.runtime.defeat[slot].defeated);
    CHECK(!fixture.runtime.revive_windows[slot].active &&
          fixture.runtime.revive_windows[slot].remaining_seconds == 0.0);
    CHECK(!fixture.runtime.was_dead[slot]);
    CHECK(!hth_player_lifecycle_runtime_unregister(&fixture.runtime, slot));

    CHECK(hth_entity_registry_create_entity(fixture.entities, &replacement));
    CHECK(hth_actor_store_attach(
        fixture.actors, fixture.entities, replacement));
    CHECK(hth_player_lifecycle_runtime_register(
        &fixture.runtime, fixture.entities, fixture.actors, replacement,
        &reused));
    CHECK(reused == slot && query_defeat(&fixture, reused, false));
    CHECK(query_window(&fixture, reused, false, 0.0));
    CHECK(!fixture.runtime.was_dead[reused]);
    fixture_destroy(&fixture);
    return true;
}

static bool test_registration_failures_and_generation_safety(void)
{
    Fixture fixture;
    HTHPlayerLifecycleRuntime snapshot;
    HTHEntityHandle missing_actor;
    HTHEntityHandle stale;
    HTHEntityHandle replacement;
    HTHPlayerSlot slot = 0U;

    CHECK(fixture_create(&fixture));
    CHECK(hth_entity_registry_create_entity(
        fixture.entities, &missing_actor));
    snapshot = fixture.runtime;
    CHECK(!hth_player_lifecycle_runtime_register(
        &fixture.runtime, fixture.entities, fixture.actors, missing_actor,
        &slot));
    CHECK(slot == HTH_PLAYER_SLOT_INVALID);
    CHECK(hth_player_roster_count(&fixture.runtime.roster) ==
          hth_player_roster_count(&snapshot.roster));

    CHECK(hth_entity_registry_create_entity(fixture.entities, &stale));
    CHECK(hth_actor_store_attach(fixture.actors, fixture.entities, stale));
    CHECK(hth_actor_store_remove(fixture.actors, fixture.entities, stale));
    CHECK(hth_entity_registry_destroy_entity(fixture.entities, stale));
    CHECK(hth_entity_registry_create_entity(
        fixture.entities, &replacement));
    CHECK(replacement.index == stale.index &&
          replacement.generation != stale.generation);
    CHECK(hth_actor_store_attach(
        fixture.actors, fixture.entities, replacement));
    CHECK(!hth_player_lifecycle_runtime_register(
        &fixture.runtime, fixture.entities, fixture.actors, stale, &slot));
    CHECK(hth_player_lifecycle_runtime_register(
        &fixture.runtime, fixture.entities, fixture.actors, replacement,
        &slot));
    CHECK(slot == 0U);
    fixture_destroy(&fixture);
    return true;
}

static bool test_solo_lifecycle_and_defeat_persistence(void)
{
    Fixture fixture;
    HTHPlayerSlot slot;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, &slot));
    CHECK(step(&fixture, slot, false, 0.25, NULL));
    CHECK(!fixture.runtime.was_dead[slot]);
    CHECK(step(&fixture, slot, true, 0.25, NULL));
    CHECK(query_defeat(&fixture, slot, true));
    CHECK(query_window(&fixture, slot, false, 0.0));
    CHECK(fixture.runtime.was_dead[slot]);
    CHECK(step(&fixture, slot, true, 0.25, NULL));
    CHECK(query_defeat(&fixture, slot, true));

    CHECK(hth_player_revive_window_begin(
        &fixture.runtime.revive_windows[slot], 1.0));
    CHECK(step(&fixture, slot, false, 0.25, NULL));
    CHECK(query_defeat(&fixture, slot, true));
    CHECK(query_window(&fixture, slot, false, 0.0));
    CHECK(!fixture.runtime.was_dead[slot]);
    fixture_destroy(&fixture);
    return true;
}

static bool test_coop_begin_advance_expiry_and_inactive_resolution(void)
{
    Fixture fixture;
    HTHPlayerSlot target;
    HTHPlayerSlot teammate;
    const double duration = 2.0;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, &target));
    CHECK(add_player(&fixture, &teammate));
    CHECK(teammate != target);
    CHECK(step(&fixture, target, true, 0.5, &duration));
    CHECK(query_defeat(&fixture, target, false));
    CHECK(query_window(&fixture, target, true, duration));
    CHECK(fixture.runtime.was_dead[target]);
    CHECK(step(&fixture, target, true, 0.0, NULL));
    CHECK(query_window(&fixture, target, true, duration));
    CHECK(step(&fixture, target, true, 0.5, NULL));
    CHECK(query_window(&fixture, target, true, 1.5));
    CHECK(step(&fixture, target, true, 2.0, NULL));
    CHECK(query_defeat(&fixture, target, true));
    CHECK(query_window(&fixture, target, false, 0.0));

    hth_player_defeat_reset(&fixture.runtime.defeat[teammate]);
    fixture.runtime.was_dead[teammate] = true;
    CHECK(step(&fixture, teammate, true, 0.25, NULL));
    CHECK(query_defeat(&fixture, teammate, true));
    CHECK(query_window(&fixture, teammate, false, 0.0));
    fixture_destroy(&fixture);
    return true;
}

static bool test_invalid_delta_and_duration_retry(void)
{
    static const double invalid_deltas[] = {-1.0, NAN, INFINITY, -INFINITY};
    static const double invalid_durations[] = {
        0.0, -1.0, NAN, INFINITY, -INFINITY
    };
    Fixture fixture;
    HTHPlayerSlot target;
    HTHPlayerSlot teammate;
    const double valid_duration = 3.0;
    size_t index;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, &target));
    CHECK(add_player(&fixture, &teammate));
    CHECK(teammate != target);
    for (index = 0U;
         index < sizeof(invalid_deltas) / sizeof(invalid_deltas[0]);
         ++index) {
        CHECK(!step(&fixture, target, true, invalid_deltas[index],
                    &valid_duration));
        CHECK(!fixture.runtime.was_dead[target]);
        CHECK(query_defeat(&fixture, target, false));
        CHECK(query_window(&fixture, target, false, 0.0));
    }
    CHECK(!step(&fixture, target, true, 0.1, NULL));
    for (index = 0U;
         index < sizeof(invalid_durations) / sizeof(invalid_durations[0]);
         ++index) {
        CHECK(!step(&fixture, target, true, 0.1,
                    &invalid_durations[index]));
        CHECK(!fixture.runtime.was_dead[target]);
        CHECK(query_window(&fixture, target, false, 0.0));
    }
    CHECK(step(&fixture, target, true, 0.1, &valid_duration));
    CHECK(query_window(&fixture, target, true, valid_duration));
    fixture_destroy(&fixture);
    return true;
}

static bool test_recovery_and_second_episode(void)
{
    Fixture fixture;
    HTHPlayerSlot target;
    HTHPlayerSlot teammate;
    const double first_duration = 4.0;
    const double second_duration = 7.0;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, &target));
    CHECK(add_player(&fixture, &teammate));
    CHECK(step(&fixture, target, true, 0.25, &first_duration));
    CHECK(step(&fixture, target, true, 1.0, &second_duration));
    CHECK(query_window(&fixture, target, true, 3.0));

    CHECK(step(&fixture, target, false, 1.0, NULL));
    CHECK(query_window(&fixture, target, false, 0.0));
    CHECK(query_defeat(&fixture, target, false));
    CHECK(!fixture.runtime.was_dead[target]);

    CHECK(step(&fixture, target, true, 0.5, &second_duration));
    CHECK(query_window(&fixture, target, true, second_duration));
    CHECK(query_defeat(&fixture, target, false));
    fixture_destroy(&fixture);
    return true;
}

static bool test_stale_membership_and_actor_loss(void)
{
    Fixture fixture;
    HTHPlayerSlot target;
    HTHPlayerSlot stale_slot;
    const double duration = 2.0;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, &target));
    CHECK(add_player(&fixture, &stale_slot));
    CHECK(hth_actor_store_remove(
        fixture.actors, fixture.entities, fixture.players[stale_slot]));
    CHECK(!step(&fixture, target, true, 0.1, &duration));
    CHECK(!fixture.runtime.was_dead[target]);
    CHECK(query_defeat(&fixture, target, false));
    CHECK(query_window(&fixture, target, false, 0.0));
    CHECK(!step(&fixture, stale_slot, false, 0.1, NULL));
    fixture_destroy(&fixture);
    return true;
}

static bool test_destroyed_entity_step(void)
{
    Fixture fixture;
    HTHPlayerSlot slot;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, &slot));
    CHECK(hth_entity_registry_destroy_entity(
        fixture.entities, fixture.players[slot]));
    CHECK(!step(&fixture, slot, false, 0.1, NULL));
    CHECK(hth_player_roster_count(&fixture.runtime.roster) == 1U);
    CHECK(!fixture.runtime.was_dead[slot]);
    fixture_destroy(&fixture);
    return true;
}

static bool test_four_slot_independence(void)
{
    Fixture fixture;
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    const double duration = 5.0;
    HTHPlayerSlot index;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        CHECK(add_player(&fixture, &slots[index]));
        CHECK(slots[index] == index);
    }
    CHECK(hth_player_revive_window_begin(
        &fixture.runtime.revive_windows[slots[0]], 2.0));
    fixture.runtime.was_dead[slots[0]] = true;
    CHECK(step(&fixture, slots[0], false, 0.5, NULL));

    CHECK(step(&fixture, slots[1], true, 0.5, &duration));

    CHECK(hth_player_defeat_mark(
        &fixture.runtime.defeat[slots[2]]));
    CHECK(hth_player_revive_window_begin(
        &fixture.runtime.revive_windows[slots[2]], 2.0));
    CHECK(step(&fixture, slots[2], true, 0.5, NULL));

    fixture.runtime.was_dead[slots[3]] = true;
    CHECK(step(&fixture, slots[3], true, 0.5, NULL));

    CHECK(query_defeat(&fixture, slots[0], false));
    CHECK(query_window(&fixture, slots[0], false, 0.0));
    CHECK(!fixture.runtime.was_dead[slots[0]]);
    CHECK(query_defeat(&fixture, slots[1], false));
    CHECK(query_window(&fixture, slots[1], true, duration));
    CHECK(query_defeat(&fixture, slots[2], true));
    CHECK(query_window(&fixture, slots[2], false, 0.0));
    CHECK(query_defeat(&fixture, slots[3], true));
    CHECK(query_window(&fixture, slots[3], false, 0.0));

    hth_player_lifecycle_runtime_reset(&fixture.runtime);
    CHECK(hth_player_roster_count(&fixture.runtime.roster) == 0U);
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        CHECK(!fixture.runtime.defeat[index].defeated);
        CHECK(!fixture.runtime.revive_windows[index].active);
        CHECK(fixture.runtime.revive_windows[index].remaining_seconds == 0.0);
        CHECK(!fixture.runtime.was_dead[index]);
    }
    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_zero_reset_and_accessors,
        test_register_unregister_reset_and_reuse,
        test_registration_failures_and_generation_safety,
        test_solo_lifecycle_and_defeat_persistence,
        test_coop_begin_advance_expiry_and_inactive_resolution,
        test_invalid_delta_and_duration_retry,
        test_recovery_and_second_episode,
        test_stale_membership_and_actor_loss,
        test_destroyed_entity_step,
        test_four_slot_independence
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("player lifecycle runtime tests passed");
    return EXIT_SUCCESS;
}
