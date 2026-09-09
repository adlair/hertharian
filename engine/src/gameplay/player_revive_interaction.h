#ifndef HTH_PLAYER_REVIVE_INTERACTION_H
#define HTH_PLAYER_REVIVE_INTERACTION_H

#include "player_lifecycle_runtime.h"
#include "player_death_snapshot.h"
#include "spatial.h"

#include <stdbool.h>

typedef struct HTHPlayerReviveInteraction {
    HTHPlayerSlot reviver_slot;
    HTHEntityHandle reviver_entity;
    HTHPlayerSlot target_slot;
    HTHEntityHandle target_entity;
    double elapsed_seconds;
    double required_seconds;
    bool active;
} HTHPlayerReviveInteraction;

void hth_player_revive_interaction_reset(
    HTHPlayerReviveInteraction *interaction);
bool hth_player_revive_interaction_query(
    const HTHPlayerReviveInteraction *interaction,
    bool *out_active,
    HTHPlayerSlot *out_target_slot,
    double *out_elapsed_seconds,
    double *out_required_seconds);
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
    bool *out_revived);
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
    bool *out_revived);

#endif
