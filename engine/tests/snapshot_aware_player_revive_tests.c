#include "player_death.h"
#include "player_death_snapshot.h"
#include "player_revive_eligibility.h"
#include "player_revive_interaction.h"
#include "player_revive_target_selection.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,       \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

typedef struct Fixture {
    HTHEntityRegistry *entities;
    HTHActorStore *actors;
    HTHHealthStore *health;
    HTHSpatialStore *spatial;
    HTHPlayerLifecycleRuntime lifecycle;
    HTHEntityHandle players[HTH_MAX_PLAYERS];
} Fixture;

static size_t death_query_count;

bool __real_hth_player_death_is_dead(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    HTHEntityHandle player,
    bool *out_dead);

bool __wrap_hth_player_death_is_dead(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    HTHEntityHandle player,
    bool *out_dead)
{
    death_query_count++;
    return __real_hth_player_death_is_dead(
        entities, actors, health, player, out_dead);
}

static bool fixture_create(Fixture *fixture)
{
    *fixture = (Fixture){0};
    fixture->entities = hth_entity_registry_create();
    fixture->actors = hth_actor_store_create();
    fixture->health = hth_health_store_create();
    fixture->spatial = hth_spatial_store_create();
    hth_player_lifecycle_runtime_reset(&fixture->lifecycle);
    return fixture->entities != NULL && fixture->actors != NULL &&
           fixture->health != NULL && fixture->spatial != NULL;
}

static void fixture_destroy(Fixture *fixture)
{
    hth_spatial_store_destroy(fixture->spatial);
    hth_health_store_destroy(fixture->health);
    hth_actor_store_destroy(fixture->actors);
    hth_entity_registry_destroy(fixture->entities);
    *fixture = (Fixture){0};
}

static bool add_player(Fixture *fixture, HTHVec3 position,
                       float current_health, HTHPlayerSlot *out_slot)
{
    HTHSpatialTransform transform = {position, 0.0F};
    HTHEntityHandle player;
    HTHPlayerSlot slot;

    if (!hth_entity_registry_create_entity(fixture->entities, &player) ||
        !hth_actor_store_attach(fixture->actors, fixture->entities, player) ||
        !hth_health_store_attach(
            fixture->health, fixture->entities, fixture->actors, player,
            (HTHHealth){current_health, 100.0F}) ||
        !hth_spatial_store_attach(
            fixture->spatial, fixture->entities, player, &transform) ||
        !hth_player_lifecycle_runtime_register(
            &fixture->lifecycle, fixture->entities, fixture->actors, player,
            &slot)) {
        return false;
    }
    fixture->players[slot] = player;
    *out_slot = slot;
    return true;
}

static bool remove_player(Fixture *fixture, HTHPlayerSlot slot)
{
    HTHEntityHandle player;
    const HTHPlayerRoster *roster =
        hth_player_lifecycle_runtime_get_roster(&fixture->lifecycle);

    return hth_player_roster_get_slot(
               roster, fixture->entities, fixture->actors, slot, &player) &&
           hth_player_lifecycle_runtime_unregister(
               &fixture->lifecycle, slot) &&
           hth_spatial_store_remove(
               fixture->spatial, fixture->entities, player) &&
           hth_health_store_remove(
               fixture->health, fixture->entities, fixture->actors, player) &&
           hth_actor_store_remove(
               fixture->actors, fixture->entities, player) &&
           hth_entity_registry_destroy_entity(fixture->entities, player);
}

static bool set_health(Fixture *fixture, HTHPlayerSlot slot, float desired)
{
    HTHHealth health;

    if (!hth_health_store_get(
            fixture->health, fixture->entities, fixture->actors,
            fixture->players[slot], &health)) {
        return false;
    }
    if (desired < health.current) {
        HTHDamageResult result;

        return hth_health_store_apply_damage(
            fixture->health, fixture->entities, fixture->actors,
            fixture->players[slot], health.current - desired, &result);
    }
    if (desired > health.current) {
        HTHHealingResult result;

        return hth_health_store_apply_healing(
            fixture->health, fixture->entities, fixture->actors,
            fixture->players[slot], desired - health.current, &result);
    }
    return true;
}

