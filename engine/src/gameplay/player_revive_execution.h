#ifndef HTH_PLAYER_REVIVE_EXECUTION_H
#define HTH_PLAYER_REVIVE_EXECUTION_H

#include "player_revive_eligibility.h"

#include <stdbool.h>

bool hth_player_revive_execute(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHHealthStore *health,
    HTHPlayerSlot reviver_slot,
    const HTHPlayerDefeatState *reviver_defeat,
    HTHPlayerSlot target_slot,
    const HTHPlayerDefeatState *target_defeat,
    HTHPlayerReviveWindow *target_window,
    float revive_health,
    bool *out_revived);

#endif
