# Player Revive Window Foundation

Hertharian v0.3.28 introduces a private caller-owned timer for a bounded future
Revive opportunity. This milestone implements only the timer primitive. It
does not store Downed, identify a Player or reviver, restore Health, or mark
Player Defeat.

## State and Invariants

The state has exactly two semantic fields:

```c
typedef struct HTHPlayerReviveWindow {
    double remaining_seconds;
    bool active;
} HTHPlayerReviveWindow;
```

A zero-initialized state is canonically inactive with zero remaining time and
no pending expiry. In valid states, inactive is equivalent to zero remaining
time, while active requires a finite positive remainder. Query, begin, and
advance reject malformed local state without mutation. Reset accepts null as a
no-op and canonicalizes any non-null state, including malformed state, to the
inactive representation. Reset therefore also serves as cancellation.

The trivial value has no pointer ownership or hidden shared state. Copying it
copies only the current timer state, and multiple instances are independent.
A future owner must reset an instance before associating it with a different
logical Player.

## Begin, Advance, and Expiry

Begin succeeds only for a valid inactive state and a finite duration strictly
greater than zero. Zero, negative, NaN, and infinite durations fail without
mutation. Beginning an active window also fails without restarting, extending,
shortening, or replacing it.

Advance receives simulation delta from its caller. A valid delta is finite and
non-negative. Advancing an inactive window or advancing an active window by
zero succeeds as a no-op. A smaller positive delta is subtracted exactly. A
delta equal to or greater than the remainder atomically saturates the timer to
zero, makes it inactive, and reports expiry through `out_expired=true`.

Expiry is a one-shot result of the advance that performs the active-to-inactive
transition. It is not persistent state. Subsequent valid advances report false
until a new positive-duration episode begins. No epsilon, tolerance, catch-up
loop, or negative intermediate remainder is used. A positive floating-point
residue therefore keeps the window active until a later delta reaches it.

Advance canonicalizes a supplied expiry output to false before validation.
Query similarly canonicalizes supplied active and remaining outputs to false
and zero. Query is pure, deterministic, and never consumes an expiry.

## Architectural Boundary

The foundation has no Store, allocation, static mutable state, Player or Entity
handle, generation mapping, wall clock, or dependency on Entity, Actor,
Spatial, Input, Camera, Enemy, Collision, Health, Player Death, or Player
Defeat. Caller-provided simulation delta does not create a Timing dependency.
Every operation is O(1) and allocation-free.

v0.3.28 is disconnected: production owns no window and performs no window work
per frame. A future lifecycle adapter may begin/cancel the timer and consume an
expiry to mark Player Defeat, but the timer never performs that transition.

The approved future ordering is Player Revive Window, Player Role/Roster,
Revive Eligibility, Revive Execution, Lifecycle/Defeat Runtime Integration,
and Session Outcome. Downed remains derived outside this primitive. Reviver
identity, range, LOS, interaction, revive Health, self-revive capabilities,
items, inventory, charges, and progression remain future architecture.

Base lifecycle policy is also not integrated here: solo will eventually mark
permanent Defeat on its first authoritative dead snapshot, while cooperative
play will begin a window and derive Downed while dead, not defeated, and the
window is active.

As of v0.3.29, Player Roster provides the future membership and slot domain for
Revive Eligibility. The roster does not own, reset, begin, or advance this
caller-owned window.

As of v0.3.30, Player Revive Eligibility queries the target window's
authoritative active state after complete dependency validation. It never
begins, advances, resets, expires, or otherwise consumes the window.

As of v0.3.31, successful Player Revive Execution cancels the target Window
with `reset()` only after Health healing succeeds. Failure or valid
ineligibility leaves the Window unchanged.