static bool begin_window(Fixture *fixture, HTHPlayerSlot target)
{
    return hth_player_revive_window_begin(
        &fixture->lifecycle.revive_windows[target], 10.0);
}

static bool build_snapshot(Fixture *fixture,
                           HTHPlayerDeathSnapshot *out_snapshot)
{
    return hth_player_death_snapshot_build(
        hth_player_lifecycle_runtime_get_roster(&fixture->lifecycle),
        fixture->entities, fixture->actors, fixture->health, out_snapshot);
}

static bool legacy_eligibility(Fixture *fixture, HTHPlayerSlot reviver,
                               HTHPlayerSlot target, bool *out_eligible)
{
    return hth_player_revive_eligibility_evaluate(
        hth_player_lifecycle_runtime_get_roster(&fixture->lifecycle),
        fixture->entities, fixture->actors, fixture->health, reviver,
        &fixture->lifecycle.defeat[reviver], target,
        &fixture->lifecycle.defeat[target],
        &fixture->lifecycle.revive_windows[target], out_eligible);
}

static bool snapshot_eligibility(
    Fixture *fixture, const HTHPlayerDeathSnapshot *snapshot,
    HTHPlayerSlot reviver, HTHPlayerSlot target, bool *out_eligible)
{
    return hth_player_revive_eligibility_evaluate_snapshot(
        hth_player_lifecycle_runtime_get_roster(&fixture->lifecycle),
        fixture->entities, fixture->actors, snapshot, reviver,
        &fixture->lifecycle.defeat[reviver], target,
        &fixture->lifecycle.defeat[target],
        &fixture->lifecycle.revive_windows[target], out_eligible);
}

static bool snapshot_step(
    Fixture *fixture, HTHPlayerReviveInteraction *interaction,
    const HTHPlayerDeathSnapshot *snapshot, HTHPlayerSlot reviver,
    HTHPlayerSlot target, double delta, double duration, bool *out_revived)
{
    return hth_player_revive_interaction_step_snapshot(
        interaction, &fixture->lifecycle, fixture->entities,
        fixture->actors, fixture->health, snapshot, fixture->spatial,
        reviver, target, true, delta, 10.0F, duration, 25.0F,
        out_revived);
}

static bool interaction_equal(HTHPlayerReviveInteraction left,
                              HTHPlayerReviveInteraction right)
{
    return left.reviver_slot == right.reviver_slot &&
           hth_entity_handle_equal(left.reviver_entity,
                                   right.reviver_entity) &&
           left.target_slot == right.target_slot &&
           hth_entity_handle_equal(left.target_entity,
                                   right.target_entity) &&
           left.elapsed_seconds == right.elapsed_seconds &&
           left.required_seconds == right.required_seconds &&
           left.active == right.active;
}

static bool eligibility_paths_match(
    Fixture *fixture, const HTHPlayerDeathSnapshot *snapshot,
    HTHPlayerSlot reviver, HTHPlayerSlot target, bool expected)
{
    bool legacy = !expected;
    bool sampled = !expected;

    death_query_count = 0U;
    if (!legacy_eligibility(fixture, reviver, target, &legacy) ||
        death_query_count != 2U) {
        return false;
    }
    death_query_count = 0U;
    return snapshot_eligibility(
               fixture, snapshot, reviver, target, &sampled) &&
           death_query_count == 0U && legacy == expected &&
           sampled == expected;
}

