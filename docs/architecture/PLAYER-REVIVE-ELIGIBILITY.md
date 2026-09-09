# Player Revive Eligibility Foundation

Hertharian v0.3.30 defines a private, stateless policy query that answers
whether one valid Player pair is semantically permitted to perform a Revive.
It does not begin an interaction or execute a Revive.

## Identity and Policy

Reviver and target are stable `HTHPlayerSlot` identities in one caller-owned
`HTHPlayerRoster`. That roster is the base cooperative membership domain, and
different valid slots establish the base teammate relation. An Actor outside
the roster is not a Player. The base policy rejects self-revive as a valid,
ineligible result.

The exact policy is:

```text
eligible =
    reviver_slot != target_slot
    && reviver current Player
    && target current Player
    && !reviver_dead
    && !reviver_defeated
    && target_dead
    && !target_defeated
    && target_window_active
```

Thus an active reviver is a current roster Player that is neither dead nor
defeated. A Downed target at this boundary is a current roster Player that is
dead, not defeated, and has an active ReviveWindow. Downed is derived; no
persistent Downed state or module is introduced.

## Query Contract

```c
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
    bool *out_eligible);
```

The return value is technical status. The output is canonicalized to false
before validation. Technical failure therefore returns false with a false
output; technical success with a false output is a valid policy rejection;
technical success with a true output is an eligible pair.

Null dependencies, invalid or empty slots, malformed roster state, stale
Entities, removed Actors, missing or invalid Health, malformed ReviveWindow,
or failure of any authority query are technical failures. None triggers
automatic repair or unregister.

All technical dependencies are validated before policy evaluation. The query
resolves both slots with `hth_player_roster_get_slot()`, queries both Death
facts with `hth_player_death_is_dead()`, queries both Defeat facts with
`hth_player_defeat_is_defeated()`, and queries the target window with
`hth_player_revive_window_query()`. Only after all succeed does it combine the
policy predicates. Consequently self-pair plus malformed Health or Window is
a technical failure, and an already defeated reviver cannot hide a malformed
target window.

The target window's authoritative `active` result is the only timer value
used. Eligibility does not interpret `remaining_seconds` itself. Individually
valid but unusual compositions—such as an alive target with an active window,
a defeated target with an active window, or a defeated reviver with positive
Health—are valid evaluations that resolve as ineligible.

## Caller Binding and Ownership

Defeat and ReviveWindow remain caller-owned values without embedded Player
identity. The caller must bind `reviver_defeat` to `reviver_slot` and
`target_defeat` plus `target_window` to `target_slot`. This explicit
precondition follows their released ownership contracts. Eligibility adds no
PlayerLifecycle aggregate, parallel-array owner, Store, persistent state,
cache, or reason enum merely to shorten the signature.

The query reads existing authorities only. It never mutates roster membership,
Entity, Actor, Health, Defeat, or ReviveWindow. It has no PlayerBody, Bridge,
Spatial, distance, LOS, Collision, Input, Camera, Timing, Enemy, Session,
networking, inventory, or progression dependency.

Evaluation is deterministic and bounded `O(1)`, uses `O(1)` auxiliary memory,
and performs zero heap allocations. v0.3.30 is disconnected: Engine and
Bootstrap own no eligibility state, make no call, and perform no related work
per frame.

Player Revive Execution consumes an eligible pair and caller-owned revive
Health policy, while future Interaction may add distance, Input, hold progress,
interruption, and presentation. Lifecycle/Defeat Runtime Integration and
Session Outcome remain later boundaries. Self-revive items and other
capabilities belong to separate policy systems and do not alter this base
foundation.

As of v0.3.31, Player Revive Execution revalidates this query internally at
mutation time. It neither caches nor accepts an earlier Eligibility result as
authorization, and this pure policy remains unchanged.

As of v0.3.33, the disconnected Player Revive Interaction foundation evaluates
Eligibility on every relevant held step through a private Lifecycle Runtime
binding adapter. Execution still performs final revalidation at mutation time;
Interaction adds Spatial range and progress without duplicating this policy.

As of v0.3.36, disconnected Player Revive Target Selection calls that same
Lifecycle Runtime adapter for every current non-self candidate before any
candidate Spatial lookup. Eligibility remains the sole revive-policy authority;
Selection adds only range and deterministic nearest-PlayerSlot ranking.

As of v0.3.40, Death acquisition is separated from the single semantic policy
helper. The legacy query retains its live reviver-then-target Death queries. A
private snapshot-aware sibling resolves both current Roster identities and
queries `HTHPlayerDeathSnapshot` with slot plus complete Entity handle, then
uses the same policy. A current identity absent or mismatched in the supplied
snapshot is a technical coherence failure, not ordinary ineligibility. Defeat
and the target ReviveWindow remain live on every evaluation.
