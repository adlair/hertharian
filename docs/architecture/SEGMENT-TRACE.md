# Point / Segment Trace Foundation

Hertharian v0.3.6 adds an internal Collision query for tracing one
zero-volume point over the finite closed segment between two absolute world
positions:

```text
P(t) = start + t * (end - start), 0 <= t <= 1
```

`hth_collision_world_trace_segment()` reuses `HTHTrace` and checks only the
static AABBs stored by `HTHCollisionWorld`. It has no gameplay, Entity, Actor,
Dynamic Body, material, layer, filter, renderer, or resource semantics.

## Result Contract

The first geometric contact is selected by the lowest segment fraction.
`fraction` is the parameter `t`; contact at the exact endpoint is therefore a
hit with fraction 1. Tied obstacle fractions retain the first CollisionWorld
obstacle, while tied slab-entry axes use the existing X, then Y, then Z
priority. Normals point outward from the contacted AABB face, and
`obstacle_index` identifies the CollisionWorld-owned static obstacle.

A clear trace reports `hit=false`, `fraction=1`, the requested end position,
zero normal, `SIZE_MAX` obstacle index, and false solid flags. An initial point
strictly inside static solid reports `hit=true`, `fraction=0`, and
`start_solid=true`. `all_solid` means the endpoint also remains strictly inside
the union of static solid volumes. A boundary point is touching rather than
inside; motion tangent to a boundary remains clear, consistent with the
existing Collision contract.

Zero-length input is valid. A point strictly inside solid reports
`start_solid` and `all_solid`; a point outside or exactly on a boundary reports
the canonical clear result. Obstacles entirely before the start or beyond the
endpoint are excluded by the closed `[0,1]` domain.

## Numerical and Runtime Policy

Start, end, and obstacle bounds must contain finite floats. Segment arithmetic
promotes every coordinate to `double` before subtraction, so two finite float
endpoints cannot overflow the displacement intermediate. The final fraction
is narrowed safely to the existing float field because it is constrained to
`[0,1]`. No epsilon, normalization, square root, heap allocation, cache, retry,
or broadphase is used.

The query checks all N stored static obstacles, so it is O(N) time and O(1)
additional memory. The existing 16-obstacle CollisionWorld limit is inherited.
An empty structural CollisionWorld is a valid clear input for this query.

Swept AABB remains a separate, unchanged operation requiring strictly
non-degenerate local `mins` and `maxs`. Segment trace never passes zero extents
through that API and has zero production callers or per-frame work in v0.3.6.