static bool test_eligibility_parity_and_query_budget(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    bool sampled;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, hth_vec3(0.0F, 0.0F, 0.0F), 100.0F,
                     &reviver));
    CHECK(add_player(&fixture, hth_vec3(1.0F, 0.0F, 0.0F), 0.0F,
                     &target));
    CHECK(begin_window(&fixture, target));

    death_query_count = 0U;
    CHECK(build_snapshot(&fixture, &snapshot));
    CHECK(death_query_count == 2U);
    CHECK(eligibility_paths_match(
        &fixture, &snapshot, reviver, target, true));

    CHECK(eligibility_paths_match(
        &fixture, &snapshot, reviver, reviver, false));
    fixture.lifecycle.defeat[reviver].defeated = true;
    CHECK(eligibility_paths_match(
        &fixture, &snapshot, reviver, target, false));
    fixture.lifecycle.defeat[reviver].defeated = false;
    fixture.lifecycle.defeat[target].defeated = true;
    CHECK(eligibility_paths_match(
        &fixture, &snapshot, reviver, target, false));
    fixture.lifecycle.defeat[target].defeated = false;
    hth_player_revive_window_reset(
        &fixture.lifecycle.revive_windows[target]);
    CHECK(eligibility_paths_match(
        &fixture, &snapshot, reviver, target, false));
    CHECK(begin_window(&fixture, target));

    CHECK(set_health(&fixture, target, 10.0F));
    CHECK(build_snapshot(&fixture, &snapshot));
    CHECK(eligibility_paths_match(
        &fixture, &snapshot, reviver, target, false));
    CHECK(set_health(&fixture, target, 0.0F));
    CHECK(set_health(&fixture, reviver, 0.0F));
    CHECK(build_snapshot(&fixture, &snapshot));
    CHECK(eligibility_paths_match(
        &fixture, &snapshot, reviver, target, false));

    snapshot.entries[target].present = false;
    sampled = true;
    CHECK(!snapshot_eligibility(
        &fixture, &snapshot, reviver, target, &sampled));
    CHECK(!sampled && death_query_count == 0U);

    fixture_destroy(&fixture);
    return true;
}

static bool test_selector_parity_and_query_budget(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;
    HTHPlayerDeathSnapshot mismatch;
    HTHPlayerSlot slots[HTH_MAX_PLAYERS];
    HTHPlayerSlot legacy_target;
    HTHPlayerSlot sampled_target;
    size_t index;

    CHECK(fixture_create(&fixture));
    for (index = 0U; index < HTH_MAX_PLAYERS; ++index) {
        const float x = index == 0U ? 0.0F :
                        (index == 3U ? 1.0F : 2.0F);
        CHECK(add_player(&fixture, hth_vec3(x, 0.0F, 0.0F),
                         index == 0U ? 100.0F : 0.0F, &slots[index]));
        CHECK(slots[index] == index);
        if (index != 0U) {
            CHECK(begin_window(&fixture, slots[index]));
        }
    }
    fixture.lifecycle.defeat[slots[3]].defeated = true;

    death_query_count = 0U;
    CHECK(build_snapshot(&fixture, &snapshot));
    CHECK(death_query_count == 4U);
    death_query_count = 0U;
    CHECK(hth_player_revive_target_select(
        &fixture.lifecycle, fixture.entities, fixture.actors,
        fixture.health, fixture.spatial, slots[0], 2.0F, &legacy_target));
    CHECK(legacy_target == slots[1] && death_query_count == 6U);
    death_query_count = 0U;
    CHECK(hth_player_revive_target_select_snapshot(
        &fixture.lifecycle, fixture.entities, fixture.actors, &snapshot,
        fixture.spatial, slots[0], 2.0F, &sampled_target));
    CHECK(sampled_target == legacy_target && death_query_count == 0U);

    mismatch = snapshot;
    mismatch.entries[slots[2]].entity.generation++;
    sampled_target = slots[1];
    CHECK(!hth_player_revive_target_select_snapshot(
        &fixture.lifecycle, fixture.entities, fixture.actors, &mismatch,
        fixture.spatial, slots[0], 2.0F, &sampled_target));
    CHECK(sampled_target == HTH_PLAYER_SLOT_INVALID);

    CHECK(remove_player(&fixture, slots[1]));
    death_query_count = 0U;
    CHECK(build_snapshot(&fixture, &snapshot));
    CHECK(death_query_count == 3U);
    CHECK(hth_player_revive_target_select_snapshot(
        &fixture.lifecycle, fixture.entities, fixture.actors, &snapshot,
        fixture.spatial, slots[0], 2.0F, &sampled_target));
    CHECK(sampled_target == slots[2]);

    fixture_destroy(&fixture);
    return true;
}

