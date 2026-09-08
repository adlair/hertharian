# Player Revive Interaction Foundation

Hertharian v0.3.33 adds a private, disconnected hold-to-revive interaction
foundation. It composes released Player lifecycle, Eligibility, Execution, and
Spatial authorities without adding an Engine caller. Production still has one
Player, so a legitimate reviver/target pair does not yet exist; tests compose
multiple roster Players locally without creating fake production multiplayer.

## State, Identity, and Target Boundary

One caller-owned `HTHPlayerReviveInteraction` represents one reviver's current
attempt:

```c
typedef struct HTHPlayerReviveInteraction {
    HTHPlayerSlot target_slot;
    double elapsed_seconds;
    double required_seconds;
    bool active;
} HTHPlayerReviveInteraction;
```

The identity of an active attempt is reviver PlayerSlot plus target PlayerSlot.
The reviver remains an explicit step input; the target is stored only while the
attempt is active. Entity handles and positions are never cached. The caller
supplies the candidate target slot, and Interaction performs no scan, nearest
query, ranking, camera ray, or automatic target switching.

Zero initialization is canonical inactive state. Stored `target_slot == 0` has
no meaning while inactive. Reset is null-safe and zeroes the value. Query
canonicalizes every output before validation and reports inactive target as
`HTH_PLAYER_SLOT_INVALID`, never as semantic slot zero. An active state requires
a valid target slot, finite positive required duration, and finite elapsed time
in `[0, required)`. Completion is never persisted as `elapsed == required`.
The value has no pointer ownership and copies independently.

## Input and Hold Progress

The step accepts a semantic `interaction_held` boolean and has no `HTHInput`
dependency or physical key binding. Inactive plus not held is a valid idle
step; release resets an active attempt without consulting Spatial or
Eligibility. A held, eligible, in-range attempt consumes caller-supplied finite
nonnegative simulation delta. Delta is not wall-clock time.

Hold duration is a finite positive caller/config value captured when a new
attempt starts. A later valid duration argument does not rewrite the active
attempt. Target change discards old progress, captures the current duration,
and applies only the current step's delta to the new target. An inactive or
retargeted attempt may complete immediately when delta covers its complete
duration. Exact completion and overshoot use the same inclusive comparison;
there is no epsilon, arbitrary minimum, pause, decay, or catch-up loop.

## Spatial Range

Interaction resolves both current Player Entities from Lifecycle Runtime's
roster and fetches their current Spatial transforms every held step. Missing or
stale Player/Spatial state is a technical failure. Spatial positions are
finite by Store contract; distance arithmetic nevertheless rejects a
non-finite squared result.

Range is the full three-dimensional Euclidean distance between the two
world-space Spatial position anchors:

```text
dx*dx + dy*dy + dz*dz <= revive_range*revive_range
```

Coordinates are promoted to `double` before subtraction and squaring. Y
participates, the exact boundary is included, and no epsilon is used. Range is
a current finite positive caller/config value rechecked every held step; it is
not captured. Out of range is a valid cancellation, while missing Spatial is a
technical failure. Interaction has no PlayerBody, Camera, facing, LOS,
Collision, or trace dependency.

## Eligibility and Execution Authority Chain

Lifecycle Runtime supplies two controlled private adapters. `can_revive`
validates slot bounds, binds runtime-owned Roster/Defeat/Window state, and
delegates exactly once to released Player Revive Eligibility. `execute_revive`
does the same binding and delegates exactly once to released Player Revive
Execution. The adapters duplicate no policy and expose no mutable Window.

Every held step validates both required Spatial dependencies before combining
policy, then evaluates Eligibility exactly once. Valid ineligibility cancels
the attempt. On provisional completion, Interaction calls the execution
adapter once; Execution performs its own final Eligibility revalidation before
healing through Health authority and resetting the target Window. Interaction
never directly reads or mutates Health, Defeat, or ReviveWindow.

`revive_health` is a current finite positive caller/config value. It is
validated on held steps, is not stored, and its completion-frame value is
passed to Execution. No default range, duration, or revive Health is defined.

## Cancellation and Transaction Contract

Input release, valid Eligibility loss, range loss, or a valid unusable target
change resets progress and returns technical success with `revived=false`.
Movement within range does not cancel. Non-lethal damage has no special
interrupt; lethal damage makes the reviver ineligible through existing Death
authority. LOS and facing do not participate.

Every technical failure reports false, canonicalizes `revived=false`, and
preserves all pre-step interaction fields. Progress and completion are first
computed in local temporaries. If Execution fails technically, provisional
completion is neither committed nor reset, allowing deterministic retry. A
valid Execution no-op means final revalidation rejected the pair; it resets
the completed attempt and reports technical success without a revive.
Successful Execution likewise resets the attempt and reports `revived=true`.
Query exposes active target, elapsed, and captured required seconds for a
future presentation layer without introducing events or presentation now.

## Multiple Revivers and Deferred Integration

Each reviver owns an independent interaction value. Multiple revivers may
progress against one target without target lock or shared progress. The first
successful Execution restores Health and cancels Window; later attempts become
ineligible and reset, preventing a second heal through final authority.

The module performs deterministic `O(1)` work, uses `O(1)` auxiliary memory,
allocates nothing, and owns no global state. Engine and Bootstrap own no
interaction value and make no interaction or revive-execution call in v0.3.33.
There is no per-frame production work, input mapping, second Player, generic
Interaction system, HUD, audio, animation, networking, self-revive, inventory,
progression, Session Outcome, Game Over, or respawn.

Future Player Coop Runtime / Revive Interaction integration must acquire a
candidate and map a semantic action, then run Interaction/Execution after Input
and before the existing authoritative Death snapshot. Lifecycle, movement,
dead-target policy, and Enemy runtime follow that one snapshot, so a same-frame
successful revive wins before Window expiry without adding another Death
query.
