# ADR-0034: Enemy Seek / Steering Boundary

- Status: Accepted
- Milestone: v0.3.10

## Context

Enemy Decision can now express `PURSUE(target)`, but the engine needs a narrow
boundary between that behavioral intent and future physical execution. That
boundary must express desired geometry without prematurely owning motion,
obstacle handling, or a general steering architecture.

## Decision

Represent Enemy Seek as a stateless internal query that converts an explicit,
structurally valid Enemy/Target Spatial pair into a canonical desired
world-space direction. Exact zero displacement returns zero; every nonzero
displacement is normalized to unit length using clear square-root mathematics
and double intermediates promoted before subtraction.

Seek never applies movement, velocity, speed, acceleration, yaw, collision,
navigation, or persistent steering state. It does not own a Target or consume
EnemyTargetStore or Intent. Decision and Target Selection remain upstream
caller-owned policy; future Chase and runtime execution remain downstream.

## Rejected Alternatives

- Having Seek modify DynamicBody velocity or Spatial position would combine
  desired geometry with physical execution.
- A persistent Steering Store or component would retain a value that is
  directly and deterministically derived from current positions.
- Giving Seek Target ownership or an EnemyTargetStore dependency would hide
  orchestration and relationship policy inside geometry.
- Having Seek call Decision, Target Selection, Perception, or LOS would invert
  the established policy pipeline.
- Pathfinding or obstacle avoidance inside Seek would conflate straight-line
  direction with navigation.
- Arrival behavior, smoothing, prediction, and a general steering framework
  add policies not required by this foundation.

## Consequences

Seek is an allocation-free, observationally pure `O(1)` query with `O(1)`
additional memory. Its output is useful to future Chase but causes no runtime
or visible gameplay change in v0.3.10.
