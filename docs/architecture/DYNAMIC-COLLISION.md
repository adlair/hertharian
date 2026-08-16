# Dynamic Collision Foundation

Hertharian v0.2.9 provides a one-Body movement operation separate from Dynamic
Body storage. It reads position from Spatial, reads shape and velocity from the
Body Store, and resolves supplied linear motion only against the existing
static `HTHCollisionWorld`.

## Motion Contract

`dt` must be finite and nonnegative. Zero `dt` is a successful canonical
no-op. For positive `dt`, the solver constructs centered local AABB extents and
requests the existing swept AABB trace for:

```text
end = Spatial.position + DynamicBody.velocity * dt
```

The sweep prevents thin static obstacles from being skipped by large
displacements. No discrete end-overlap path exists. A visible-only WEDGE never
enters `HTHCollisionWorld` and therefore does not block a Body.

At a hit, the solver moves to trace contact, removes the velocity component
entering the blocking normal, and continues with the remaining time. This
preserves tangential velocity for wall sliding and provides no bounce or
friction. Up to four impacts are processed by one call; if time remains after
that conservative limit, velocity becomes zero. Static AABB normals are
axis-aligned, so no general manifold solver is introduced.

Resolved position is written through the Spatial Store API and resolved
velocity through the Body Store API. Yaw is preserved and ignored by
collision. Spatial can be moved independently before a call; the solver always
uses its current value and keeps no hidden Body position.

## Result and Exceptional Cases

The result contains only `moved`, `collided`, and `start_solid`. Invalid calls,
missing Body, stale Entity, or absent Spatial fail with a canonical false
result and no state mutation. A Body that starts inside static collision
reports `start_solid=true`, `collided=true`, and `moved=false`; position and
velocity remain unchanged. There is no depenetration.

The solver applies no gravity, acceleration, grounded detection, restitution,
friction, drag, step climbing, ground snap, or Player locomotion policy. It
does not perform Dynamic-vs-Dynamic collision, broadphase, triggers, layers,
substeps, or automatic Engine-frame stepping. Tests invoke one-Body movement
explicitly; production creates no Bodies and performs no Dynamic Collision
work per frame.
