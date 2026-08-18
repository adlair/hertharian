# ADR-0030: Dedicated Finite Point / Segment Trace

- Status: Accepted
- Milestone: v0.3.6

## Context

Collision already exposes swept AABB tracing, whose established contract
requires strictly non-degenerate extents. Future geometry queries need an
exact zero-volume point moving over a finite segment without weakening that
contract or approximating the point with an arbitrary volume.

## Decision

Collision exposes a separate internal
`hth_collision_world_trace_segment()` query. It accepts absolute start and end
positions, traces the closed `[0,1]` point segment against the original static
CollisionWorld AABBs, and returns the existing `HTHTrace` vocabulary. Its
segment-specific slab path uses double intermediates; swept AABB retains its
historical implementation and validation unchanged.

The query is pure, allocation-free, static-world-only, and unused by
production in v0.3.6. It establishes Collision capability only; Enemy
line-of-sight and all gameplay interpretation remain later work.

## Rejected Alternatives

- Passing zero extents through swept AABB or relaxing `mins < maxs` would
  change its existing contract.
- A tiny AABB or epsilon volume would approximate rather than trace a point.
- A gameplay raycaster would duplicate Collision geometry authority.
- An infinite ray API would not encode the required finite endpoint.
- Endpoint epsilon would weaken the closed-segment contract.
- Changing Dynamic Collision would introduce an unrelated behavior change.
- A generic Shape Trace framework, Ray object, filters, or premature
  broadphase would expand scope beyond one segment primitive.
- Implementing Enemy line-of-sight here would combine Collision capability
  with gameplay policy before its own milestone.

## Consequences

Collision has two explicit trace foundations: swept non-degenerate AABB and
finite zero-volume segment. Both traverse the current static obstacle array,
reuse `HTHTrace`, inherit its obstacle limit, and remain internal. Dynamic
objects, materials, layers, filters, batching, caching, threading, and
gameplay callers remain absent.
