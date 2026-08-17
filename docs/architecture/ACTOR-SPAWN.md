# Actor Spawn and Runtime Composition

Hertharian v0.3.3 provides an internal orchestration utility for creating and
tearing down runtime Actor compositions through the existing specialized
Stores. It centralizes ordering and rollback policy without owning Store state
or changing the meaning of Actor.

```text
Entity + Actor + optional Spatial + optional DynamicBody + optional Health
```

Production has no Actor Spawn consumer in this milestone. The utility has no
Engine-owned state, initialization, shutdown, queue, or per-frame work.

## Spawn Specification

`HTHActorSpawnSpec` is a caller-owned value containing three optional flags and
their payloads: Spatial transform, Dynamic Body, and Health. It is only the
initial composition request. The utility neither retains its pointer nor
persists a copy, component mask, or original-composition record.

A completely zero-initialized specification is valid and requests exactly an
Entity with Actor. When an optional flag is false, its payload is ignored and
need not contain valid values. When a flag is true, the caller supplies a value
valid under that subsystem's existing contract; Actor Spawn invents no default
transform, extents, velocity, or Health.

The valid composition matrix is:

```text
Actor
Actor + Spatial
Actor + Health
Actor + Spatial + Health
Actor + Spatial + DynamicBody
Actor + Spatial + DynamicBody + Health
```

Dynamic Body requires Spatial, so Body without Spatial is rejected before
Entity creation, with or without Health. Health requires Actor, which every
successful Actor Spawn guarantees; Health does not require Spatial or Body.
Spatial does not imply Body, and Body does not imply Health.

## Validation and Spawn Transaction

The Registry, Actor Store, Spatial Store, Dynamic Body Store, Health Store,
specification, and output handle are all required. The output is set to the
canonical invalid Entity handle before validation or mutation and remains
invalid on every failure.

Predictable invalid input is rejected before Entity creation. Validation
enforces the Body-to-Spatial relationship and checks only enabled payloads.
Health uses `hth_health_is_valid`. Spatial and Dynamic Body currently expose
their value validators only inside their implementations, so Actor Spawn
mirrors their exact finite-value and positive-half-extent contracts without
changing those subsystems.

Successful attachment order follows dependencies:

```text
Entity → Actor → Spatial → DynamicBody → Health
```

If an operation after Entity creation fails, locally tracked successful stages
are rolled back in reverse order:

```text
Health → DynamicBody → Spatial → Actor → Entity destruction
```

This transaction is semantic rather than byte-identical:

```text
SpawnFailure → no newly composed live Actor remains observable
```

A late failure may create and then destroy an Entity, consuming a generation.
Store or Registry capacity grown before failure may remain allocated. Neither
generation nor capacity is rolled back, and Stores are not shrunk. The current
Store APIs offer no legitimate deterministic post-creation failure for a valid
fresh composition other than internal allocation failure, so v0.3.3 adds no
fault-injection hooks; every such failure uses the same structural rollback
path.

## Despawn

Actor despawn requires every Registry/Store pointer, a live exact-generation
Entity handle, and Actor presence. It is not arbitrary Entity cleanup: a live
non-Actor, a stale handle, a second despawn, or an Entity whose Actor was
already removed returns false without affecting a replacement generation.

Despawn queries the current Stores and removes associations in dependency
order:

```text
Health → DynamicBody → Spatial → Actor → Entity destruction
```

Missing optional associations are normal. A composition may change after
spawn through existing Store APIs. For example, Health attached after an
Actor+Spatial spawn is still removed by despawn, while an optional association
removed before despawn does not make despawn fail. No SpawnSpec is required or
consulted. The specialized Stores are the authoritative current runtime
composition.

All Store queries and removals remain generation-safe. A stale handle with the
same numeric index cannot remove associations from its replacement Entity.
Despawn performs no capacity shrinking and returns only a boolean result.

## Ownership, Complexity, and Boundaries

Actor Spawn receives explicit Store pointers and retains none. Correct
Registry/Store pairing remains the caller's responsibility. The module has no
mutable global state and directly allocates no memory; underlying Store growth
may allocate. Spawn is O(1) amortized, and despawn is O(1), with a fixed number
of Store operations and no iteration.

Actor continues to mean only that an Entity participates in gameplay as an
Actor. Actor owns none of Spatial, Dynamic Body, or Health, and runtime
composition is not immutable. Actor Spawn performs no physics, collision,
rendering, filesystem, resource, platform, or graphics work.

Deferred scope includes ActorKind, Enemy, Player migration, Prefab,
ActorDefinition, Archetype, Template, ECS or generic component infrastructure,
component masks, runtime context objects, SpawnManager, factories, queues,
Level Actor declarations or automatic population, respawn, spawn points,
rendering integration, automatic physics, persistence, networking, scripting,
names, teams, hierarchy, and spawn provenance.
