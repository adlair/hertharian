# ADR-0043: Enemy Attack Intent Runtime Integration

- Status: Accepted
- Release: Hertharian Engine v0.3.19

## Context

The released attack-capable Enemy Decision can return `IDLE`, `PURSUE`, or
`ATTACK`, but production Pursuit still calls the historical two-state Decision
API. Treating every non-IDLE value as pursuit would incorrectly pass ATTACK
through Seek, Chase, and Dynamic Collision. The runtime therefore needs an
explicit movement policy for the released intent without adding attack
execution.

## Decision

Migrate Enemy Pursuit Runtime to
`hth_enemy_decision_evaluate_with_attack()`. Its private step API gains an
independent finite nonnegative `attack_range` immediately after
`perception_radius`; every caller supplies it explicitly. The bootstrap value
is `1.25F`.

Consume intent through an exhaustive switch. `IDLE` suppresses pursuit
movement. `PURSUE` retains the released Seek, optional-DynamicBody, Chase, and
Dynamic Collision path. `ATTACK` suppresses Pursuit Runtime movement for the
frame: it calls no Seek, Chase, or Dynamic Collision and performs no Spatial or
velocity write. An unknown intent kind is a technical failure.

The current EnemyTarget remains stored in every valid intent branch and is
reevaluated next frame. Existing `DynamicBody.velocity` is retained during
ATTACK and is not guaranteed to be zero; Pursuit simply does not integrate it
while ATTACK remains active. A later PURSUE frame derives current Seek direction
and lets Chase replace velocity normally.

## Consequences

Enemy Attack Eligibility becomes production-reachable only through Decision.
Pursuit does not duplicate range, Perception, or LOS policy and adds no direct
Eligibility call. The blocked in-range Decision path may retain its released
maximum of two LOS evaluations, an accepted constant factor that does not
change the existing asymptotic bound or allocation behavior.

No attack execution, DamageIntent, Health mutation, cooldown, facing, stop
primitive, velocity zeroing, zero-direction Chase, or new Engine phase is
introduced. Production continues to reevaluate intent once per applicable
Enemy step.

## Rejected Alternatives

- Treat ATTACK as PURSUE: this would keep advancing through the selected attack
  region and erase the semantic distinction.
- Zero DynamicBody velocity: Pursuit does not own a stop primitive, and ATTACK
  requires no velocity write.
- Invoke Chase with a zero direction: that would execute movement machinery and
  overwrite retained velocity in the ATTACK branch.
- Reevaluate attack range, Perception, or LOS in Pursuit: Decision and
  Eligibility already own those policies.
- Execute damage or add cooldown: those are later combat boundaries.
