# Enemy Pursuit Engine Integration

Hertharian v0.3.15 connects the released Enemy pursuit foundations to the
production Engine lifecycle. The Engine owns one private bootstrap integration
value containing one stable Player Target Bridge and one canonical Runtime
Enemy:

```text
HTHBootstrapEnemyPursuit
  Player Target Bridge -> Entity + Actor + Spatial + Health Player target
  Enemy handle         -> Entity + Actor + Enemy + Spatial + DynamicBody + Health
```

This is bootstrap integration/demo composition, not an Enemy manager,
scheduler, prefab, definition, archetype, Level gameplay schema, or final
balance system. Both handles use the canonical invalid Entity handle as their
inactive state; no separate lifecycle booleans or heap-owned manager state are
required.

## Bootstrap Composition

Exactly one Enemy is spawned through `hth_enemy_runtime_spawn()` with:

```text
position       = (3.0, 0.95, 3.0)
yaw            = 0.0
half extents   = (0.30, 0.90, 0.30)
velocity       = (0.0, 0.0, 0.0)
health         = 100.0 / 100.0
perception     = 8.0
attack range   = 1.25
chase speed    = 2.0
```

These constants are private bootstrap pursuit integration data. They are not
final Enemy statistics or reusable content defaults. Before bridge creation or
Enemy spawn, a zero-length AABB trace verifies that the fixed body placement is
not start-solid in the selected Level's CollisionWorld. The integration fails
cleanly if Level variation makes this position invalid; it never searches for,
slides to, or synthesizes a replacement spawn.

The Player Target Bridge creates one stable Entity + Actor + Spatial + Health
target at the Player body center. Each frame obtains that handle through
`GetTarget` and places it in a stack-local one-element candidate array. Pursuit
retains neither the array nor its pointer. There is no Entity scan, candidate
Store, cached duplicate Player handle, or Player-specific branch in Enemy AI.

## Initialization Transaction

Integration happens after World, all Entity-related Stores, Player Body,
CollisionWorld, Platform, optional Renderer, Input, and Timing are valid, and
immediately before the Engine publishes `initialized` and `running`:

```text
canonical integration state
  -> fixed placement validation
  -> Player Target Bridge Create
  -> Runtime Enemy Spawn
  -> Engine ready
```

Placement or bridge failure creates no Enemy. Runtime Population owns its
internal spawn rollback. If Enemy spawn fails after bridge creation, the bridge
is destroyed before existing Engine rollback continues. The same private
cleanup path supports init rollback and normal shutdown.

## Simulation Order and Delta

The authoritative production order is:

```text
Events / Input
  -> FPS orientation / Player intent
  -> Player Movement / static Collision
  -> Player Target Bridge Sync
  -> Player proxy GetTarget
  -> candidates[1]
  -> Enemy Pursuit Runtime
  -> View Dynamics
  -> Camera
  -> Renderer
  -> pacing
```

Player physical movement therefore finishes before the proxy is synchronized,
and Pursuit observes the current frame's Player body-center position rather
than the previous frame's position. Sync, GetTarget, and Pursuit each execute
once for every simulation frame that successfully completes Player movement,
including headless frames.

Timing still uses the variable frame delta. Engine integration caps one shared
effective physical delta at `0.1` seconds before passing it to both Player
Movement and Enemy Pursuit. This preserves the existing Player policy and
prevents Enemy movement from consuming a larger interval after a stall. It is
not a fixed timestep, accumulator, substep system, or new timing foundation.

Sync, GetTarget, or Pursuit returning false is a technical Engine failure: the
Engine reports the failed boundary, stops the run loop, and leaves cleanup to
normal ownership. Outside perception, blocked LOS, Decision IDLE, zero Seek,
zero displacement, and static collision are valid successful no-ops.

An acquired target persists while perception or LOS temporarily prevents
pursuit; Decision becomes IDLE and resumes against the same generation when
conditions recover. Inside the inclusive `1.25` attack range with clear LOS,
Decision becomes ATTACK and Pursuit suppresses movement for that frame. Moving
the Player proxy outside attack range resumes pursuit without reselection. No
continuous retargeting policy is added.

## Cleanup

Shutdown stops simulation and then performs:

```text
Runtime Enemy Despawn
  -> Player Target Bridge Destroy
  -> EnemyTarget / Health / Enemy / Actor / DynamicBody / Spatial Stores
  -> Entity Registry
  -> World
```

Enemy-first cleanup allows Runtime Population to clear its outgoing target
while the Player proxy is still alive. Cleanup tolerates neither, only bridge,
or both resources being active. Shutdown remains best-effort: a despawn or
bridge-destroy failure is reported, then Store teardown continues because the
whole owner is being discarded.

All state is per Engine instance. Repeated init/shutdown creates new Entity
generations; stale handles from an earlier instance never regain validity.

## Current Boundaries

Pursuit is simulation and runs identically with or without Renderer. As of
v0.3.16, graphical frames observe the same authoritative Enemy handle after
Pursuit and extract one transient BOX draw; headless frames skip that pure
presentation boundary. No fake World object or gameplay rendering state is
introduced; see `RUNTIME-BODY-VISUALIZATION.md`.

Enemy Dynamic Collision resolves only against static CollisionWorld geometry.
The Enemy can overlap the Player/proxy and has no gravity, grounding, facing,
attack execution, damage, death, respawn, navigation, or pathfinding. ATTACK
suppresses Pursuit Runtime movement for the frame: it calls no Seek, Chase, or
Dynamic Collision and writes neither Spatial nor velocity. Stored
`DynamicBody.velocity` is retained and is not guaranteed to be zero. These
limitations define future milestones; they do not weaken the production
pursuit integration.

## Production Call Structure

For each successful Engine lifecycle:

- Bridge Create: once during init;
- Runtime Enemy Spawn: once during init;
- Bridge Sync, GetTarget, and Pursuit Runtime: once per applicable simulation
  frame;
- Runtime Enemy Despawn: once during shutdown;
- Bridge Destroy: once during shutdown.

No public API, Level or Material format revision, Player migration, AI manager,
Renderer expansion, or SDL/Platform/Input change is part of this milestone.

As of v0.3.19, Pursuit passes the private bootstrap attack range to the
attack-capable Decision API. Eligibility is reached only through Decision;
there is no direct Eligibility call, separate attack phase, DamageIntent,
Health mutation, cooldown, or execution work in the Engine loop.

As of v0.3.20, the Player target's Actor and Health associations make that
same handle structurally compatible with future DamageIntent resolution. Its
initial `100/100` Health is private temporary bootstrap tuning. ATTACK still
has no execution, production DamageIntent, damage application, or cadence.
