# Multi-Player Dead Target Exclusion Foundation

Hertharian v0.3.35 generalizes the private Enemy targeting boundary from one
optional excluded Entity handle to a caller-owned list. This is a disconnected
foundation: production continues to supply one Player candidate and zero or
one exclusion derived from its existing once-per-frame Death snapshot.

## Contract

Target Selection and Pursuit Runtime receive:

```text
const HTHEntityHandle *excluded_targets
size_t excluded_target_count
```

Count zero accepts a null or non-null pointer. Positive count requires a
non-null pointer. Neither operation copies, mutates, retains, validates, or
owns exclusion entries. Membership compares the complete Entity index and
generation. Invalid and stale handles are innocuous keys; duplicates are
idempotent; ordering cannot influence candidate ranking.

Selection excludes a matching candidate before Spatial lookup, Perception,
LOS, distance, and tie evaluation. A valid query with every candidate excluded
is a successful no-target result and preserves the existing Selection
transaction contract. Non-Player candidates, invalid candidate skipping,
minimum 3D squared-distance ranking, lower-index exact ties, Perception, and LOS
retain their released semantics.

Pursuit advances each Enemy's cadence, clears a Current Target matching any
exclusion before the missing-Spatial early-out, and passes the same list into
Selection. A nonmatching Current Target—including a non-Player target—is
retained under the historical policy. Clearing does not reset cadence.

## Player Policy Composition

Death remains the authority that decides whether a Player handle belongs in
the list. Defeat is orthogonal. Selection and Pursuit import no PlayerRoster,
PlayerLifecycleRuntime, Health, Player Death, Defeat, or Revive authority.
Healing is represented by a later Death snapshot omitting that handle; there
is no sticky exclusion state.

The focused integration test creates four real gameplay Players through Player
Runtime Population, damages P1 and P3 through Health, queries one Death snapshot
per Player, and constructs `[P1, P3]`. Only alive P0/P2 compete and historical
ranking selects the winner. Healing P1 makes it eligible after a new snapshot;
excluding all four yields valid no-target and clears an excluded Current
Target. An alive-but-defeated Player remains eligible.

## Ownership and Cost

For `N` candidates and `E` exclusions, Selection membership work is `O(N*E)`.
Pursuit Current Target membership is `O(E)` per Enemy, followed when needed by
the existing Selection and Decision/Chase work. Auxiliary memory is `O(1)` and
the exclusion plumbing performs zero heap allocations. There is no
ExclusionStore, callback filter framework, global mutable state, Player scan,
Death snapshot batch, fake Player, or new production per-frame work.

Multi-Player Engine integration, PlayerBody arrays, Input routing, networking,
sessions, revive targeting, and later cooperative gameplay remain deferred.
