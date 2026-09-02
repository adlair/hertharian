# ADR-0041: Enemy Attack Eligibility Boundary

- Status: Accepted
- Release: Hertharian Engine v0.3.17

## Context

Released foundations can represent an explicit Enemy and Target, measure
Enemy Perception range, and evaluate static-world Enemy LOS. Production can
spawn, pursue, collide, and render a bootstrap Enemy, but no boundary answers
the narrower question of whether an explicit pair is currently eligible for a
future attack. Executing an attack would additionally require policy for
intent, pursuit, cooldown, damage, and presentation that this release does not
own.

## Decision

Represent Enemy Attack Eligibility as a stateless internal explicit-pair
query. It validates a live Actor+Enemy+Spatial Enemy and a live Spatial Target,
validates a caller-owned finite nonnegative `attack_range`, delegates range
exclusively to released Enemy Perception, and evaluates released Enemy LOS only
after range succeeds. The query returns a technical bool plus a canonicalized
gameplay `out_eligible` bool.

Self-target is structurally valid but ineligible after full semantic
validation. Health, DynamicBody, velocity, yaw, facing, FOV, movement state,
and `HTHEnemyTargetStore` do not participate. The Player Target Bridge proxy is
valid through the ordinary Entity+Spatial Target contract. LOS retains its
static-only occlusion and conservative trace-failure behavior.

The boundary stores no eligibility, allocates no memory, mutates no state,
selects no Target, creates no DamageIntent, and is not called by production
Decision or Pursuit in v0.3.17. Its pre-LOS cost is O(1), and its worst-case
cost is O(N) over static CollisionWorld obstacles with O(1) auxiliary space.

## Consequences

Future attack policy can consume a deterministic, generation-safe eligibility
answer without duplicating distance or trace geometry. Technical failures stay
distinct from ordinary gameplay-negative results. A LOS-negative result does
not expose whether the cause was an obstruction or conservative Segment Trace
failure. Entity handles still rely on callers pairing them with their
originating Registry and Stores.

## Rejected Alternatives

- Hide an `HTHEnemyTargetStore` dependency or perform Target Selection inside
  Eligibility: the Target is explicit and selection remains independent.
- Duplicate distance math or call Collision/Segment Trace directly: Perception
  and Enemy LOS are the released authorities.
- Measure from DynamicBody, PlayerBody, Camera, or a Player-specific anchor:
  this would replace the Spatial Target contract.
- Gate on Health, facing, FOV, faction, team, movement, or hostility: those
  policies do not exist in this foundation.
- Add cooldown, attack types, weapons, animation, DamageIntent, Health
  mutation, or an `ATTACK` intent: these execute or define attacks rather than
  answer eligibility.
- Persist eligibility in a Store or cache: the answer is derived from current
  caller-owned state.
- Integrate with production Decision, Chase, or Pursuit: the response policy
  when eligibility becomes true is intentionally deferred.
