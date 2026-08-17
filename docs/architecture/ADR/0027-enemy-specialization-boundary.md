# ADR-0027: Represent Enemy as a Presence-Only Actor Specialization

- Status: Accepted
- Milestone: v0.3.4

## Context

Entity already owns runtime identity and lifetime, Actor marks gameplay
participation, and specialized Stores own optional Spatial, Dynamic Body, and
Health state. The first Enemy concept needs to classify an Actor without
prematurely defining its composition, behavior, content representation, or
another identity.

## Decision

Add one dedicated internal Enemy Store. Enemy identity is exactly
`HTHEntityHandle`, and each Enemy entry contains only generation and presence.
Attach requires a live same-generation Actor. Semantic lookup and iteration
also require Actor, while removal requires only the exact live Entity so a
retained association can be cleaned after Actor removal.

The Store neither owns nor requires Spatial, Dynamic Body, or Health. It does
not create Entities or Actors, call Actor Spawn, cascade removals, or perform
per-frame work. Entity and Actor lifetime validation filters stale physical
entries. Engine owns one empty Store, but v0.3.4 creates no production Enemy.

## Rejected Alternatives

- A separate Enemy identity would duplicate Entity generation and lifetime
  authority.
- A rigid Actor+Spatial+DynamicBody+Health definition would conflate gameplay
  role with optional runtime composition.
- EnemyKind, EnemyType, species, or another taxonomy would encode categories
  before concrete gameplay requirements exist.
- EnemyDefinition, Prefab, Template, factory, or Enemy spawn helper would add
  content and creation policy; Actor Spawn remains the composition boundary.
- An Enemy subclass hierarchy would introduce object inheritance foreign to
  the current Store architecture.
- An Enemy Store that owns or mutates other Stores would create hidden
  lifecycle cascades and duplicated component authority.
- Bundling AI, targeting, movement, attacks, damage, or death processing would
  exceed a presence-only foundation.
- A generic ECS tag/component/query framework would replace a narrow concrete
  role with unrequested infrastructure.

## Consequences

Enemy can specialize any Actor regardless of optional components, and stale
handles cannot affect replacement generations. Actor removal temporarily
hides Enemy; same-generation Actor reattachment restores visibility unless
Enemy was explicitly removed. Callers must pair each Store with the correct
Registry. Iteration is deterministic but mutation during iteration is not
supported. Behavior, composition policy, content, and presentation remain
deferred.
