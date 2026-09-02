# Enemy Chase Foundation

Hertharian v0.3.11 introduces the first Enemy capability that applies a
controlled movement mutation. The completed feasibility audit established
that Player Locomotion is Player-specific while the reusable generic path is
Entity + Spatial + DynamicBody through `hth_dynamic_collision_move()`. Enemy
Chase is therefore a thin semantic adapter over that existing path, not a new
physics or locomotion system.

The internal `hth_enemy_chase_apply()` API receives the Entity, Actor, Enemy,
Spatial and DynamicBody authorities, a CollisionWorld, an explicit Enemy,
desired world-space direction, caller-owned speed and delta time, and an
`HTHDynamicCollisionResult` output. The Enemy must be a live current-generation
Entity with Actor, Enemy, Spatial, and DynamicBody associations. Health,
Target, Intent, Player, Camera, and Input are irrelevant.

## Application Contract

The caller supplies a finite direction compatible with Enemy Seek: exactly
zero or a unit world-space vector. Chase validates finiteness but deliberately
does not renormalize, clamp, project, or reinterpret it. It computes:

```text
desired_velocity = desired_direction * chase_speed
```

Speed and delta time must be finite and nonnegative. Chase verifies that each
float desired-velocity component remains finite, writes it through
`hth_dynamic_body_set_velocity()`, and delegates time integration and physical
resolution to `hth_dynamic_collision_move()`. It never writes Spatial position
directly or performs collision traces itself.

Zero direction or zero speed explicitly requests zero velocity. Zero delta
time is also valid: position remains unchanged while the requested velocity is
installed according to the Dynamic Collision contract. All three axes pass
through literally, including vertical movement. Chase applies no gravity,
grounding, floor snap, step, friction, acceleration, smoothing, arrival,
navigation, pathfinding, yaw, or facing policy.

Collision, sliding, tunneling prevention, start-solid behavior, resolved
position, and resolved velocity retain Dynamic Collision semantics. A physical
collision is a valid result rather than an API failure, and Chase returns the
existing `HTHDynamicCollisionResult` unchanged.

## Failure and Ownership

When writable, the result is canonicalized before validation. `false` denotes
an argument, structural, or technical mutation failure; `true` denotes a
structurally valid request evaluated by Dynamic Collision, including no-op,
collision, blocking, and start-solid cases.

Chase validates everything possible before mutation and captures the previous
Body velocity. If Dynamic Collision fails after desired velocity is written,
Chase restores that previous velocity and returns a canonical result. Dynamic
Collision owns the resolved velocity after success; Chase never overwrites it.

Chase is stateless and deterministic and performs no direct per-call heap
allocation. Its adapter work and additional memory are `O(1)`. Delegated
movement uses at most four bumps and traces the static CollisionWorld, making
overall cost `O(N)` in its `N` static obstacles under the current collision
implementation.

Target Selection, Decision, and Seek remain caller-owned upstream stages.
Chase receives no Target or Intent and calls none of them. Production contains
zero Chase calls and performs zero Chase work per frame in v0.3.11. The future
v0.3.12 Pursuit Runtime Loop owns scheduling and composition.
