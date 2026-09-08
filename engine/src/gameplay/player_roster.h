#ifndef HTH_PLAYER_ROSTER_H
#define HTH_PLAYER_ROSTER_H

#include "actor.h"
#include "entity.h"

#include <stdbool.h>
#include <stddef.h>

enum { HTH_MAX_PLAYERS = 4 };

typedef size_t HTHPlayerSlot;

#define HTH_PLAYER_SLOT_INVALID ((HTHPlayerSlot)HTH_MAX_PLAYERS)

typedef struct HTHPlayerRosterEntry {
    HTHEntityHandle entity;
    bool occupied;
} HTHPlayerRosterEntry;

typedef struct HTHPlayerRoster {
    HTHPlayerRosterEntry entries[HTH_MAX_PLAYERS];
} HTHPlayerRoster;

void hth_player_roster_reset(HTHPlayerRoster *roster);
bool hth_player_roster_register(
    HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHEntityHandle entity,
    HTHPlayerSlot *out_slot);
bool hth_player_roster_unregister(
    HTHPlayerRoster *roster,
    HTHPlayerSlot slot);
size_t hth_player_roster_count(const HTHPlayerRoster *roster);
bool hth_player_roster_get_slot(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHPlayerSlot slot,
    HTHEntityHandle *out_entity);
bool hth_player_roster_find_entity(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHEntityHandle entity,
    HTHPlayerSlot *out_slot);

#endif
