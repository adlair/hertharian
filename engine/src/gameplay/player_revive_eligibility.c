#include "player_revive_eligibility.h"

#include "player_death.h"

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
    bool *out_eligible)
{
    HTHEntityHandle reviver;
    HTHEntityHandle target;
    double target_window_remaining;
    bool reviver_dead;
    bool target_dead;
    bool reviver_defeated;
    bool target_defeated;
    bool target_window_active;

    if (out_eligible != NULL) {
        *out_eligible = false;
    }
    if (roster == NULL || entities == NULL || actors == NULL ||
        health == NULL || reviver_defeat == NULL || target_defeat == NULL ||
        target_window == NULL || out_eligible == NULL) {
        return false;
    }
    if (!hth_player_roster_get_slot(roster, entities, actors,
                                    reviver_slot, &reviver) ||
        !hth_player_roster_get_slot(roster, entities, actors,
                                    target_slot, &target) ||
        !hth_player_death_is_dead(entities, actors, health, reviver,
                                  &reviver_dead) ||
        !hth_player_death_is_dead(entities, actors, health, target,
                                  &target_dead) ||
        !hth_player_defeat_is_defeated(reviver_defeat,
                                       &reviver_defeated) ||
        !hth_player_defeat_is_defeated(target_defeat,
                                       &target_defeated) ||
        !hth_player_revive_window_query(target_window,
                                        &target_window_active,
                                        &target_window_remaining)) {
        return false;
    }

    *out_eligible = reviver_slot != target_slot &&
                    !reviver_dead && !reviver_defeated &&
                    target_dead && !target_defeated &&
                    target_window_active;
    return true;
}
