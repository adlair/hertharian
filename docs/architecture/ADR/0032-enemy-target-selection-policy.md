# ADR-0032: Enemy Target Selection Policy

- Status: Accepted
- Milestone: v0.3.8

## Context

Enemy Target, radius Perception, and static-world Line of Sight already exist
as independent foundations. Gameplay now needs one bounded policy operation
that chooses among candidates explicitly supplied by its caller without adding
global discovery, automatic behavior, or persistent selection state.

## Decision

Enemy Target Selection consumes a caller-owned `const` candidate array. It
skips invalid candidates and self, filters each remaining candidate through
the released EnemyPerception query and then EnemyLOS, and ranks all eligible
candidates by nearest 3D squared distance. Ranking promotes coordinates before
subtraction and uses `double`; exact equal distance is broken by lower Entity
index. Candidate order and duplicates do not affect the winner.

Selection determines the complete winner before mutation and then invokes the
released EnemyTarget set API exactly once. Structural failure and success with
no winner preserve the existing Target. The current Target has no preference,
and absence of a winner never clears it. The operation is internal, explicit,
stateless, uses no candidate-list allocation, and is absent from the Engine
loop. The final existing Store set may grow its own indexed storage.

## Rejected Alternatives

- Scanning the Registry or World would conflate selection with candidate
  discovery and broaden work beyond the caller's explicit set.
- First-eligible selection or candidate-array order as priority would make the
  result order-dependent.
- Player hardcoding, Actor-only candidates, Enemy exclusion, or faction
  assumptions would introduce gameplay classifications not yet defined.
- Current-target preference, sticky targeting, hysteresis, randomness, threat,
  aggro, or Health scoring would add undeclared ranking policy and state.
- Automatically clearing Target would conflate selection with target lifecycle.
- Automatic per-frame selection would introduce behavior and recurring work.
- Duplicating Perception math or tracing Collision directly would violate the
  ownership of the released Perception and LOS foundations.
- Sorting, copying, or allocating a candidate list is unnecessary for a
  deterministic one-pass minimum.
- A generic AI or policy-callback framework would exceed this foundation.

## Consequences

Candidate filtering and ranking are deterministic in `O(M + K*N)` time,
worst-case `O(M*N)`, and `O(1)` auxiliary memory; a final Store growth retains
the existing EnemyTarget capacity-management cost. Candidate discovery,
target lifecycle, factions, memory, AI, locomotion, and combat remain separate
future policies.
