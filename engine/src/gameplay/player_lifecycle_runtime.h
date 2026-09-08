#ifndef HTH_PLAYER_LIFECYCLE_RUNTIME_H
#define HTH_PLAYER_LIFECYCLE_RUNTIME_H

#include "player_defeat.h"
#include "player_revive_execution.h"
#include "player_revive_window.h"
#include "player_roster.h"

#include <stdbool.h>

typedef struct HTHPlayerLifecycleRuntime {
    HTHPlayerRoster roster;
    HTHPlayerDefeatState defeat[HTH_MAX_PLAYERS];
    HTHPlayerReviveWindow revive_windows[HTH_MAX_PLAYERS];
    bool was_dead[HTH_MAX_PLAYERS];
} HTHPlayerLifecycleRuntime;

void hth_player_lifecycle_runtime_reset(
    HTHPlayerLifecycleRuntime *runtime);
bool hth_player_lifecycle_runtime_register(
    HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHEntityHandle entity,
    HTHPlayerSlot *out_slot);
bool hth_player_lifecycle_runtime_unregister(
    HTHPlayerLifecycleRuntime *runtime,
    HTHPlayerSlot slot);
bool hth_player_lifecycle_runtime_step_player(
    HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHPlayerSlot slot,
    bool dead_snapshot,
    double delta_seconds,
    const double *cooperative_revive_window_duration_seconds);

const HTHPlayerRoster *hth_player_lifecycle_runtime_get_roster(
    const HTHPlayerLifecycleRuntime *runtime);
bool hth_player_lifecycle_runtime_get_defeat(
    const HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHPlayerSlot slot,
    const HTHPlayerDefeatState **out_defeat);
bool hth_player_lifecycle_runtime_get_revive_window(
    const HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHPlayerSlot slot,
    const HTHPlayerReviveWindow **out_window);
bool hth_player_lifecycle_runtime_can_revive(
    const HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    HTHPlayerSlot reviver_slot,
    HTHPlayerSlot target_slot,
    bool *out_eligible);
bool hth_player_lifecycle_runtime_execute_revive(
    HTHPlayerLifecycleRuntime *runtime,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHHealthStore *health,
    HTHPlayerSlot reviver_slot,
    HTHPlayerSlot target_slot,
    float revive_health,
    bool *out_revived);

#endif
