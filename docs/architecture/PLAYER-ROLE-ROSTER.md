# Player Role / Roster Foundation

Hertharian v0.3.29 defines Player as an explicit gameplay role. A current
usable Player is a live Entity with an Actor association and explicit
membership in one `HTHPlayerRoster`. Actor, Player Target Bridge, and
PlayerBody do not independently confer the Player role.

## Caller-Owned Fixed Roster

The private roster permits zero through four registered Players. Four is the
frozen product maximum for this foundation; a future Session may require at
least one Player. The roster is a caller-owned value with no allocation,
Store, global state, or stored count:

```c
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
```

The explicit occupancy bit is structural metadata only. It allows `{0}` to be
the canonical empty roster without inventing a second Entity-handle sentinel;
the Entity field has no semantics while its entry is unoccupied. Occupancy is
not active, alive, connected, local, remote, Downed, or Defeated state.

Slots are stable and sparse. Register chooses the lowest free slot, rejects an
exact-handle duplicate and rejects a fifth Player without mutation. Unregister
clears only the selected membership and never compacts later slots. Reset is
null-safe and clears all membership without destroying or modifying external
objects. Count scans occupancy, so an occupied stale, dead, Downed, or Defeated
entry continues to count until explicitly unregistered or reset.

## Current Player Validation

Registration requires a live Entity and Actor. Spatial and Health are not
Player-role requirements: their absence represents lack of readiness for the
systems that consume them, not loss of roster membership. Likewise, removing
Actor or destroying Entity after registration does not auto-unregister.
`get_slot` and `find_entity` return only a current live Entity+Actor Player;
they fail canonically for stale or missing-Actor entries while leaving the
slot occupied for cleanup. Unregister needs no Registry or Actor Store and can
therefore remove a stale entry.

Every comparison uses the complete Entity index and generation. Reusing an
Entity index cannot transfer Player membership to the new generation. Two
occupied entries with the same exact handle, or an occupied handle with
`index == UINT32_MAX` or generation zero, are malformed local state. Register,
get, and find reject such a roster without mutation. A historically valid
stale handle is not malformed. Reset recovers any non-null roster.

PlayerSlot is roster/session identity; Entity handle is the current gameplay
avatar identity. v0.3.29 associates the two without claiming permanent
equivalence. Future Respawn may add an explicit rebind operation. Iteration
needs no iterator object: callers scan slots from zero to
`HTH_MAX_PLAYERS - 1` and use `get_slot`.

## Ownership Boundary

The roster owns membership and slot identity only. It does not know or own
PlayerBody, Player Target Bridge, Spatial, Health, Player Death, Player
Defeat, Player Revive Window, Input, Camera, networking, Session IDs, Team IDs,
characters, inventory, weapons, capabilities, or progression. It consumes
only the gameplay Entity identity produced by a Bridge when that composition
is used. Defeat and Revive Window remain caller-owned and may later be
associated externally by PlayerSlot.

For base single-team cooperative play, one roster is one membership domain;
future multi-team policy can map teams externally. Player slots can likewise
become external anchors for Revive Eligibility, Session Outcome, inventory,
and progression without putting those systems inside the roster.

Reset, register, count, and find are `O(P)` for `P <= 4`; get and unregister
are `O(1)`. Heap allocation is zero. v0.3.29 is deliberately disconnected:
Engine and Bootstrap own no roster, make no calls, and perform no roster work
per frame.

As of v0.3.30, the disconnected Player Revive Eligibility query consumes two
slots in one roster. Roster membership supplies Player identity and the
same-roster cooperative domain; Eligibility does not mutate membership.
