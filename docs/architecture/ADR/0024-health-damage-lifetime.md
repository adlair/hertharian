# ADR-0024: Keep Health as Actor-Dependent Generational State

- Status: Accepted
- Milestone: v0.3.1

## Context

Entity already owns runtime identity and lifetime, while Actor identifies
gameplay participation. Hertharian needs bounded mutable Health arithmetic
without turning Actor into a state container or prematurely defining combat,
death, or an ECS.

## Decision

Represent Health in a separate private sparse Store keyed by
`HTHEntityHandle`. An attached value contains only finite `current` and
`maximum` floats satisfying `maximum > 0` and
`0 <= current <= maximum`. Health requires both a live Entity and a matching
Actor association. Entity Registry remains lifetime authority and Actor Store
remains gameplay-membership authority.

Damage and healing are explicit Store operations with saturating arithmetic
and copy-out results. Zero Health remains valid state; only a positive-to-zero
damage transition reports `became_zero`. Maximum is immutable after attach.

No lifecycle cascade is introduced. Entity death or Actor removal makes Health
semantically unavailable through authority validation. Same-generation Actor
reattach reveals retained Health, while Entity index reuse does not inherit it
because generations differ. Store destruction owns only its backing memory.

Engine creates one empty Health Store after Actor and destroys it before
Actor. Production performs no Health iteration or mutation in this milestone.

## Rejected Alternatives

- Embedding Health in Actor would make presence-only gameplay participation a
  payload container and couple every Actor to Health.
- Embedding Health in Entity, Spatial, or Dynamic Body would collapse lifetime,
  location, or physics into gameplay state.
- A separate Health identity would duplicate Entity's generational authority.
- Automatic removal callbacks from Actor or Entity would introduce lifecycle
  coupling and cascade infrastructure outside this milestone.
- A dead flag would duplicate the canonical `current == 0` state.
- Automatically destroying an Entity when Health reaches zero would collapse
  depleted gameplay state into runtime lifetime; Entity Registry remains the
  sole authority for Entity destruction.
- Treating negative damage as healing would overload signed meaning. Damage
  and healing remain separate explicit operations.
- Damage sources, instigators, weapons, and damage types or categories would
  prematurely define future combat specialization outside Health Foundation.
- A generic component/ECS framework would broaden a concrete association into
  speculative infrastructure.

## Consequences

Health is optional, independently removable gameplay state whose semantic
validity composes with Entity and Actor. Sparse generational storage provides
constant-time ordinary access, deterministic growth, stale-handle safety, and
ascending iteration. Callers must pair a Health Store with the Registry and
Actor Store that populated it. Combat policy, death behavior, and mutation
during iteration remain deliberately undefined.
