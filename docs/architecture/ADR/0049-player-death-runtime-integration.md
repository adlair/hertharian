# ADR-0049: Integrate Player Death at Movement Intent Construction

- Status: Accepted
- Milestone: v0.3.25

## Context

Player Death already derives current dead state from the Health attached to the
stable Player Target Bridge identity. Production Enemy attacks can reduce that
Health to zero, but v0.3.24 does not consume the query and therefore permits
ordinary voluntary Player movement after death.

Player Movement also owns gravity, friction, collision, slide, step handling,
ground probing, momentum, and landing results. Skipping that step or directly
zeroing velocity would conflate voluntary control with physical simulation and
could freeze an airborne Player.

## Decision

DEAD PLAYER FULL MOVEMENT STEP WITH ZERO VOLUNTARY INTENT.

INPUT CONTINUES, VOLUNTARY MOVEMENT INTENT SUPPRESSED.

MOUSE LOOK REMAINS ACTIVE WHILE DEAD.

CAMERA CONTINUES CURRENT PLAYER VIEW.

TARGETING POLICY REMAINS DEFERRED; DEAD PLAYER IDENTITY PRESERVED.

NO PERSISTENT DEATH STATE; QUERY HEALTH EACH FRAME.

ENGINE ORCHESTRATION.

Engine queries `hth_player_death_is_dead()` exactly once after Input, capture,
FPS Controller, and effective-delta validation, but before movement intent and
Player Movement. It uses the Bridge-owned target handle and computes:

```text
movement_enabled = capture_active && !player_dead
```

The released intent builder therefore produces normal historical intent for an
alive captured Player and canonical zero direction, magnitude, and jump for a
dead Player. Player Movement executes exactly once in both cases. Existing
velocity evolves only through historical locomotion and collision: gravity,
friction, momentum, slide, step, grounding, and landing remain active.

Input state and capture are retained. Held movement keys may act again as soon
as healing makes Health positive. A jump edge suppressed during death is not
buffered or replayed. Controller, physical eye, View Dynamics, Camera, Bridge
sync, Enemy targeting, cadence, attack, damage, rendering, and shutdown retain
their established order and behavior.

Because Enemy Runtime follows Player Movement, lethal damage in frame N affects
voluntary movement beginning in frame N+1. Healing before the single query
restores control that frame; healing afterward is observed next frame. No latch
or explicit revive transition exists.

Technical query failure stops the frame before PlayerBody mutation rather than
assuming alive. The valid path adds one O(1), allocation-free query per frame.

## Rejected Alternatives

- Skipping Player Movement would also skip gravity and collision.
- Zeroing velocity or adding a stop primitive would introduce new physical
  policy and destroy historical momentum/friction behavior.
- A dead flag, latch, event, Player Runtime Store, or new Player runtime module
  would duplicate Health authority or add unnecessary ownership.
- Clearing Input, buffering death-time jump edges, releasing capture, freezing
  mouse-look, or adding a death Camera would broaden control/presentation scope.
- Clearing or filtering Enemy targets would decide the separately deferred
  dead-target policy.
- Reordering Enemy damage before Player Movement would alter established frame
  semantics.
- Game over, restart, respawn, revive, Downed state, Player despawn, corpse,
  animation, audio, HUD, and spectator behavior require later milestones.

## Consequences

A dead grounded Player decelerates naturally; a dead airborne Player continues
falling, colliding, and landing without voluntary air acceleration or jump.
Bridge identity and EnemyTarget relations remain stable, repeated attacks may
saturate Health at zero, and healing above zero automatically restores normal
control. Headless and graphical execution use the same policy.
