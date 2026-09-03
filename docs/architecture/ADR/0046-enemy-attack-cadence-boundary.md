# ADR-0046: Enemy Attack Cadence Boundary

- Status: Accepted
- Release: Hertharian Engine v0.3.22

## Context

Enemy Decision can express `ATTACK`, Pursuit Runtime suppresses movement for
that intent, and Attack Execution can build one explicit DamageIntent. Direct
runtime composition would otherwise permit an attack on every applicable
frame. This milestone needs only the deterministic temporal primitive, without
choosing production ownership or coupling time to an Enemy Store.

## Decision

Represent Enemy attack cadence as caller-owned state containing exactly one
`double remaining_seconds`. Fresh or reset state is zero and immediately ready.
The caller supplies both simulation delta and the interval; the primitive reads
no clock.

Advance subtracts a finite nonnegative simulation delta and saturates at zero.
Commit is explicit, succeeds only while exactly ready, and accepts a finite
nonnegative interval, including zero. Readiness is exact zero. Overshoot grants
one ready opportunity and carries no attack credit, so no catch-up burst is
possible. Queries never consume cadence.

The foundation has no Store, Enemy handle, target, damage, Health, Intent,
Decision, Eligibility, Spatial, Collision, or production integration. Future
generation-safe ownership and runtime transaction order remain separate design
work.

## Consequences

The state is deterministic, independently instantiable, allocation-free, O(1),
and straightforward to reset after invalid external state. Invalid numeric
input and invalid state fail without mutation. Exact-double readiness means
some decimal frame partitions may retain a tiny positive remainder until a
later positive delta saturates it; no epsilon is hidden in the primitive.

v0.3.22 remains a disconnected foundation: production performs zero cadence
calls and no per-frame cadence work.

## Rejected Alternatives

- Add fields to `HTHEnemyStore`: changes the released presence-only Enemy role.
- Add `HTHEnemyAttackCadenceStore` now: prematurely decides runtime ownership
  and generation lifecycle.
- Store absolute `next_attack_time` or `last_attack_time` plus a sentinel:
  couples the primitive to a clock domain and complicates canonical state.
- Read wall-clock or platform time: breaks caller-owned simulation timing.
- Duplicate readiness in a boolean: creates two sources of truth.
- Consume cadence automatically on query: conflates observation with an attack
  transaction.
- Carry overshoot or issue catch-up attacks: creates burst semantics outside
  the milestone.
- Make cooldown target-specific: adds target identity and policy.
- Tick only for a particular Intent: couples temporal state to Decision policy.
- Wire cadence into production: requires the deferred generation-safe owner and
  attack transaction.