static bool test_interaction_multiframe_and_semantic_reset(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot first;
    HTHPlayerDeathSnapshot second;
    HTHPlayerReviveInteraction interaction = {0};
    HTHPlayerReviveInteraction legacy_interaction = {0};
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    bool revived = true;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, hth_vec3(0.0F, 0.0F, 0.0F), 100.0F,
                     &reviver));
    CHECK(add_player(&fixture, hth_vec3(1.0F, 0.0F, 0.0F), 0.0F,
                     &target));
    CHECK(begin_window(&fixture, target));
    CHECK(build_snapshot(&fixture, &first));

    death_query_count = 0U;
    CHECK(hth_player_revive_interaction_step(
        &legacy_interaction, &fixture.lifecycle, fixture.entities,
        fixture.actors, fixture.health, fixture.spatial, reviver, target,
        true, 0.1, 10.0F, 1.0, 25.0F, &revived));
    CHECK(!revived && death_query_count == 2U &&
          legacy_interaction.active);

    death_query_count = 0U;
    CHECK(snapshot_step(
        &fixture, &interaction, &first, reviver, target, 0.25, 1.0,
        &revived));
    CHECK(!revived && death_query_count == 0U && interaction.active);
    CHECK(interaction.reviver_slot == reviver &&
          hth_entity_handle_equal(
              interaction.reviver_entity, fixture.players[reviver]) &&
          interaction.target_slot == target &&
          hth_entity_handle_equal(
              interaction.target_entity, fixture.players[target]) &&
          interaction.elapsed_seconds == 0.25 &&
          interaction.required_seconds == 1.0);

    CHECK(build_snapshot(&fixture, &second));
    death_query_count = 0U;
    CHECK(snapshot_step(
        &fixture, &interaction, &second, reviver, target, 0.25, 9.0,
        &revived));
    CHECK(!revived && death_query_count == 0U &&
          interaction.elapsed_seconds == 0.5 &&
          interaction.required_seconds == 1.0);

    CHECK(set_health(&fixture, target, 10.0F));
    CHECK(build_snapshot(&fixture, &second));
    CHECK(snapshot_step(
        &fixture, &interaction, &second, reviver, target, 0.1, 1.0,
        &revived));
    CHECK(!revived && !interaction.active);

    fixture_destroy(&fixture);
    return true;
}

static bool test_snapshot_interaction_live_resets_and_retarget(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;
    HTHPlayerReviveInteraction interaction = {0};
    HTHPlayerSlot reviver;
    HTHPlayerSlot target_a;
    HTHPlayerSlot target_b;
    bool revived = true;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, hth_vec3(0.0F, 0.0F, 0.0F), 100.0F,
                     &reviver));
    CHECK(add_player(&fixture, hth_vec3(1.0F, 0.0F, 0.0F), 0.0F,
                     &target_a));
    CHECK(add_player(&fixture, hth_vec3(1.5F, 0.0F, 0.0F), 0.0F,
                     &target_b));
    CHECK(begin_window(&fixture, target_a));
    CHECK(begin_window(&fixture, target_b));
    CHECK(build_snapshot(&fixture, &snapshot));

    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target_a, 0.2, 1.0,
        &revived));
    CHECK(interaction.active && interaction.elapsed_seconds == 0.2);
    CHECK(hth_player_revive_interaction_step_snapshot(
        &interaction, &fixture.lifecycle, fixture.entities, fixture.actors,
        fixture.health, &snapshot, fixture.spatial, reviver, target_a, true,
        0.1, 0.5F, 1.0, 25.0F, &revived));
    CHECK(!interaction.active);

    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target_a, 0.2, 1.0,
        &revived));
    fixture.lifecycle.defeat[target_a].defeated = true;
    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target_a, 0.1, 1.0,
        &revived));
    CHECK(!interaction.active);
    fixture.lifecycle.defeat[target_a].defeated = false;

    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target_a, 0.2, 1.0,
        &revived));
    fixture.lifecycle.defeat[reviver].defeated = true;
    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target_a, 0.1, 1.0,
        &revived));
    CHECK(!interaction.active);
    fixture.lifecycle.defeat[reviver].defeated = false;

    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target_a, 0.2, 1.0,
        &revived));
    hth_player_revive_window_reset(
        &fixture.lifecycle.revive_windows[target_a]);
    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target_a, 0.1, 1.0,
        &revived));
    CHECK(!interaction.active);
    CHECK(begin_window(&fixture, target_a));

    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target_a, 0.4, 1.0,
        &revived));
    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target_b, 0.1, 2.0,
        &revived));
    CHECK(interaction.active && interaction.target_slot == target_b &&
          interaction.elapsed_seconds == 0.1 &&
          interaction.required_seconds == 2.0);
    CHECK(hth_player_revive_interaction_step_snapshot(
        &interaction, &fixture.lifecycle, fixture.entities, fixture.actors,
        fixture.health, &snapshot, fixture.spatial, reviver, target_b, false,
        0.0, 10.0F, 2.0, 25.0F, &revived));
    CHECK(!interaction.active);

    fixture_destroy(&fixture);
    return true;
}

