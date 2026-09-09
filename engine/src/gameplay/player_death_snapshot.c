#include "player_death_snapshot.h"

#include "player_death.h"

#include <stdint.h>

static bool handle_is_structurally_possible(HTHEntityHandle entity)
{
    return entity.index != UINT32_MAX && entity.generation != 0U;
}

bool hth_player_death_snapshot_build(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    HTHPlayerDeathSnapshot *out_snapshot)
{
    HTHPlayerDeathSnapshot snapshot = {0};
    HTHPlayerSlot slot;

    if (out_snapshot != NULL) {
        *out_snapshot = (HTHPlayerDeathSnapshot){0};
    }
    if (roster == NULL || entities == NULL || actors == NULL ||
        health == NULL || out_snapshot == NULL) {
        return false;
    }

    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        HTHEntityHandle entity;
        bool dead;

        if (!roster->entries[slot].occupied) {
            continue;
        }
        if (!hth_player_roster_get_slot(roster, entities, actors, slot,
                                        &entity) ||
            !hth_player_death_is_dead(entities, actors, health, entity,
                                      &dead)) {
            return false;
        }
        snapshot.entries[slot].entity = entity;
        snapshot.entries[slot].present = true;
        snapshot.entries[slot].dead = dead;
    }

    *out_snapshot = snapshot;
    return true;
}

bool hth_player_death_snapshot_query(
    const HTHPlayerDeathSnapshot *snapshot,
    HTHPlayerSlot slot,
    HTHEntityHandle expected_entity,
    bool *out_present,
    bool *out_dead)
{
    const HTHPlayerDeathSnapshotEntry *entry;

    if (out_present != NULL) {
        *out_present = false;
    }
    if (out_dead != NULL) {
        *out_dead = false;
    }
    if (snapshot == NULL || out_present == NULL || out_dead == NULL ||
        slot >= HTH_MAX_PLAYERS ||
        !handle_is_structurally_possible(expected_entity)) {
        return false;
    }

    entry = &snapshot->entries[slot];
    if (!entry->present ||
        !hth_entity_handle_equal(entry->entity, expected_entity)) {
        return true;
    }
    if (!handle_is_structurally_possible(entry->entity)) {
        return false;
    }

    *out_present = true;
    *out_dead = entry->dead;
    return true;
}
