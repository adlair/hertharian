# ADR-0047: Enemy Attack Runtime Integration

- Status: Accepted
- Release: Hertharian Engine v0.3.23

## Context

The released runtime already selects a Target and evaluates Decision exactly
once, but `ATTACK` only suppresses movement. Released foundations can build and
resolve DamageIntent and represent cadence, yet production needs a
generation-safe cadence owner and an explicit partial-progress transaction.

## Decision

Adopt a **dedicated Enemy Attack Cadence Store**. It owns one released
`HTHEnemyAttackCadence` per attached Runtime Enemy generation and participates
in Runtime Population spawn/despawn and Engine lifecycle. EnemyStore remains
the iteration authority; cadence lookup is direct and O(1).

**Extend Enemy Pursuit Runtime** rather than create a second runtime. For each
Enemy, advance cadence exactly once before geometry/target early exits,
evaluate Decision at most once, then dispatch IDLE, historical PURSUE, or
ATTACK. The historical name remains for minimal churn.

The exact ready ATTACK transaction is:

```text
cadence advance
-> Decision once
-> readiness
-> build DamageIntent
-> commit cadence
-> resolve DamageIntent
```

Builder failure leaves cadence uncommitted. `applied=false` is successful and
does not refund cadence. Technical resolve failure leaves already committed
cadence consumed. One Enemy emits at most one attack per step, including zero
interval and overshoot; there is no catch-up loop.

A Spatial non-Actor target that reaches ATTACK is a **technical runtime
failure**. The released builder remains the Actor-compatibility boundary; no
silent no-op or Player-specific rule is added.

## Consequences

Runtime Enemy composition gains a private generation-safe cadence association.
Cooldown advances independently of Target, intent, LOS, perception, and
movement. Damage and interval remain caller policy; bootstrap uses 10 damage
and one second. DamageIntent and Health remain the construction and arithmetic
authorities. Health zero has no death semantics.

The attack path allocates nothing per frame. The Store grows only during
attach/capacity growth. No public API, Level/Material format, asset, renderer,
platform, input, timing, or frozen gameplay-foundation change is required.

## Rejected Alternatives

- Put cadence in the payload-free `HTHEnemy` marker.
- Store one bootstrap-private cadence that cannot support multiple Enemies.
- Add a second runtime that reevaluates Decision, Eligibility, or LOS.
- Resolve Health before committing cadence.
- Refund cadence for `applied=false` or after technical resolve failure.
- Treat a non-Actor ATTACK target as a silent successful no-op.
- Duplicate Health arithmetic instead of resolving DamageIntent.
- Persist or queue DamageIntent.
- Loop while ready or accumulate catch-up attacks.
- Add death semantics to v0.3.23.
