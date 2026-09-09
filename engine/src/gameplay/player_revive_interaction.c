#include "player_revive_interaction.h"

#include "player_revive_eligibility.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum HTHReviveEligibilitySource {
    HTH_REVIVE_ELIGIBILITY_LIVE = 0,
    HTH_REVIVE_ELIGIBILITY_SNAPSHOT
} HTHReviveEligibilitySource;

static bool handle_is_structurally_possible(HTHEntityHandle entity)
{
    return entity.index != UINT32_MAX && entity.generation != 0U;
}

static bool interaction_is_valid(
    const HTHPlayerReviveInteraction *interaction)
{
    if (interaction == NULL) {
        return false;
    }
    if (!interaction->active) {
        return true;
    }
    return interaction->reviver_slot < HTH_MAX_PLAYERS &&
           handle_is_structurally_possible(interaction->reviver_entity) &&
           interaction->target_slot < HTH_MAX_PLAYERS &&
           handle_is_structurally_possible(interaction->target_entity) &&
           isfinite(interaction->elapsed_seconds) &&
           interaction->elapsed_seconds >= 0.0 &&
           isfinite(interaction->required_seconds) &&
           interaction->required_seconds > 0.0 &&
           interaction->elapsed_seconds < interaction->required_seconds;
}

void hth_player_revive_interaction_reset(
    HTHPlayerReviveInteraction *interaction)
{
    if (interaction != NULL) {
        memset(interaction, 0, sizeof(*interaction));
    }
}

bool hth_player_revive_interaction_query(
    const HTHPlayerReviveInteraction *interaction,
    bool *out_active,
    HTHPlayerSlot *out_target_slot,
    double *out_elapsed_seconds,
    double *out_required_seconds)
{
    if (out_active != NULL) {
        *out_active = false;
    }
    if (out_target_slot != NULL) {
        *out_target_slot = HTH_PLAYER_SLOT_INVALID;
    }
    if (out_elapsed_seconds != NULL) {
        *out_elapsed_seconds = 0.0;
    }
    if (out_required_seconds != NULL) {
        *out_required_seconds = 0.0;
    }
    if (out_active == NULL || out_target_slot == NULL ||
        out_elapsed_seconds == NULL || out_required_seconds == NULL ||
        !interaction_is_valid(interaction)) {
        return false;
    }
    if (!interaction->active) {
        return true;
    }
    *out_active = true;
    *out_target_slot = interaction->target_slot;
    *out_elapsed_seconds = interaction->elapsed_seconds;
    *out_required_seconds = interaction->required_seconds;
    return true;
}

static bool interaction_distance_squared(
    HTHVec3 reviver_position,
    HTHVec3 target_position,
    double *out_distance_squared)
{
    const double dx = (double)target_position.x -
                      (double)reviver_position.x;
    const double dy = (double)target_position.y -
                      (double)reviver_position.y;
    const double dz = (double)target_position.z -
                      (double)reviver_position.z;
    const double distance_squared = dx * dx + dy * dy + dz * dz;

    if (out_distance_squared == NULL || !isfinite(distance_squared)) {
        return false;
    }
    *out_distance_squared = distance_squared;
    return true;
}

static bool interaction_can_revive(
    const HTHPlayerLifecycleRuntime *lifecycle,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    const HTHPlayerDeathSnapshot *death_snapshot,
    HTHReviveEligibilitySource source,
    HTHPlayerSlot reviver_slot,
    HTHPlayerSlot target_slot,
    bool *out_eligible)
{
    const HTHPlayerDefeatState *reviver_defeat;
    const HTHPlayerDefeatState *target_defeat;
    const HTHPlayerReviveWindow *target_window;
    const HTHPlayerRoster *roster;

    if (source == HTH_REVIVE_ELIGIBILITY_LIVE) {
        return hth_player_lifecycle_runtime_can_revive(
            lifecycle, entities, actors, health, reviver_slot, target_slot,
            out_eligible);
    }

    roster = hth_player_lifecycle_runtime_get_roster(lifecycle);
    if (source != HTH_REVIVE_ELIGIBILITY_SNAPSHOT || roster == NULL ||
        !hth_player_lifecycle_runtime_get_defeat(
            lifecycle, entities, actors, reviver_slot, &reviver_defeat) ||
        !hth_player_lifecycle_runtime_get_defeat(
            lifecycle, entities, actors, target_slot, &target_defeat) ||
        !hth_player_lifecycle_runtime_get_revive_window(
            lifecycle, entities, actors, target_slot, &target_window)) {
        return false;
    }
    return hth_player_revive_eligibility_evaluate_snapshot(
        roster, entities, actors, death_snapshot, reviver_slot,
        reviver_defeat, target_slot, target_defeat, target_window,
        out_eligible);
}

