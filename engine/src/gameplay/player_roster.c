#include "player_roster.h"

#include <stdint.h>
#include <string.h>

static bool handle_is_structurally_possible(HTHEntityHandle entity)
{
    return entity.index != UINT32_MAX && entity.generation != 0U;
}

static bool roster_is_valid(const HTHPlayerRoster *roster)
{
    HTHPlayerSlot left;

    if (roster == NULL) {
        return false;
    }
    for (left = 0U; left < HTH_MAX_PLAYERS; ++left) {
        HTHPlayerSlot right;

        if (!roster->entries[left].occupied) {
            continue;
        }
        if (!handle_is_structurally_possible(
                roster->entries[left].entity)) {
            return false;
        }
        for (right = left + 1U; right < HTH_MAX_PLAYERS; ++right) {
            if (roster->entries[right].occupied &&
                hth_entity_handle_equal(roster->entries[left].entity,
                                        roster->entries[right].entity)) {
                return false;
            }
        }
    }
    return true;
}

void hth_player_roster_reset(HTHPlayerRoster *roster)
{
    if (roster != NULL) {
        memset(roster, 0, sizeof(*roster));
    }
}

bool hth_player_roster_register(
    HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHEntityHandle entity,
    HTHPlayerSlot *out_slot)
{
    HTHPlayerSlot first_free = HTH_PLAYER_SLOT_INVALID;
    HTHPlayerSlot slot;

    if (out_slot != NULL) {
        *out_slot = HTH_PLAYER_SLOT_INVALID;
    }
    if (!roster_is_valid(roster) || entities == NULL || actors == NULL ||
        out_slot == NULL ||
        !hth_entity_registry_is_alive(entities, entity) ||
        !hth_actor_store_has(actors, entities, entity)) {
        return false;
    }
    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        if (roster->entries[slot].occupied) {
            if (hth_entity_handle_equal(roster->entries[slot].entity,
                                        entity)) {
                return false;
            }
        } else if (first_free == HTH_PLAYER_SLOT_INVALID) {
            first_free = slot;
        }
    }
    if (first_free == HTH_PLAYER_SLOT_INVALID) {
        return false;
    }
    roster->entries[first_free].entity = entity;
    roster->entries[first_free].occupied = true;
    *out_slot = first_free;
    return true;
}

bool hth_player_roster_unregister(HTHPlayerRoster *roster,
                                  HTHPlayerSlot slot)
{
    if (roster == NULL || slot >= HTH_MAX_PLAYERS ||
        !roster->entries[slot].occupied) {
        return false;
    }
    memset(&roster->entries[slot], 0, sizeof(roster->entries[slot]));
    return true;
}

size_t hth_player_roster_count(const HTHPlayerRoster *roster)
{
    size_t count = 0U;
    HTHPlayerSlot slot;

    if (roster == NULL) {
        return 0U;
    }
    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        if (roster->entries[slot].occupied) {
            count++;
        }
    }
    return count;
}

bool hth_player_roster_get_slot(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHPlayerSlot slot,
    HTHEntityHandle *out_entity)
{
    if (out_entity != NULL) {
        *out_entity = hth_entity_handle_invalid();
    }
    if (!roster_is_valid(roster) || entities == NULL || actors == NULL ||
        out_entity == NULL || slot >= HTH_MAX_PLAYERS ||
        !roster->entries[slot].occupied ||
        !hth_entity_registry_is_alive(entities,
                                      roster->entries[slot].entity) ||
        !hth_actor_store_has(actors, entities,
                             roster->entries[slot].entity)) {
        return false;
    }
    *out_entity = roster->entries[slot].entity;
    return true;
}

bool hth_player_roster_find_entity(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHEntityHandle entity,
    HTHPlayerSlot *out_slot)
{
    HTHPlayerSlot slot;

    if (out_slot != NULL) {
        *out_slot = HTH_PLAYER_SLOT_INVALID;
    }
    if (!roster_is_valid(roster) || entities == NULL || actors == NULL ||
        out_slot == NULL ||
        !hth_entity_registry_is_alive(entities, entity) ||
        !hth_actor_store_has(actors, entities, entity)) {
        return false;
    }
    for (slot = 0U; slot < HTH_MAX_PLAYERS; ++slot) {
        if (roster->entries[slot].occupied &&
            hth_entity_handle_equal(roster->entries[slot].entity, entity)) {
            *out_slot = slot;
            return true;
        }
    }
    return false;
}
