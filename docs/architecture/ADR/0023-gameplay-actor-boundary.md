# ADR-0023: Represent Gameplay Participation as an Entity Association

- Status: Accepted
- Milestone: v0.3.0

## Context

Entity provides runtime identity and lifetime, Spatial optionally provides
world-space state, and Dynamic Body optionally provides physical shape and
velocity. Hertharian needs to distinguish which runtime Entities participate
in gameplay without making gameplay identity own those independent concerns
or prematurely defining Player, Enemy, Pickup, Projectile, health, AI, or
behavior architecture.

## Decision

Represent gameplay participation as a presence-only, generational Actor
association keyed by `HTHEntityHandle`. Actor introduces no independent
identity or generation counter. Its private Store retains only an entry array
and capacity; each entry retains only the Entity generation and whether the
association is present.

Actor attach requires a live Entity but neither Spatial nor Dynamic Body.
Actor removal affects only the Actor Store, while Entity destruction
invalidates Actor semantically through the existing liveness/generation
predicate. Reused Entity indices cannot inherit stale Actor associations.
Iteration yields live matching Entity handles in ascending index order without
requiring any other Store.

Engine owns one empty Actor Store, but production creates no Actors and runs no
Actor update loop in this milestone. Physics remains independent of Actor and
the current Player is not migrated.

## Rejected Alternatives

- A separate Actor handle, generation, or UUID would duplicate Entity identity
  and create competing lifetime authority.
- An Actor base class or callback/vtable model would prematurely couple
  gameplay presence to behavior and object-oriented inheritance.
- Actor ownership of Transform/Spatial would collapse gameplay classification
  into world-space state.
- Actor ownership of Dynamic Body or automatic physics would make gameplay
  classification a hidden physics prerequisite.
- An ActorKind taxonomy would speculate about future Player/Enemy/Pickup/
  Projectile design without a present requirement.
- A generic ECS/component framework would replace a concrete, bounded
  association problem with infrastructure outside this milestone.

## Consequences

Entity identity, Actor participation, Spatial state, and Dynamic Body state
remain independently composable. Actors can exist without location or physics,
and physical non-Actors remain valid. Store growth and stale-handle safety use
the established generational association pattern.

Actor remains presence-only and Registry/Store pairing remains a caller
contract. Actor kinds, payload, behavior, rendering, physics policy, Level
declarations, persistence, and specialization are deliberately undecided.
