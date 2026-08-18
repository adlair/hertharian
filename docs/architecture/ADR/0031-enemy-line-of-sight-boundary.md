# ADR-0031: Enemy Line-of-Sight Boundary

- Status: Accepted
- Milestone: v0.3.7

## Context

Collision v0.3.6 already owns an exact finite, closed, zero-volume segment
trace against static CollisionWorld geometry. Gameplay needs a minimal answer
for whether the segment between an Enemy position and an arbitrary candidate
Entity position is statically unobstructed, without conflating that answer
with target relation, radius perception, target selection, or AI.

## Decision

Enemy Line-of-Sight is a pure gameplay interpretation of the released
`hth_collision_world_trace_segment()` result. It validates a live Actor,
Enemy, and Spatial observer and a live Spatial candidate, treats exactly
identical positions as clear gameplay LOS, then maps one distinct-position
trace to `!trace.hit`. Trace failure is conservatively false.

The query is internal, synchronous, stateless, allocation-free, center-to-
center, static-world-only, and independent from Enemy Target, Enemy
Perception, future target selection, and future AI. It owns no Engine state or
production call site.

## Rejected Alternatives

- Storing LOS in Enemy, Enemy Target, a visibility table, or a cache would add
  persistent derived state and invalidation policy.
- Automatically clearing occluded targets or acquiring visible candidates
  would conflate a query with target mutation and selection.
- Bundling FOV or radius would merge independent geometric policies.
- Duplicating segment mathematics, adding a gameplay raycaster, or using a
  tiny AABB would violate Collision geometry ownership and point exactness.
- An epsilon endpoint or fraction policy would contradict the released closed
  segment and `trace.hit` contract.
- Actor or Dynamic Body occlusion and material-aware transparency require
  future dynamic/filtering policy absent from CollisionWorld.
- Renderer/depth visibility would make gameplay depend on presentation.
- Bundling AI, behavior, locomotion, or combat would exceed this foundation.

## Consequences

LOS remains an O(N), O(1)-memory read-only query over the current static
obstacle array. Target relation and radius Perception can disagree with LOS by
design. Dynamic occlusion, visibility caching, selection, and AI remain future
work.
