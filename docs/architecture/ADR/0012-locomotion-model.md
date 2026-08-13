# ADR-0012: Separate Locomotion Policy from Body and Collision

- Status: Accepted
- Milestone: v0.1.9

## Context

v0.1.8 assigned horizontal velocity directly from digital movement intent,
then applied gravity and constrained displacement through swept traces. This
validated collision but discarded momentum and provided no distinct ground,
air, friction, or jump policy. Placing those policies in Player Body would mix
physical state with character tuning; placing them in Collision would make a
geometric query layer decide gameplay movement.

## Decision

Hertharian separates locomotion velocity policy from physical body state and
geometric collision.

- Movement intent carries a normalized horizontal wish direction, a retained
  wish magnitude, and a jump pressed transition.
- Per-instance `HTHMovementConfig` owns ground speed, friction, stop speed,
  separate ground/air acceleration, air wish-speed limit, gravity, jump height,
  and maximum fall speed.
- Ground friction runs before directional ground acceleration.
- Air receives no friction and uses weaker directional acceleration with a
  limited wish speed, not a hard cap on existing momentum.
- Velocity remains in world space when view yaw changes.
- Jump launch velocity is derived from `jump_height` and gravity.
- The existing trace/slide/step path accepts generated velocity and retains the
  collision-clipped result as physical truth.

v0.1.9 supplies one default profile. The representation permits future
per-character profiles without introducing character identity or selection.

## Consequences

- Characters can eventually vary locomotion without rewriting Collision.
- `HTHPlayerBody` remains pure physical state with no mass or movement tuning.
- Collision remains geometric and owns no friction, acceleration, or jump.
- Momentum-preserving turns, reverse acceleration, limited air influence, and
  height-based jumping become explicit and testable.
- Jump height can later distinguish optional routes, secrets, and tactical
  traversal opportunities.
- Variable-delta integration remains approximate; this decision introduces no
  fixed tick, advanced air control, or network movement model.