static bool interaction_step_impl(
    HTHPlayerReviveInteraction *interaction,
    HTHPlayerLifecycleRuntime *lifecycle,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHHealthStore *health,
    const HTHPlayerDeathSnapshot *death_snapshot,
    const HTHSpatialStore *spatial,
    HTHPlayerSlot reviver_slot,
    HTHPlayerSlot candidate_target_slot,
    bool interaction_held,
    double delta_seconds,
    float revive_range,
    double hold_duration_seconds,
    float revive_health,
    bool *out_revived,
    HTHReviveEligibilitySource source)
{
    HTHPlayerReviveInteraction next;
    HTHSpatialTransform reviver_transform;
    HTHSpatialTransform target_transform;
    HTHEntityHandle reviver;
    HTHEntityHandle target;
    const HTHPlayerRoster *roster;
    double distance_squared;
    double range_squared;
    double remaining_seconds;
    bool eligible;
    bool revived;

    if (out_revived != NULL) {
        *out_revived = false;
    }
    if (interaction == NULL || lifecycle == NULL || entities == NULL ||
        actors == NULL || health == NULL || spatial == NULL ||
        out_revived == NULL || !interaction_is_valid(interaction) ||
        (source == HTH_REVIVE_ELIGIBILITY_SNAPSHOT &&
         death_snapshot == NULL) ||
        (source != HTH_REVIVE_ELIGIBILITY_LIVE &&
         source != HTH_REVIVE_ELIGIBILITY_SNAPSHOT)) {
        return false;
    }
    if (!interaction_held) {
        hth_player_revive_interaction_reset(interaction);
        return true;
    }
    if (!isfinite(delta_seconds) || delta_seconds < 0.0 ||
        !isfinite(revive_range) || revive_range <= 0.0F ||
        !isfinite(hold_duration_seconds) || hold_duration_seconds <= 0.0 ||
        !isfinite(revive_health) || revive_health <= 0.0F ||
        reviver_slot >= HTH_MAX_PLAYERS ||
        candidate_target_slot >= HTH_MAX_PLAYERS) {
        return false;
    }

    roster = hth_player_lifecycle_runtime_get_roster(lifecycle);
    if (roster == NULL ||
        !hth_player_roster_get_slot(roster, entities, actors,
                                    reviver_slot, &reviver) ||
        !hth_player_roster_get_slot(roster, entities, actors,
                                    candidate_target_slot, &target) ||
        !hth_spatial_store_get(spatial, entities, reviver,
                               &reviver_transform) ||
        !hth_spatial_store_get(spatial, entities, target,
                               &target_transform) ||
        !interaction_distance_squared(reviver_transform.position,
                                      target_transform.position,
                                      &distance_squared)) {
        return false;
    }
    range_squared = (double)revive_range * (double)revive_range;
    if (!isfinite(range_squared)) {
        return false;
    }
    if (!interaction_can_revive(
            lifecycle, entities, actors, health, death_snapshot, source,
            reviver_slot, candidate_target_slot, &eligible)) {
        return false;
    }
    if (!eligible || distance_squared > range_squared) {
        hth_player_revive_interaction_reset(interaction);
        return true;
    }

    next = *interaction;
    if (!next.active || next.reviver_slot != reviver_slot ||
        !hth_entity_handle_equal(next.reviver_entity, reviver) ||
        next.target_slot != candidate_target_slot ||
        !hth_entity_handle_equal(next.target_entity, target)) {
        next.reviver_slot = reviver_slot;
        next.reviver_entity = reviver;
        next.target_slot = candidate_target_slot;
        next.target_entity = target;
        next.elapsed_seconds = 0.0;
        next.required_seconds = hold_duration_seconds;
        next.active = true;
    }
    remaining_seconds = next.required_seconds - next.elapsed_seconds;
    if (delta_seconds < remaining_seconds) {
        next.elapsed_seconds += delta_seconds;
        *interaction = next;
        return true;
    }

    if (!hth_player_lifecycle_runtime_execute_revive(
            lifecycle, entities, actors, health, reviver_slot,
            candidate_target_slot, revive_health, &revived)) {
        return false;
    }
    hth_player_revive_interaction_reset(interaction);
    *out_revived = revived;
    return true;
}

bool hth_player_revive_interaction_step(
    HTHPlayerReviveInteraction *interaction,
    HTHPlayerLifecycleRuntime *lifecycle,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHHealthStore *health,
    const HTHSpatialStore *spatial,
    HTHPlayerSlot reviver_slot,
    HTHPlayerSlot candidate_target_slot,
    bool interaction_held,
    double delta_seconds,
    float revive_range,
    double hold_duration_seconds,
    float revive_health,
    bool *out_revived)
{
    return interaction_step_impl(
        interaction, lifecycle, entities, actors, health, NULL, spatial,
        reviver_slot, candidate_target_slot, interaction_held,
        delta_seconds, revive_range, hold_duration_seconds, revive_health,
        out_revived, HTH_REVIVE_ELIGIBILITY_LIVE);
}

bool hth_player_revive_interaction_step_snapshot(
    HTHPlayerReviveInteraction *interaction,
    HTHPlayerLifecycleRuntime *lifecycle,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHHealthStore *health,
    const HTHPlayerDeathSnapshot *death_snapshot,
    const HTHSpatialStore *spatial,
    HTHPlayerSlot reviver_slot,
    HTHPlayerSlot candidate_target_slot,
    bool interaction_held,
    double delta_seconds,
    float revive_range,
    double hold_duration_seconds,
    float revive_health,
    bool *out_revived)
{
    return interaction_step_impl(
        interaction, lifecycle, entities, actors, health, death_snapshot,
        spatial, reviver_slot, candidate_target_slot, interaction_held,
        delta_seconds, revive_range, hold_duration_seconds, revive_health,
        out_revived, HTH_REVIVE_ELIGIBILITY_SNAPSHOT);
}
