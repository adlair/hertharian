# Enemy Seek / Steering Foundation

Hertharian v0.3.10 introduces an internal geometric query that converts an
explicit Enemy and explicit Target into a desired world-space direction. The
module boundary ends at direction: Seek does not move either Entity, set
velocity or speed, rotate yaw, perform collision, or execute behavior.

The internal API is `hth_enemy_seek_compute()`. Its Enemy must be a live Entity
of the current generation with Actor, Enemy, and Spatial associations. Its
Target needs only to be a live Entity of the current generation with Spatial;
it need not be an Actor, Enemy, Player, Health owner, or Dynamic Body. Seek
receives the Target explicitly and depends on neither `HTHEnemyTargetStore` nor
`HTHEnemyIntent`. The caller owns Decision and Target orchestration.

## Geometric Contract

For Enemy position `E` and Target position `T`, Seek computes `delta = T - E`.
Exact zero displacement produces the canonical zero vector. Otherwise it
returns `delta / length(delta)`, a unit direction whose distance magnitude has
been discarded. Self-targeting and distinct colocated Entities are valid zero
displacements; there is no stopping epsilon or arrival policy.

Subtraction, squared magnitude, and square-root normalization use `double`
intermediates. Each finite float component is promoted before subtraction, so
large opposite-sign coordinates cannot overflow in float arithmetic. The
normalized components are then safely narrowed to `HTHVec3` floats.

When writable, the output is canonicalized to `{0, 0, 0}` before structural
validation. `false` means structural failure and retains that zero output;
`true` means the query was structurally valid, whether its result is zero or a
unit direction. The boolean does not indicate that movement exists.

## Ownership and Deferred Execution

Seek is stateless, deterministic, observationally pure, and performs no direct
allocation. It mutates no Registry or Store and retains no pointer or previous
direction. Each query takes `O(1)` time and `O(1)` additional memory, with no
Entity, candidate, obstacle, or path scan.

Seek calls no Target Selection, Decision, Perception, LOS, Collision, World,
navigation, or pathfinding capability. It applies no speed, velocity,
acceleration, turn rate, smoothing, arrival, prediction, or obstacle avoidance.
A direct direction through a wall is therefore valid output at this boundary.

Decision may produce `PURSUE(target)`; an external future orchestrator can then
pass that explicit Target to Seek. Future Chase owns conversion of direction
into movement policy, while later runtime work owns scheduling and execution.
Production contains zero Seek calls and performs zero Seek work per frame in
v0.3.10.

As of v0.3.11, Enemy Chase can apply this direction through the generic
DynamicBody and Dynamic Collision path. Seek remains pure and never invokes
Chase itself.
