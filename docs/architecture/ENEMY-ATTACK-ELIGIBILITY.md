# Enemy Attack Eligibility

Hertharian Engine v0.3.17 introduces an internal, stateless query that answers
whether one explicit Enemy is currently eligible to attack one explicit
Target. It is a foundation boundary only: it neither performs an attack nor
changes Decision, Pursuit, Health, DamageIntent, or any Store.

## API and Result Contract

```c
bool hth_enemy_attack_eligibility_evaluate(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    const HTHCollisionWorld *collision_world,
    HTHEntityHandle enemy,
    HTHEntityHandle target,
    float attack_range,
    bool *out_eligible);
```

The return value reports technical success. `false` means a required pointer,
range, Enemy, or Target failed structural validation. If `out_eligible` is
non-null, it is set to `false` before any validation, so every failure leaves a
canonical result. A `true` return means the query was structurally valid;
`*out_eligible` then distinguishes gameplay-ineligible from eligible.

The Enemy must be a live, generation-current Entity with Actor, Enemy, and
Spatial associations. The Target must be a live, generation-current Entity
with Spatial only. It need not be an Actor, Enemy, Player, Health owner, or
DynamicBody owner. This admits the canonical Player Target Bridge proxy
without a Player-specific path and also permits another Enemy or a plain
spatial Entity. The query observes the proxy's Spatial anchor, never Player
feet, Camera eye, PlayerBody, Input, View, or movement state.

After both sides pass complete semantic validation, self-target is a valid but
ineligible result. Invalid equal handles do not become valid through equality.
The explicit Target is independent of `HTHEnemyTargetStore`; callers may pass
the current stored Target, but the query neither requires nor changes that
relationship and performs no selection.

## Range and LOS Authorities

`attack_range` is caller-owned and must be finite and nonnegative. There is no
default, clamp, epsilon, Enemy field, definition, prefab, or Level value.
Range is delegated exclusively to `hth_enemy_perception_can_perceive()`.
Consequently it inherits full 3D Euclidean center-to-center geometry from the
two Spatial positions, double-safe intermediate arithmetic, and an inclusive
boundary. Zero range accepts distinct colocated Entities and rejects separated
ones. Attack Eligibility performs no subtraction, squaring, or distance math.

Only after range succeeds does the query call
`hth_enemy_los_has_line_of_sight()`. LOS owns all trace and occlusion semantics;
Attack Eligibility calls no Collision or trace primitive directly. Occlusion
is therefore limited to static CollisionWorld AABBs. Dynamic bodies, Actors,
Enemies, the Player body, materials, and rendered depth do not obstruct this
query. A nonzero segment that starts solid is ineligible. Colocated distinct
Entities inherit LOS's zero-length clear result.

The exact evaluation order is:

```text
canonicalize output
validate required pointers
validate attack_range
validate Enemy semantics
validate Target semantics
reject self as valid/ineligible
Enemy Perception using attack_range
Enemy LOS only when range passes
mark eligible
```

Perception and LOS expose bool-only contracts that can combine invalid and
gameplay-negative outcomes. This boundary resolves that ambiguity by
prevalidating every required semantic association. A later Perception `false`
is therefore interpreted as valid out-of-range. A later LOS `false` is
conservatively interpreted as valid ineligible; it may mean static obstruction
or an underlying Segment Trace failure, and v0.3.17 deliberately does not
distinguish those reasons.

## Purity, Safety, and Cost

Health presence and `Health.current`, DynamicBody presence or velocity, yaw,
facing, FOV, grounded state, movement results, Camera, Renderer, Input, and
Timing are irrelevant. The function retains no pointers, mutates no Registry,
Store, World, or handle, owns no state, and directly performs no heap
allocation. Equal inputs produce equal results.

Semantic validation and failed-range evaluation are O(1). When range passes,
LOS is O(N) over N static obstacles, making worst-case time O(N) and auxiliary
space O(1). Generation-safe Registry and Store access rejects stale handles.
Handles do not carry Registry identity, however: pairing a handle with its
originating Registry and associated Stores remains a caller contract, as in
the rest of the Entity foundation.

The query is headless-compatible because it depends only on gameplay and
static collision data. v0.3.17 adds no production call site. As of v0.3.18,
`hth_enemy_decision_evaluate_with_attack()` consumes this query after its outer
perception gate. As of v0.3.19, production Pursuit calls that Decision variant
once for each Enemy with a valid perceptible Target, making Eligibility
production-reachable without adding a direct Pursuit-to-Eligibility call.
Cooldown, attack type, facing, factions, dynamic occlusion, DamageIntent
creation, Health mutation, animation, and attack execution remain future
boundaries.