static bool test_target_reuse_and_transactionality(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot old_snapshot;
    HTHPlayerDeathSnapshot new_snapshot;
    HTHPlayerReviveInteraction interaction = {0};
    HTHPlayerReviveInteraction before;
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerSlot replacement_slot;
    HTHEntityHandle old_target;
    bool revived = true;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, hth_vec3(0.0F, 0.0F, 0.0F), 100.0F,
                     &reviver));
    CHECK(add_player(&fixture, hth_vec3(1.0F, 0.0F, 0.0F), 0.0F,
                     &target));
    CHECK(begin_window(&fixture, target));
    CHECK(build_snapshot(&fixture, &old_snapshot));
    CHECK(snapshot_step(
        &fixture, &interaction, &old_snapshot, reviver, target, 0.4, 1.0,
        &revived));
    old_target = fixture.players[target];
    CHECK(remove_player(&fixture, target));
    CHECK(add_player(&fixture, hth_vec3(1.0F, 0.0F, 0.0F), 0.0F,
                     &replacement_slot));
    CHECK(replacement_slot == target);
    CHECK(fixture.players[target].index == old_target.index);
    CHECK(fixture.players[target].generation != old_target.generation);
    CHECK(begin_window(&fixture, target));

    before = interaction;
    CHECK(!snapshot_step(
        &fixture, &interaction, &old_snapshot, reviver, target, 0.1, 2.0,
        &revived));
    CHECK(!revived && interaction_equal(interaction, before));

    CHECK(build_snapshot(&fixture, &new_snapshot));
    CHECK(snapshot_step(
        &fixture, &interaction, &new_snapshot, reviver, target, 0.1, 2.0,
        &revived));
    CHECK(!revived && interaction.active &&
          interaction.elapsed_seconds == 0.1 &&
          interaction.required_seconds == 2.0 &&
          hth_entity_handle_equal(
              interaction.target_entity, fixture.players[target]));

    fixture_destroy(&fixture);
    return true;
}

