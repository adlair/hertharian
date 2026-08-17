# ADR-0025: Separate Damage Intent from Health Mutation

- Status: Accepted
- Milestone: v0.3.2

## Context

Health owns bounded damage arithmetic but deliberately knows nothing about who
requested damage. Hertharian needs a minimal boundary value that can preserve
source and target Actor identity while current runtime state determines whether
the request can reach target Health. This requirement does not yet justify a
combat system or generic action architecture.

## Decision

Represent Damage Intent as a caller-owned value containing source Entity
handle, target Entity handle, and finite non-negative amount. Intent validity
requires both handles to identify live Actors, but does not require Health on
either source or target. Target Health is a resolution concern.

Provide pure validation and explicit resolution. A valid Actor target without
Health is processed successfully with `applied=false`. When target Health is
present, resolution delegates exactly once to the existing Health damage API
and reports `applied=true` only after that delegation succeeds. Health remains
provenance-agnostic and authoritative for all arithmetic.

Damage Intent has no Store, lifecycle, queue, consumption state, or Engine
ownership. The same value may be explicitly resolved repeatedly, with every
call evaluating current runtime state. Production creates no Intents and runs
no automatic resolver in v0.3.2.

## Rejected Alternatives

- Adding source or attacker provenance to Health state or its core mutation
  model would make Health own combat context instead of Health arithmetic.
- A generic `HTHGameplayAction` hierarchy, kind enum, or union is premature
  while only one concrete action semantic exists. Shared structure should be
  extracted only after multiple real cases require it.
- A Damage Intent or gameplay action queue has no scheduling requirement in
  this milestone and would introduce persistence and processing policy.
- EventBus, observer, signal, or callback infrastructure is unnecessary when
  explicit return values fully describe resolution.
- Combat, Attack, or Weapon systems would broaden a boundary value and resolver
  into gameplay specialization without a production consumer.
- Range, line-of-sight, collision, and hit detection belong to upstream logic
  deciding whether an Intent should be constructed, not to Intent validity.
- Damage types, weapons, projectiles, and extra instigator metadata are future
  combat concerns and are not part of Health or Damage Intent Foundation.

## Consequences

Future attack or hit logic can construct a narrow Damage Intent without
changing Health, while no exact future producer architecture is promised.
Source and target generations remain governed by Entity and Actor. Target
Health may appear or disappear without changing the Intent value, and callers
control repeated explicit resolution. Store pairing remains their
responsibility under the Engine's current single-threaded model.
