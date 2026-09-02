# ADR-0039: Enemy Pursuit Engine Integration

- Status: Accepted
- Release: Hertharian Engine v0.3.15

## Context

Enemy Target Selection, Decision, Seek, Chase, Pursuit Runtime, Runtime
Population, and the Player Target Bridge are released caller-driven
foundations. Production Engine initialization and frames did not yet compose
them, so no Runtime Enemy pursued the Player outside focused tests.

The Engine already owns all required Entity-related Stores, the Player Body,
and the static CollisionWorld simultaneously. The Player Target Bridge can
represent the non-Entity Player through one stable generation-safe Entity +
Spatial proxy without migrating Player ownership.

Player physical movement limits a frame delta to `0.1` seconds internally,
while Enemy Chase accepts any finite nonnegative delta. Supplying the raw
uncapped delta only to Enemy movement would let Player and Enemy consume
different physical intervals after a stall.

## Decision

Integrate the released pursuit stack through one private Engine-owned
`HTHBootstrapEnemyPursuit` value containing one stable Player Target Bridge and
one canonical Runtime Enemy handle.

The private bootstrap module owns only canonical state, explicit bootstrap
constants, fixed placement validation, bridge creation, Runtime Population
spawn, per-frame proxy synchronization and candidate construction, Pursuit
invocation, and reverse-order cleanup. It is not a manager, scheduler, prefab,
definition, Level gameplay system, or reusable content framework.

Initialization occurs after all required Engine dependencies and immediately
before publishing the Engine ready. A zero-length AABB trace rejects a
start-solid fixed Enemy placement. Bridge creation precedes canonical Runtime
Enemy spawn; any failure rolls back the resources already owned.

Each applicable simulation frame uses this order:

```text
Player Movement
  -> Player Target Bridge Sync
  -> Player Target Bridge GetTarget
  -> stack-local Player candidate[1]
  -> Enemy Pursuit Runtime
  -> View / Camera / Renderer
```

Engine integration derives one variable simulation delta capped at `0.1`
seconds and supplies that same effective interval to Player Movement and Enemy
Pursuit. This does not introduce a fixed timestep or a new timing system.

Shutdown despawns the Runtime Enemy before destroying the Player proxy, then
continues existing Store and World teardown. Cleanup is shared with init
rollback and remains best-effort during void Engine shutdown.

Pursuit runs headless and does not depend on Renderer or platform backend. The
Enemy remains invisible and collides only with static CollisionWorld geometry.

## Consequences

Hertharian now has one real production Enemy that can acquire the current-frame
Player proxy and execute Perception, LOS, Decision, Seek, Chase, and Dynamic
Collision throughout the Engine lifecycle. Existing target persistence and
valid no-op behavior remain authoritative.

The fixed transform, body, health, perception radius, and chase speed are
bootstrap integration data, not final balance or an Enemy archetype. A selected
Level that blocks the fixed placement causes initialization to fail rather than
triggering spawn-position solving.

The Player remains outside Entity/Actor ownership. Runtime Enemy visualization,
dynamic-vs-dynamic collision, Player contact, gravity, grounding, facing,
attack, combat, death, spawning policy, navigation, and persistence remain
separate future capabilities.

## Rejected Alternatives

- Zero production Enemies: would leave the production integration unexercised.
- Multiple bootstrap Enemies: adds placement and balance policy without proving
  another boundary.
- Scan Entity Registry for candidates: replaces explicit caller ownership with
  implicit policy.
- Cache a second Player target handle: duplicates bridge identity state.
- Add Level Enemy syntax or Enemy definitions/prefabs: expands content formats
  beyond this integration milestone.
- Add an AI manager or scheduler: one direct production orchestration call does
  not justify one.
- Sync before Player movement: exposes the previous frame's Player position.
- Run Pursuit before Sync or after Renderer: violates simulation ordering.
- Give Enemy the raw unlimited delta: diverges from Player physical time after
  stalls.
- Make Pursuit renderer-dependent or add fake static Enemy geometry: conflates
  simulation with presentation.
- Migrate Player, add dynamic collision with Player, or add combat: each is an
  independent gameplay architecture milestone.
