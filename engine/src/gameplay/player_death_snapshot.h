#ifndef HTH_PLAYER_DEATH_SNAPSHOT_H
#define HTH_PLAYER_DEATH_SNAPSHOT_H

#include "health.h"
#include "player_roster.h"

#include <stdbool.h>

typedef struct HTHPlayerDeathSnapshotEntry {
    HTHEntityHandle entity;
    bool present;
    bool dead;
} HTHPlayerDeathSnapshotEntry;

typedef struct HTHPlayerDeathSnapshot {
    HTHPlayerDeathSnapshotEntry entries[HTH_MAX_PLAYERS];
} HTHPlayerDeathSnapshot;

bool hth_player_death_snapshot_build(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    HTHPlayerDeathSnapshot *out_snapshot);

bool hth_player_death_snapshot_query(
    const HTHPlayerDeathSnapshot *snapshot,
    HTHPlayerSlot slot,
    HTHEntityHandle expected_entity,
    bool *out_present,
    bool *out_dead);

#endif
