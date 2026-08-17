# ADR-0026: Orchestrate Runtime Actor Composition with Specialized Stores

- Status: Accepted
- Milestone: v0.3.3

## Context

Entity, Actor, Spatial, Dynamic Body, and Health already own distinct runtime
state and enforce their own lifetime and association rules. Creating a useful
runtime Actor composition requires those APIs to be called in dependency order
and partially completed work to be reversed on failure. Duplicating that
policy in future gameplay producers would make transactional behavior
inconsistent, but the current requirement does not justify generic runtime or
content infrastructure.

## Decision

Add a narrow internal Actor Spawn utility that receives explicit Registry and
Store pointers. A caller-owned SpawnSpec requests optional Spatial, Dynamic
Body, and Health associations; Actor is mandatory. The specification describes
initial composition only and is not retained.

Spawn prevalidates predictable input, attaches in Entity, Actor, Spatial,
Dynamic Body, Health order, and reverses successfully attached stages on late
failure. Transactionality is semantic: no newly composed live Actor remains.
A failed late attempt may consume an Entity generation or leave grown capacity
allocated, so byte-identical Registry or Store rollback is neither promised nor
required.

Despawn requires a live Actor and queries current Store state. It removes
Health, Dynamic Body, Spatial, Actor, and finally the Entity. Associations
added or removed after spawn therefore participate according to current state,
without an original SpawnSpec or persistent composition record.

Specialized Stores remain the authoritative state owners and retain their own
validation and lifetime rules. Actor remains presence-only. Actor Spawn owns no
Store, has no lifecycle or Engine state, allocates no memory directly, and has
no automatic production processing in v0.3.3.

## Rejected Alternatives

- A generic ECS, Component/System/Query model, SparseSet, Archetype, or
  component-mask abstraction would replace established specialized Stores and
  broaden the milestone far beyond composition orchestration.
- Prefab, ActorDefinition, Archetype, or Template assets would introduce a
  content model before concrete runtime producers require one.
- A SpawnManager, ActorFactory, EntityFactory, persistent spawn object, or
  deferred queue would add ownership and scheduling policy without a current
  consumer.
- Persisting the original SpawnSpec or a composition mask would duplicate and
  eventually disagree with mutable Store state.
- Level Actor declarations and automatic level population would couple runtime
  composition to content parsing before that format is designed.
- Migrating Player or adding Enemy, ActorKind, respawn, or spawn points would
  add gameplay specialization unrelated to this foundation.

## Consequences

Future gameplay code can request a small composition through one explicit
transactional boundary while existing Stores remain independently usable.
Despawn reflects current runtime composition rather than historical intent.
Callers still provide correctly paired Stores, and production has no Actor
Spawn consumer until a later milestone establishes one.
