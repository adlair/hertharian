# Player Death Snapshot Foundation

Hertharian v0.3.39 introduces a private, disconnected observation of Player
Death across the fixed Player population. It samples the released Player Death
authority exactly once for each current Player during a build and preserves the
result as a caller-owned value. It adds no Engine integration, runtime consumer,
or per-frame work in this release.

## Authority and Scope

```text
Entity + Actor + Health
        |
        v
hth_player_death_is_dead()
        |
        v
HTHPlayerDeathSnapshot
```

PLAYER DEATH SNAPSHOT IS A FRAME OBSERVATION, NOT A NEW AUTHORITY. Player
Death, derived from valid current Health, remains authoritative. The builder
calls `hth_player_death_is_dead()` and never compares or stores Health values.

The snapshot contains only presence, exact Entity identity, and the sampled
Death boolean. Defeat, Revive Window, Spatial, movement, Input, PlayerBody,
Camera, Enemy state, frame number, timestamp, and population epoch remain
outside it. It is Death-specific rather than a generic Player frame state.

## Fixed Value and Identity

```c
typedef struct HTHPlayerDeathSnapshotEntry {
    HTHEntityHandle entity;
    bool present;
    bool dead;
} HTHPlayerDeathSnapshotEntry;

typedef struct HTHPlayerDeathSnapshot {
    HTHPlayerDeathSnapshotEntry entries[HTH_MAX_PLAYERS];
} HTHPlayerDeathSnapshot;
```

The array index is the `HTHPlayerSlot`; entries are never compacted or sorted.
Capacity comes directly from `HTH_MAX_PLAYERS`, currently four. Empty and
sparse rosters are valid. `{0}` is the canonical empty snapshot: every entry is
absent and not dead, and an absent entry's Entity field has no semantic meaning.

An applicable fact is identified by both its slot and complete
`HTHEntityHandle`, including generation. A fact captured for an old occupant
never applies to a replacement in the same slot, even when the Entity index is
reused. No roster epoch is needed.

## Transactional Build

```c
bool hth_player_death_snapshot_build(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    HTHPlayerDeathSnapshot *out_snapshot);
```

The writable output is first canonicalized to empty. The builder scans the
canonical slots, skips unoccupied entries, resolves each occupied entry through
the Roster, and calls the Player Death query once. It constructs a local value
and publishes only after every current Player succeeds. A stale Entity, missing
Actor or Health, malformed Roster, duplicate current identity, or other Death
prerequisite failure rejects the complete build and exposes no partial entries.
An empty Roster succeeds.

For `P` current Players:

```text
SnapshotBuildDeathQueries = CurrentPlayerCount = P
0 <= P <= HTH_MAX_PLAYERS == 4
```

Empty slots cause no Death query. Build scans fixed capacity, so its conceptual
cost is O(P) over the bounded Player capacity and constant in the deployed
four-slot architecture. It uses O(1) bounded auxiliary state and no heap.

## Identity-Gated Query

```c
bool hth_player_death_snapshot_query(
    const HTHPlayerDeathSnapshot *snapshot,
    HTHPlayerSlot slot,
    HTHEntityHandle expected_entity,
    bool *out_present,
    bool *out_dead);
```

Writable outputs are canonicalized to false before validation. Null arguments,
an invalid slot, or a structurally invalid expected handle are technical
failures. An absent entry or complete-handle mismatch is semantic absence:
query succeeds with `present=false` and `dead=false`. This does not classify the
Player as alive; it means the snapshot has no applicable fact for that exact
identity. Exact slot and handle matches return `present=true` and the captured
Death value.

Query directly indexes the requested slot. It performs no scan, live Roster or
Health lookup, Death query, allocation, or source mutation, and is O(1).

## Immutability and Population Changes

There are no set, mark, invalidate, or per-slot refresh APIs. After a successful
build, the snapshot is immutable and copyable by assignment, with no internal
pointers. A caller rebuilds the complete value at the next observation boundary.
Freshness is caller-owned; the module has no clock or lifecycle.

Health and population changes after build do not rewrite the captured value:

- lethal damage leaves the old snapshot alive; rebuild observes dead;
- ordinary healing from zero leaves the old snapshot dead; rebuild observes
  alive;
- a newly registered Player is absent until rebuild;
- a despawned identity can remain recorded historically, but cannot match a
  replacement handle;
- slot reuse with a new generation makes the old fact non-applicable.

The future integration boundary is the point where Engine currently acquires
its local `player_dead`: after Input/orientation/delta and before Lifecycle,
Movement, and Enemy Pursuit. Player population should remain stable between
build and read-side consumption where possible. Generation matching remains a
defense against stale use.

## Revive Boundary

Future Lifecycle, Movement suppression, Enemy dead-target exclusion, Revive
Target Selection, and Revive Interaction read-side paths may share one snapshot
fact per Player. Defeat, Revive Window, and Spatial remain live state. This
allows a fresh sampled death to start a live Revive Window, and a window expiry
or Defeat transition to make the Player ineligible during the same frame.

Revive Execution is deliberately different: it must retain live Death and
eligibility revalidation immediately before mutating Health. This protects
ordinary healing after sampling, reviver death, identity changes, window
expiry/cancellation, and first-reviver-wins behavior. Correct mutation safety
overrides universal once-per-frame query purity.

Hertharian v0.3.40 adds disconnected snapshot-aware Eligibility, Target
Selection, and Interaction siblings. They consume this value without further
live Death queries, while Defeat, ReviveWindow, and Spatial remain live.
Revive Execution is unchanged and retains live commit-time validation. Engine
still has no snapshot-aware revive caller.

## Ownership and Dependencies

The module depends only on Roster/PlayerSlot, Entity identity, Actor and Health
stores required by Player Death, and the Player Death query. It has no Store,
manager, registry, mutable global, heap allocation, persistent history,
rollback, prediction, replication, networking, or public header.
