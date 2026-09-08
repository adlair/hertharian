# Player Defeat Foundation

Hertharian v0.3.27 distinguishes current Player Death from persistent Player
Defeat. Death remains derived from current Health and reverses when Health
becomes positive. Defeat is an independent caller-owned lifecycle fact that is
set and cleared only by explicit operations.

## State and Operations

The private foundation contains exactly one semantic field:

```c
typedef struct HTHPlayerDefeatState {
    bool defeated;
} HTHPlayerDefeatState;
```

A zero-initialized or reset state is not defeated. Reset accepts null as a
no-op. Mark fails for null, otherwise changes false to true; marking an already
defeated state succeeds without changing it. Between resets, false to true is
the only transition.

The query canonicalizes a supplied output to false before validation. Null
state or output is a technical failure. A valid query copies the current
defeated fact without consuming or changing it. Copying the struct by value
copies only that fact, and separate instances have no shared state.

## Death and Health Independence

Player Defeat does not include or call Player Death and has no Entity, Actor,
Health, Spatial, PlayerBody, Player Target Bridge, clock, Input, Camera,
Targeting, or Enemy dependency. It neither reads nor mutates Health. There is
no automatic Death-to-Defeat transition and healing never resets Defeat.

All four combinations are legal:

| Defeat | Health | Meaning at this boundary |
|---|---|---|
| false | positive | alive and not permanently defeated |
| false | zero | currently dead, not marked permanently defeated |
| true | zero | defeated with zero current life |
| true | positive | numerical life restored while defeat remains persistent |

The last combination is not an error. Future runtime policy will decide
participation precedence. Reset only removes Defeat; it does not heal, revive,
respawn, recreate identity, or mutate any other system. Mark does not damage or
kill the Player.

## Ownership and Future Composition

State is caller-owned and contains no Player or Entity handle. Its owner must
reset it before associating the same instance with a different logical Player
identity. The primitive adds no PlayerStore, Defeat Store, lifecycle enum,
capacity, generation map, allocation, static mutable state, or global
singleton. A future cooperative owner can hold independent instances for
multiple Players.

Downed, revive eligibility, ReviveWindow, timers, automatic defeat triggers,
respawn, Game Over, Session Outcome, victory, roster aggregation, and runtime
participation remain deferred. In particular, this foundation does not define
`dead && !defeated` as Downed or revivable.

v0.3.27 is disconnected: production has no state instance, caller, or per-frame
work. Reset, mark, and query are deterministic O(1) operations, use O(1)
auxiliary memory, and perform zero heap allocations. The module is private and
adds no public API.

As of v0.3.28, the separate Player Revive Window timer is also a disconnected
foundation. Its expiry result does not call or otherwise mutate Player Defeat;
a future lifecycle adapter will own that policy composition.

As of v0.3.29, Player Roster supplies a future per-Player slot identity but
does not own or reset Defeat. Lifecycle state remains externally associated
and caller-owned.

As of v0.3.30, Player Revive Eligibility queries caller-bound reviver and
target Defeat states read-only. Eligibility never marks or resets Defeat, and
the caller remains responsible for associating each state with its slot.

As of v0.3.31, ordinary Player Revive Execution revalidates both states
read-only through Eligibility and never resets permanent Defeat. Successful
execution changes only target Health and target ReviveWindow.