static bool test_reviver_reuse(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;
    HTHPlayerReviveInteraction interaction = {0};
    HTHPlayerSlot reviver;
    HTHPlayerSlot target;
    HTHPlayerSlot replacement_slot;
    HTHEntityHandle old_reviver;
    bool revived = true;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, hth_vec3(0.0F, 0.0F, 0.0F), 100.0F,
                     &reviver));
    CHECK(add_player(&fixture, hth_vec3(1.0F, 0.0F, 0.0F), 0.0F,
                     &target));
    CHECK(begin_window(&fixture, target));
    CHECK(build_snapshot(&fixture, &snapshot));
    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target, 0.4, 1.0,
        &revived));
    old_reviver = fixture.players[reviver];
    CHECK(remove_player(&fixture, reviver));
    CHECK(add_player(&fixture, hth_vec3(0.0F, 0.0F, 0.0F), 100.0F,
                     &replacement_slot));
    CHECK(replacement_slot == reviver);
    CHECK(fixture.players[reviver].index == old_reviver.index);
    CHECK(fixture.players[reviver].generation != old_reviver.generation);
    CHECK(build_snapshot(&fixture, &snapshot));
    CHECK(snapshot_step(
        &fixture, &interaction, &snapshot, reviver, target, 0.1, 2.0,
        &revived));
    CHECK(!revived && interaction.elapsed_seconds == 0.1 &&
          interaction.required_seconds == 2.0 &&
          hth_entity_handle_equal(
              interaction.reviver_entity, fixture.players[reviver]));

    fixture_destroy(&fixture);
    return true;
}

static bool test_live_commit_guards_and_multiple_revivers(void)
{
    Fixture fixture;
    HTHPlayerDeathSnapshot snapshot;
    HTHPlayerReviveInteraction first = {0};
    HTHPlayerReviveInteraction second = {0};
    HTHPlayerSlot reviver_a;
    HTHPlayerSlot reviver_b;
    HTHPlayerSlot target;
    HTHHealth target_health;
    bool revived = true;

    CHECK(fixture_create(&fixture));
    CHECK(add_player(&fixture, hth_vec3(0.0F, 0.0F, 0.0F), 100.0F,
                     &reviver_a));
    CHECK(add_player(&fixture, hth_vec3(0.5F, 0.0F, 0.0F), 100.0F,
                     &reviver_b));
    CHECK(add_player(&fixture, hth_vec3(1.0F, 0.0F, 0.0F), 0.0F,
                     &target));
    CHECK(begin_window(&fixture, target));
    CHECK(build_snapshot(&fixture, &snapshot));

    death_query_count = 0U;
    CHECK(snapshot_step(
        &fixture, &first, &snapshot, reviver_a, target, 1.0, 1.0,
        &revived));
    CHECK(revived && death_query_count == 2U);
    death_query_count = 0U;
    revived = true;
    CHECK(snapshot_step(
        &fixture, &second, &snapshot, reviver_b, target, 1.0, 1.0,
        &revived));
    CHECK(!revived && death_query_count == 0U && !second.active);
    CHECK(hth_health_store_get(
        fixture.health, fixture.entities, fixture.actors,
        fixture.players[target], &target_health));
    CHECK(target_health.current == 25.0F);

    hth_player_revive_interaction_reset(&first);
    CHECK(set_health(&fixture, target, 0.0F));
    CHECK(begin_window(&fixture, target));
    CHECK(build_snapshot(&fixture, &snapshot));
    CHECK(set_health(&fixture, target, 10.0F));
    death_query_count = 0U;
    revived = true;
    CHECK(snapshot_step(
        &fixture, &first, &snapshot, reviver_a, target, 1.0, 1.0,
        &revived));
    CHECK(!revived && death_query_count == 2U && !first.active);
    CHECK(hth_health_store_get(
        fixture.health, fixture.entities, fixture.actors,
        fixture.players[target], &target_health));
    CHECK(target_health.current == 10.0F);

    CHECK(set_health(&fixture, target, 0.0F));
    CHECK(build_snapshot(&fixture, &snapshot));
    CHECK(set_health(&fixture, reviver_a, 0.0F));
    death_query_count = 0U;
    revived = true;
    CHECK(snapshot_step(
        &fixture, &first, &snapshot, reviver_a, target, 1.0, 1.0,
        &revived));
    CHECK(!revived && death_query_count == 2U && !first.active);

    fixture_destroy(&fixture);
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_eligibility_parity_and_query_budget,
        test_selector_parity_and_query_budget,
        test_interaction_multiframe_and_semantic_reset,
        test_snapshot_interaction_live_resets_and_retarget,
        test_target_reuse_and_transactionality,
        test_reviver_reuse,
        test_live_commit_guards_and_multiple_revivers,
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
