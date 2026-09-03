# ADR-0042: Enemy Attack Intent / Decision Boundary

- Status: Accepted
- Release: Hertharian Engine v0.3.18

## Context

Released Enemy Decision can classify the current Target as `IDLE` or `PURSUE`,
and released Enemy Attack Eligibility can independently determine whether an
explicit Enemy/Target pair is eligible to attack. Production Pursuit consumes
the historical Decision API and assumes every non-IDLE intent proceeds through
Seek and Chase. It has no defined ATTACK movement or retained-velocity policy.

## Decision

Extend `HTHEnemyIntentKind` with `HTH_ENEMY_INTENT_ATTACK`, carrying the same
explicit generation-sensitive Target handle as `PURSUE`. Add the disconnected
internal `hth_enemy_decision_evaluate_with_attack()` variant while preserving
the historical Decision signature, results, and production callers.

The new evaluator canonicalizes output to `IDLE`, validates independent
caller-owned perception and attack ranges, and consumes only the stored Current
Target. Self Target produces `IDLE` without changing the relationship.
Perception is the outer gate. A perceptible Target is delegated to Enemy Attack
Eligibility; eligible produces `ATTACK`, while ineligible falls back to Enemy
LOS and produces `PURSUE` when clear or `IDLE` when blocked. Technical
Eligibility failure propagates as failure with canonical output.

Eligibility contains its own LOS check and exposes no reason for ineligibility.
The blocked in-range path may therefore perform a second fallback LOS. The
maximum of two LOS evaluations is accepted to preserve the released authority
boundaries and avoid a reason enum or combined query.

## Consequences

ATTACK-capable Decision remains deterministic, mutation-free, allocation-free,
headless-compatible, and O(N) in the number of static CollisionWorld obstacles.
It reads no Health, DynamicBody, velocity, yaw, facing, FOV, cooldown, or attack
execution state. ATTACK is semantic output only and creates no DamageIntent.

Production continues to call only the historical IDLE/PURSUE evaluator, so
v0.3.18 adds zero production Attack Eligibility work and no visible behavior
change. Runtime consumption of ATTACK, including Chase and existing
DynamicBody velocity handling, is deferred to v0.3.19.

## Rejected Alternatives

- Replace or alter the historical Decision API: this would force unresolved
  ATTACK semantics into production Pursuit.
- Evaluate attack before perception: an attack range larger than perception
  could then produce ATTACK outside the Decision awareness gate.
- Evaluate pursuit LOS before Eligibility: this duplicates LOS on the
  successful ATTACK path.
- Modify Eligibility to expose rejection reasons: that would reopen the frozen
  v0.3.17 contract solely to remove a bounded constant factor.
- Add attack execution, cooldown, damage, facing, stop, or velocity policy:
  these belong to later milestones.
