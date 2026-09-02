# ADR-0035: Enemy Chase Application Boundary

- Status: Accepted
- Milestone: v0.3.11

## Context

Enemy Seek can derive a desired direction but intentionally performs no
action. The completed Chase feasibility audit found that Player Locomotion is
Player-specific and that the existing Entity + Spatial + DynamicBody path,
resolved by Dynamic Collision, is the correct reusable movement capability.
A generic locomotion prerequisite is therefore unnecessary.

## Decision

Implement Enemy Chase as a thin internal adapter. It applies an explicit
world-space direction and caller-owned speed by setting DynamicBody velocity
through its Store API, then delegates physical movement to
`hth_dynamic_collision_move()`. It propagates `HTHDynamicCollisionResult`,
preserves Dynamic Collision's resolved velocity, and restores the previous
velocity if a downstream technical failure occurs.

Chase does not own Target, Intent, speed, scheduling, facing, gravity, or
navigation. It calls no Selection, Decision, Seek, Perception, or LOS logic and
does not directly write Spatial or perform collision traces.

## Rejected Alternatives

- Reusing Player Locomotion directly would import Player-specific input,
  grounding, and movement policy.
- Duplicating Player movement would create parallel Enemy physics.
- Writing Spatial position directly or tracing collision inside Chase would
  bypass the generic Dynamic Collision authority.
- Giving Chase Target ownership or calls to Selection, Decision, or Seek would
  combine orchestration and application.
- Storing speed in Chase or renormalizing Seek output would seize caller-owned
  policy and alter the upstream contract.
- Automatic yaw/facing, gravity, pathfinding, or navigation would expand the
  movement boundary.
- A persistent Chase Store or automatic per-frame Chase loop would introduce
  state and scheduling reserved for later runtime milestones.

## Consequences

The adapter has `O(1)` work and memory with no direct heap allocation; total
movement inherits Dynamic Collision's `O(N)` static-obstacle tracing and
constant four-bump limit. v0.3.11 is movement-capable only through explicit
callers and tests. Production remains unscheduled until the future Pursuit
Runtime Loop.
