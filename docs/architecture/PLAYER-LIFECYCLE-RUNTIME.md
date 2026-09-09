# Player Lifecycle / Defeat Runtime Integration

Hertharian v0.3.32 composes the released Player Roster, Player Death, Player
Defeat, and Revive Window foundations into the first production Player
lifecycle runtime. The runtime is a private fixed-size value owned by Engine.
It introduces no public API and performs no heap allocation.

## Ownership and Slot Binding

`HTHPlayerLifecycleRuntime` contains one identity-only `HTHPlayerRoster`, one
`HTHPlayerDefeatState`, one `HTHPlayerReviveWindow`, and one `was_dead` bit per
`HTHPlayerSlot`. PlayerSlot is the binding authority for those parallel states.
The roster itself remains unchanged and owns membership only.

Zero initialization is canonical: the roster is empty, every Defeat state is
false, every Window is inactive with zero remainder, and every `was_dead` bit
is false. Registration delegates to PlayerRoster and canonicalizes the selected
slot. Unregistration first removes fallible roster membership, then resets the
slot's lifecycle values. Reusing a numerical slot therefore cannot inherit a
previous Player's lifecycle.

Engine registers the stable Entity produced by the existing Player Target
Bridge and stores the returned local PlayerSlot. The first empty roster makes
that slot numerically zero today, but zero has no local-Player semantics.
Production still owns exactly one `HTHPlayerBody`; no physical multiplayer,
second Camera, remote Player, or split-screen state is introduced.

## Derived State and Historical Edge

Downed remains derived:

```text
current Player && dead && !defeated && ReviveWindow active
```

There is no stored Downed flag or lifecycle phase enum. `was_dead[slot]` means
only that the authoritative Player Death snapshot processed by the previous
successful lifecycle step for that slot was dead. It detects the next
alive-to-dead edge without duplicating current Death authority. A failed step
does not commit `was_dead`, preserving retry behavior.

Each step validates finite nonnegative simulation delta and resolves the slot
through PlayerRoster, including current Entity generation and Actor
association. It also rejects a roster containing occupied membership that is
no longer current instead of counting stale entries as cooperative Players.

## Solo and Cooperative Episodes

On a fresh dead snapshot, one current roster Player is solo and becomes
permanently Defeated immediately. No solo Revive Window is created and no
arbitrary duration is needed.

With more than one current roster Player, a fresh dead snapshot begins one
Window using a caller/config-owned finite positive duration. The newly begun
Window retains its complete duration during that step; its first advance is
the next successful continuing-Downed step. Existing active Windows retain
their own remaining time and are not restarted by later duration inputs or
roster changes.

A continuing active Window advances exactly once with simulation delta. Exact
or overshooting expiry makes the Window inactive and marks Defeat in the same
operation. Continuing death with no Defeat and an inactive Window also marks
Defeat; it never restarts the opportunity. Defeated Players do not begin or
advance Windows, and any active Window found with Defeat is cancelled.

## Recovery and Persistent Defeat

An alive snapshot cancels any active Window and commits `was_dead=false`.
Lifecycle does not infer why Health became positive: generic healing and an
authorized future Revive are reconciled identically as current alive state.
This reconciliation does not authorize ordinary Downed healing; that remains
a separate gameplay policy.

Defeat is persistent and orthogonal to Health. An alive snapshot never clears
Defeat. Consequently Defeat plus positive Health remains a valid composition,
although current production has no ordinary Player healing path capable of
creating it. Movement and dead-target exclusion continue to depend only on
Player Death; defeated-Player participation and Session Outcome remain later
policies.

## Frame Snapshot and Future Revive

Engine retains exactly one local Player Death query per frame and passes that
same `player_dead` value to lifecycle, movement gating, and Bootstrap's
dead-target exclusion. The order is:

```text
Input and early-frame setup
→ effective simulation delta
→ authoritative local Player Death snapshot
→ local Player lifecycle step
→ Player Movement using the same snapshot
→ Player Target Bridge sync and Enemy Runtime using the same snapshot
→ rendering and frame completion
```

Enemy damage remains after the snapshot. Lethal damage in frame N therefore
enters the lifecycle in N+1; current solo production marks Defeat then. No
second Death query or post-damage rollback is added.

Player Revive Execution retains zero production callers. A future Revive
Interaction must complete its authorized Execution before the authoritative
Death snapshot. A successful Execution then restores Health and resets Window,
the snapshot observes alive, and lifecycle reconciliation preserves that
result before potential expiry. Interaction, range, LOS, Input hold/progress,
presentation, Session Outcome, Game Over, and respawn are outside v0.3.32.

As of v0.3.33, two private adapters bind runtime-owned Roster, Defeat, and
Window state to released Eligibility and Execution without exposing a mutable
Window or changing lifecycle ownership. The disconnected Revive Interaction
module consumes those adapters, but only tests invoke that module; Engine still
owns no interaction state and makes no runtime revive call.

Runtime storage is fixed `O(HTH_MAX_PLAYERS)`. Registration is bounded `O(P)`
for `P <= 4`; one slot step performs bounded work over at most four roster
slots and is `O(1)` for the frozen capacity. Current production invokes only
the local slot, so lifecycle adds fixed `O(1)` work per frame.

As of v0.3.34, disconnected Player Runtime Population uses the existing
register/unregister boundary transactionally. Lifecycle Runtime remains the
owner of Roster, Defeat, Window, and `was_dead`; Population stores no duplicate
membership or lifecycle state.

As of v0.3.36, disconnected Player Revive Target Selection uses the existing
`can_revive` adapter as its sole policy authority while ranking current roster
slots spatially. Lifecycle Runtime gains no selection state, adapter, or
accessor, and Engine adds no selection caller. See
`PLAYER-REVIVE-TARGET-SELECTION.md` and ADR-0060.

As of v0.3.38, disconnected Player Revive Configuration groups the future
cooperative-window duration with the other Revive tuning. Lifecycle keeps its
scalar pointer contract and defensive validation; no config instance or
runtime consumer is added.
