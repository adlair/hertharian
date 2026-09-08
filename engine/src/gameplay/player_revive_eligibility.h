#ifndef HTH_PLAYER_REVIVE_ELIGIBILITY_H
#define HTH_PLAYER_REVIVE_ELIGIBILITY_H

#include "health.h"
#include "player_defeat.h"
#include "player_revive_window.h"
#include "player_roster.h"

#include <stdbool.h>

bool hth_player_revive_eligibility_evaluate(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    HTHPlayerSlot reviver_slot,
    const HTHPlayerDefeatState *reviver_defeat,
    HTHPlayerSlot target_slot,
    const HTHPlayerDefeatState *target_defeat,
    const HTHPlayerReviveWindow *target_window,
    bool *out_eligible);

#endif
