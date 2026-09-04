# Player Death Runtime Integration

Hertharian v0.3.25 consumes the released Player Death query once per simulation
frame in Engine orchestration. Health remains the sole life authority; no dead
flag, latch, event, timestamp, Store, or Player runtime module is introduced.

## Frame Boundary

The production order is:

```text
Platform/Input processing
→ capture reconciliation
→ FPS Controller and mouse-look
→ effective simulation delta validation
→ Player Death query
→ movement_enabled = capture_active && !player_dead
→ Player Movement Intent
→ Player Movement always
→ Player eye and Player Target Bridge sync
→ Enemy Runtime and Damage
→ View Dynamics, Camera, and Renderer
```

The query uses the stable `HTHPlayerTargetBridge.target_entity` identity and
runs before intent construction and PlayerBody mutation. A stale identity,
missing Actor, missing Health, invalid Health, or other technical query failure
stops the current Engine frame under the existing failure convention; it is
never interpreted as alive.

Enemy damage remains later in the frame. If an Enemy deals lethal damage in
frame N, the Player has already completed that frame's alive movement. The one
query in frame N+1 observes Health zero and suppresses voluntary movement from
that frame onward. There is no second post-damage query and no rollback or
reordering of established simulation stages.

## Dead Movement Policy

A dead Player receives the existing canonical disabled intent:

```text
direction = (0, 0, 0)
magnitude = 0
jump_pressed = false
```

`hth_player_movement_step_with_result()` still runs exactly once. Only
voluntary acceleration and jump initiation are suppressed. Existing velocity
is not zeroed: airborne momentum and gravity continue, grounded momentum decays
through normal friction, and collision, slide, step handling, ground probing,
fall-speed limiting, landing, and `HTHPlayerMovementResult` remain historical.
A Player killed while airborne therefore falls and lands rather than freezing.

Input continues recording held and transient state. W/A/S/D held through death
cannot produce intent while Health is zero, but can contribute immediately when
Health becomes positive. A Space pressed edge during a dead frame is consumed
normally and is not replayed after healing; a later jump requires the ordinary
release and new valid press transition.

## Preserved Observation and Identity

FPS Controller and mouse capture are not gated by death, so yaw and pitch remain
active. Camera continues following the post-movement physical eye and current
orientation. View Dynamics continues receiving real velocity, grounded, and
landing observations and settles through its existing rules. There is no death
camera or presentation state.

Player Target Bridge sync remains after Player Movement and preserves the same
Entity + Actor + Spatial + Health identity and generation. Targeting policy is
deferred: EnemyTarget relations are not cleared, Enemy AI is unchanged, and
Enemies may continue cadence-controlled attacks against zero Health. Damage
saturates at zero without despawning the Player.

Healing current Health above zero before a frame's query immediately restores
normal intent construction in that frame. Healing after the query takes effect
on the next frame. This reversible derived behavior is not a revive system and
requires no reset.

## Scope and Cost

The policy is identical in headless and graphical execution and has no Renderer
dependency. It adds one deterministic O(1), allocation-free Player Death query
per simulation frame and does not change overall frame complexity.

Game over, restart, respawn, revive, Downed state, Player despawn, corpse
physics, animation, audio, HUD, target filtering, spectator cameras,
persistence, and multiplayer death remain outside v0.3.25.

As of v0.3.26, Engine reuses this exact frame snapshot for dead-Player
targetability; no second death query is added. See
`DEAD-PLAYER-TARGETING-POLICY.md` and ADR-0050.

As of v0.3.27, movement suppression still consumes only Player Death. The
separate persistent Player Defeat primitive has no production integration or
effect on this frame policy.
