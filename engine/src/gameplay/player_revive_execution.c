#include "player_revive_execution.h"

#include <math.h>

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
    bool *out_revived)
{
    HTHEntityHandle target;
    HTHHealingResult healing_result;
    bool eligible;

    if (out_revived != NULL) {
        *out_revived = false;
    }
    if (roster == NULL || entities == NULL || actors == NULL ||
        health == NULL || reviver_defeat == NULL || target_defeat == NULL ||
        target_window == NULL || out_revived == NULL ||
        !isfinite(revive_health) || revive_health <= 0.0F) {
        return false;
    }
    if (!hth_player_revive_eligibility_evaluate(
            roster, entities, actors, health, reviver_slot,
            reviver_defeat, target_slot, target_defeat, target_window,
            &eligible)) {
        return false;
    }
    if (!eligible) {
        return true;
    }
    if (!hth_player_roster_get_slot(roster, entities, actors,
                                    target_slot, &target) ||
        !hth_health_store_apply_healing(
            health, entities, actors, target, revive_health,
            &healing_result)) {
        return false;
    }

    hth_player_revive_window_reset(target_window);
    *out_revived = true;
    return true;
}
