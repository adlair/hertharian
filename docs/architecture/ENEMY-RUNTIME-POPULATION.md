# Enemy Runtime Population Foundation

Hertharian v0.3.13 defines an internal, synchronous, caller-driven boundary
for creating and removing the canonical runtime Enemy composition:

```text
Entity + Actor + Enemy + Spatial + DynamicBody + Health
```

This composition does not change the flexible, presence-oriented Enemy
foundation. Spatial, DynamicBody, and Health are mandatory only when this
higher-level spawn operation creates a runtime Enemy. Population owns no Store,
manager, queue, scheduler, or persistent state.

## Internal Contract

The caller supplies every payload without defaults or optional flags:

```c
typedef struct {
    HTHSpatialTransform transform;
    HTHDynamicBody body;
    HTHHealth health;
} HTHEnemyRuntimeSpawnSpec;
```

```c
bool hth_enemy_runtime_spawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    const HTHEnemyRuntimeSpawnSpec *spec,
    HTHEntityHandle *out_enemy);

bool hth_enemy_runtime_despawn(
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    HTHEnemyTargetStore *targets,
    HTHEntityHandle enemy);
```

Spawn canonicalizes a writable output before validation. It translates the
three mandatory payloads into an `HTHActorSpawnSpec` with Spatial, DynamicBody,
and Health enabled, then delegates the complete base transaction to Actor
Spawn. Actor Spawn validates the payload through the existing component
contracts before creating an Entity. Population attaches Enemy only after that
transaction succeeds and publishes the handle only after the six-part
composition exists. A new runtime Enemy has no Target relation.

If Enemy attachment fails, Actor Despawn removes the composition created by
the call. Store capacities may remain grown and the Entity generation consumed
by create-and-rollback is not rewound. Neither is a semantic leak.

## Despawn Feasibility and Ordering

The v0.3.13 feasibility audit found the existing authorities sufficient.
`hth_actor_despawn()` validates all Store dependencies and a live Actor before
mutation. Its optional Health, DynamicBody, and Spatial removals, Actor removal,
and Entity destruction perform no allocation and cannot fail after their
immediately preceding current-state checks in this synchronous model. Enemy
removal and Target clear likewise become deterministic writes after ownership
validation. No frozen foundation or rollback extension is required.

Population Despawn therefore uses this safe order:

```text
validate live current Actor + Enemy and every dependency
clear the Enemy's outgoing Target relation when physically present
remove Enemy
Actor Despawn current Spatial/DynamicBody/Health, Actor, and Entity
```

An invalid, dead, stale, non-Actor, or non-Enemy handle fails before mutation.
Missing Spatial, DynamicBody, or Health is permitted because those components
may legitimately have been removed after spawn; Actor Despawn cleans whichever
optional associations remain. Health equal to zero has no special meaning for
despawn and never triggers it automatically.

Only the removed Enemy's outgoing Target entry is cleared. Population does not
scan or cascade incoming relations. If another Enemy targets the removed
Entity, generation/liveness validation makes that relation semantically absent,
and later reuse of the numeric index cannot transfer it to a new generation.

## Boundaries and Cost

Population performs no occupancy query, CollisionWorld trace, placement
adjustment, or start-solid rejection. It defines no EnemyKind, definition,
prefab, archetype, Level grammar, Resource representation, Player bridge, AI
policy, combat behavior, or automatic death handling. Its production module
does not depend on Target Selection, Perception, LOS, Decision, Seek, Chase, or
Pursuit Runtime; callers and tests may compose these released capabilities.

Enemy Runtime Population performs no direct heap allocation and uses `O(1)`
additional module-owned memory. Delegated Registry and Store operations retain
their existing allocation and capacity-growth semantics. Spawn is `O(1)`
amortized and Despawn is `O(1)` under the current direct-index Stores.

Production contains zero Enemy Runtime Spawn calls, zero Enemy Runtime Despawn
calls, and zero Population work per frame in v0.3.13. Future Player Target
Bridge and Engine integration remain separate milestones.
