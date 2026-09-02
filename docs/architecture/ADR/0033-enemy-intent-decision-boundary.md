# ADR-0033: Enemy Intent / Decision Boundary

- Status: Accepted
- Milestone: v0.3.9

## Context

Enemy Target, radius Perception, static-world LOS, and explicit Target
Selection already provide the facts needed for a first behavioral decision.
The engine needs to represent the resulting desire without prematurely adding
execution, persistent behavior state, or a generic AI architecture.

## Decision

Introduce a small internal `HTHEnemyIntent` value with canonical `IDLE` and
`PURSUE(target)` states, produced synchronously by the stateless
`hth_enemy_decision_evaluate()` query. Decision consumes only the current
Enemy Target, then composes the released Perception and LOS authorities in
that order. It never selects or mutates a Target and never executes the Intent.

The query is deterministic, allocation-free, and observationally pure. It has
no Store, initialization, shutdown, cache, timer, memory, previous value, or
automatic Engine-loop caller. Technical failure is distinct from a valid
`IDLE` gameplay result.

## Rejected Alternatives

- A generic FSM, Behavior Tree, GOAP, or Utility AI framework would introduce
  architecture before the engine has behavior breadth requiring it.
- A persistent Decision component or Intent Store would turn a derived,
  ephemeral value into unnecessary synchronized state.
- An automatic per-frame think loop would add production work and scheduling
  policy outside this foundation.
- Fusing Decision with movement would violate the boundary between deciding
  and executing.
- Fusing Target Selection with Decision would conflate choosing a relationship
  with evaluating the current relationship.
- Reimplementing radius or collision logic would duplicate the established
  Perception and LOS authorities.

## Consequences

Identical current facts yield an identical Intent. Evaluation is `O(1)` until
LOS is required and then inherits Enemy LOS `O(N)` static-obstacle work, using
`O(1)` auxiliary memory. Seek, Chase, movement, combat, death processing,
memory, and richer AI policy remain future milestones.
