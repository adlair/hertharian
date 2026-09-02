# ADR-0037: Enemy Runtime Population Boundary

- Status: Accepted
- Milestone: v0.3.13

## Context

Released pursuit capabilities require Enemies with a coherent runtime
composition, but the flexible Enemy association intentionally does not require
Spatial, DynamicBody, or Health. A canonical creation/removal boundary is
needed without introducing automatic population or duplicating Actor Spawn.

The mandatory despawn feasibility audit confirmed that existing APIs are
sufficient. After validating a live current Actor and Enemy, Target clear and
Enemy removal are deterministic non-allocating writes. Actor Despawn removes
whichever optional Health, DynamicBody, and Spatial associations remain, then
Actor and Entity; after its prevalidation these operations cannot fail late in
the current synchronous model. Actor Despawn is therefore also a safe rollback
after successful Actor Spawn followed by failed Enemy attachment.

## Decision

Define the canonical runtime Enemy as Entity + Actor + Enemy + Spatial +
DynamicBody + Health. Layer Enemy attachment over the existing Actor Spawn
transaction, requiring explicit Transform, Body, and Health payloads. Provide
a symmetric generation-safe Despawn that validates Actor+Enemy ownership,
clears only the outgoing EnemyTarget relation, removes Enemy, and delegates the
remaining composition and Entity removal to Actor Despawn.

Both operations are internal, synchronous, stateless, and caller-driven. They
perform no direct allocation, while delegated Stores preserve their growth
semantics. They do not validate world occupancy or execute AI.

## Rejected Alternatives

- Duplicating Entity creation or Actor Spawn would create competing
  transactions and rollback rules.
- An Enemy-specific allocator would duplicate Registry ownership.
- Exposing arbitrary `HTHActorSpawnSpec` would weaken the canonical mandatory
  composition.
- Optional Spatial, Body, or Health flags would make runtime readiness
  ambiguous.
- EnemyPrefab, EnemyDefinition, EnemyKind, or a generic ECS archetype would add
  type/data systems outside this foundation.
- EnemySpawnManager, deferred queues, scheduling, or automatic startup
  population would add ownership and production execution prematurely.
- Level-driven spawning or Resource-driven Enemy definitions would change
  frozen formats and loading boundaries.
- Scanning and clearing incoming Target relations would add an unnecessary
  global cascade; generational semantics already invalidate them.
- Population-owned AI state would invert the dependency on pursuit modules.
- Automatically despawning at zero Health would introduce death policy.

## Consequences

Callers gain one canonical construction path usable by Target Selection and
Pursuit Runtime without production integration. Spawn is `O(1)` amortized,
Despawn is `O(1)`, and module-owned auxiliary memory is `O(1)`. Capacity growth
and consumed Entity generations are not rolled back. Player bridging, Level
population, and Engine scheduling remain deferred.
