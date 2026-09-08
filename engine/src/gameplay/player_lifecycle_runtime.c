#include "player_lifecycle_runtime.h"

#include <math.h>
#include <stddef.h>

static void reset_slot(HTHPlayerLifecycleRuntime *runtime,
                       HTHPlayerSlot slot)
{
    hth_player_defeat_reset(&runtime->defeat[slot]);
    hth_player_revive_window_reset(&runtime->revive_windows[slot]);
    runtime->was_dead[slot] = false;
}

static bool mark_defeated(HTHPlayerLifecycleRuntime *runtime,
                          HTHPlayerSlot slot)
{
    return hth_player_defeat_mark(&runtime->defeat[slot]);
}

static bool current_player_count(
    const HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    size_t *out_count)
{
    size_t current_count = 0U;
    size_t occupied_count;
    HTHPlayerSlot slot;

    if (runtime == NULL || entities == NULL || actors == NULL ||
        out_count == NULL) {
        return false;
    }
    occupied_count = hth_player_roster_count(&runtime->roster);
    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        HTHEntityHandle entity;

        if (hth_player_roster_get_slot(
                &runtime->roster, entities, actors, slot, &entity)) {
            current_count++;
        }
    }
    if (current_count != occupied_count) {
        return false;
    }
    *out_count = current_count;
    return true;
}

static bool validate_binding(
    const HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHPlayerSlot slot)
{
    HTHEntityHandle entity;

    return runtime != NULL &&
           hth_player_roster_get_slot(
               &runtime->roster, entities, actors, slot, &entity);
}

void hth_player_lifecycle_runtime_reset(
    HTHPlayerLifecycleRuntime *runtime)
{
    HTHPlayerSlot slot;

    if (runtime == NULL) {
        return;
    }
    hth_player_roster_reset(&runtime->roster);
    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        reset_slot(runtime, slot);
    }
}

bool hth_player_lifecycle_runtime_register(
    HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHEntityHandle entity,
    HTHPlayerSlot *out_slot)
{
    HTHPlayerSlot slot;

    if (out_slot != NULL) {
        *out_slot = HTH_PLAYER_SLOT_INVALID;
    }
    if (runtime == NULL || out_slot == NULL ||
        !hth_player_roster_register(
            &runtime->roster, entities, actors, entity, &slot)) {
        return false;
    }
    reset_slot(runtime, slot);
    *out_slot = slot;
    return true;
}

bool hth_player_lifecycle_runtime_unregister(
    HTHPlayerLifecycleRuntime *runtime,
    HTHPlayerSlot slot)
{
    if (runtime == NULL ||
        !hth_player_roster_unregister(&runtime->roster, slot)) {
        return false;
    }
    reset_slot(runtime, slot);
    return true;
}

bool hth_player_lifecycle_runtime_step_player(
    HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHPlayerSlot slot,
    bool dead_snapshot,
    double delta_seconds,
    const double *cooperative_revive_window_duration_seconds)
{
    size_t player_count;
    double remaining_seconds;
    bool defeated;
    bool window_active;
    bool window_expired;

    if (!isfinite(delta_seconds) || delta_seconds < 0.0 ||
        !validate_binding(runtime, entities, actors, slot) ||
        !current_player_count(runtime, entities, actors, &player_count) ||
        player_count == 0U ||
        !hth_player_defeat_is_defeated(
            &runtime->defeat[slot], &defeated) ||
        !hth_player_revive_window_query(
            &runtime->revive_windows[slot], &window_active,
            &remaining_seconds)) {
        return false;
    }

    if (!dead_snapshot) {
        if (window_active) {
            hth_player_revive_window_reset(&runtime->revive_windows[slot]);
        }
        runtime->was_dead[slot] = false;
        return true;
    }
    if (defeated) {
        if (window_active) {
            hth_player_revive_window_reset(&runtime->revive_windows[slot]);
        }
        runtime->was_dead[slot] = true;
        return true;
    }
    if (!runtime->was_dead[slot]) {
        if (player_count == 1U) {
            if (!mark_defeated(runtime, slot)) {
                return false;
            }
        } else {
            if (cooperative_revive_window_duration_seconds == NULL ||
                !isfinite(*cooperative_revive_window_duration_seconds) ||
                *cooperative_revive_window_duration_seconds <= 0.0 ||
                !hth_player_revive_window_begin(
                    &runtime->revive_windows[slot],
                    *cooperative_revive_window_duration_seconds)) {
                return false;
            }
        }
        runtime->was_dead[slot] = true;
        return true;
    }
    if (!window_active) {
        if (!mark_defeated(runtime, slot)) {
            return false;
        }
        runtime->was_dead[slot] = true;
        return true;
    }
    if (!hth_player_revive_window_advance(
            &runtime->revive_windows[slot], delta_seconds,
            &window_expired)) {
        return false;
    }
    if (window_expired && !mark_defeated(runtime, slot)) {
        return false;
    }
    runtime->was_dead[slot] = true;
    return true;
}

const HTHPlayerRoster *hth_player_lifecycle_runtime_get_roster(
    const HTHPlayerLifecycleRuntime *runtime)
{
    return runtime != NULL ? &runtime->roster : NULL;
}

bool hth_player_lifecycle_runtime_get_defeat(
    const HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHPlayerSlot slot,
    const HTHPlayerDefeatState **out_defeat)
{
    if (out_defeat != NULL) {
        *out_defeat = NULL;
    }
    if (out_defeat == NULL ||
        !validate_binding(runtime, entities, actors, slot)) {
        return false;
    }
    *out_defeat = &runtime->defeat[slot];
    return true;
}

bool hth_player_lifecycle_runtime_get_revive_window(
    const HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHPlayerSlot slot,
    const HTHPlayerReviveWindow **out_window)
{
    if (out_window != NULL) {
        *out_window = NULL;
    }
    if (out_window == NULL ||
        !validate_binding(runtime, entities, actors, slot)) {
        return false;
    }
    *out_window = &runtime->revive_windows[slot];
    return true;
}
