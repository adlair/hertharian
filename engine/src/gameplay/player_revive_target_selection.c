#include "player_revive_target_selection.h"

#include "player_revive_eligibility.h"

#include <math.h>

typedef enum HTHReviveEligibilitySource {
    HTH_REVIVE_ELIGIBILITY_LIVE = 0,
    HTH_REVIVE_ELIGIBILITY_SNAPSHOT
} HTHReviveEligibilitySource;

static bool distance_squared(HTHVec3 left, HTHVec3 right,
                             double *out_distance_squared)
{
    const double dx = (double)right.x - (double)left.x;
    const double dy = (double)right.y - (double)left.y;
    const double dz = (double)right.z - (double)left.z;
    const double result = dx * dx + dy * dy + dz * dz;

    if (out_distance_squared == NULL || !isfinite(result)) {
        return false;
    }
    *out_distance_squared = result;
    return true;
}

static bool can_revive(
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

static bool target_select_impl(
    const HTHPlayerLifecycleRuntime *lifecycle,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    const HTHPlayerDeathSnapshot *death_snapshot,
    const HTHSpatialStore *spatial,
    HTHPlayerSlot reviver_slot,
    float revive_range,
    HTHPlayerSlot *out_target_slot,
    HTHReviveEligibilitySource source)
{
    const HTHPlayerRoster *roster;
    HTHSpatialTransform reviver_transform;
    HTHEntityHandle reviver;
    HTHPlayerSlot best_slot = HTH_PLAYER_SLOT_INVALID;
    HTHPlayerSlot slot;
    double best_distance_squared = 0.0;
    double range_squared;
    size_t current_count = 0U;
    size_t occupied_count;
    bool reviver_dead;
    bool reviver_present;
    bool found = false;

    if (out_target_slot != NULL) {
        *out_target_slot = HTH_PLAYER_SLOT_INVALID;
    }
    if (lifecycle == NULL || entities == NULL || actors == NULL ||
        spatial == NULL || out_target_slot == NULL ||
        reviver_slot >= HTH_MAX_PLAYERS || !isfinite(revive_range) ||
        revive_range <= 0.0F ||
        (source == HTH_REVIVE_ELIGIBILITY_LIVE && health == NULL) ||
        (source == HTH_REVIVE_ELIGIBILITY_SNAPSHOT &&
         death_snapshot == NULL) ||
        (source != HTH_REVIVE_ELIGIBILITY_LIVE &&
         source != HTH_REVIVE_ELIGIBILITY_SNAPSHOT)) {
        return false;
    }
    range_squared = (double)revive_range * (double)revive_range;
    roster = hth_player_lifecycle_runtime_get_roster(lifecycle);
    if (!isfinite(range_squared) || roster == NULL ||
        !hth_player_roster_get_slot(roster, entities, actors, reviver_slot,
                                    &reviver) ||
        (source == HTH_REVIVE_ELIGIBILITY_LIVE &&
         !hth_health_store_has(health, entities, actors, reviver)) ||
        (source == HTH_REVIVE_ELIGIBILITY_SNAPSHOT &&
         (!hth_player_death_snapshot_query(
              death_snapshot, reviver_slot, reviver,
              &reviver_present, &reviver_dead) ||
          !reviver_present)) ||
        !hth_spatial_store_get(spatial, entities, reviver,
                               &reviver_transform)) {
        return false;
    }

    occupied_count = hth_player_roster_count(roster);
    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        HTHSpatialTransform candidate_transform;
        HTHEntityHandle candidate;
        double candidate_distance_squared;
        bool eligible;

        if (!hth_player_roster_get_slot(roster, entities, actors, slot,
                                        &candidate)) {
            continue;
        }
        current_count++;
        if (slot == reviver_slot) {
            continue;
        }
        if (!can_revive(lifecycle, entities, actors, health,
                        death_snapshot, source, reviver_slot, slot,
                        &eligible)) {
            return false;
        }
        if (!eligible) {
            continue;
        }
        if (!hth_spatial_store_get(spatial, entities, candidate,
                                   &candidate_transform) ||
            !distance_squared(reviver_transform.position,
                              candidate_transform.position,
                              &candidate_distance_squared)) {
            return false;
        }
        if (candidate_distance_squared > range_squared) {
            continue;
        }
        if (!found || candidate_distance_squared < best_distance_squared ||
            (candidate_distance_squared == best_distance_squared &&
             slot < best_slot)) {
            found = true;
            best_slot = slot;
            best_distance_squared = candidate_distance_squared;
        }
    }
    if (current_count != occupied_count) {
        return false;
    }
    if (found) {
        *out_target_slot = best_slot;
    }
    return true;
}

bool hth_player_revive_target_select(
    const HTHPlayerLifecycleRuntime *lifecycle,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    const HTHSpatialStore *spatial,
    HTHPlayerSlot reviver_slot,
    float revive_range,
    HTHPlayerSlot *out_target_slot)
{
    return target_select_impl(
        lifecycle, entities, actors, health, NULL, spatial, reviver_slot,
        revive_range, out_target_slot, HTH_REVIVE_ELIGIBILITY_LIVE);
}

bool hth_player_revive_target_select_snapshot(
    const HTHPlayerLifecycleRuntime *lifecycle,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHPlayerDeathSnapshot *death_snapshot,
    const HTHSpatialStore *spatial,
    HTHPlayerSlot reviver_slot,
    float revive_range,
    HTHPlayerSlot *out_target_slot)
{
    return target_select_impl(
        lifecycle, entities, actors, NULL, death_snapshot, spatial,
        reviver_slot, revive_range, out_target_slot,
        HTH_REVIVE_ELIGIBILITY_SNAPSHOT);
}
