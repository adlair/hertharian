#include "player_revive_interaction.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static bool interaction_is_valid(
    const HTHPlayerReviveInteraction *interaction)
{
    if (interaction == NULL) {
        return false;
    }
    if (!interaction->active) {
        return true;
    }
    return interaction->target_slot < HTH_MAX_PLAYERS &&
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
        out_revived == NULL || !interaction_is_valid(interaction)) {
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
    if (!hth_player_lifecycle_runtime_can_revive(
            lifecycle, entities, actors, health, reviver_slot,
            candidate_target_slot, &eligible)) {
        return false;
    }
    if (!eligible || distance_squared > range_squared) {
        hth_player_revive_interaction_reset(interaction);
        return true;
    }

    next = *interaction;
    if (!next.active || next.target_slot != candidate_target_slot) {
        next.target_slot = candidate_target_slot;
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
