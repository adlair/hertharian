# ADR-0014: Make World the Static Content Source of Truth

- Status: Accepted
- Milestone: v0.2.1

## Context

Through v0.2.0 Collision owned the bootstrap AABB list and Renderer depended on
that list and its indices to reconstruct visible geometry and colors. Engine
also embedded the player spawn. This made a collision backend act as scene
storage and coupled diagnostic rendering to collision-array order.

## Decision

Hertharian introduces an internal, engine-owned `HTHWorld` built and finalized
before Collision and Renderer.

- World owns static-object bounds, independent collidable/visible flags,
  bootstrap diagnostic visual classes, aggregate bounds, and default spawn.
- Collision copies the collidable AABBs at initialization and retains its own
  backend-local indices.
- Renderer frontend reads visible World objects and converts visual classes to
  draw colors; the OpenGL backend receives only derived bounds and colors.
- `HTHAABB` remains geometry-only, and neither Renderer backend nor public API
  receives World internals.
- World becomes immutable after finalization and performs no frame-time
  allocation or mutation.
- AABB is the current bootstrap primitive, not a constraint on future loaders
  or BSP-backed world representations.

## Consequences

Bootstrap content and spawn now have one authoritative representation while
Collision and rendering remain separately testable consumers. Visible-only and
collidable-only objects have defined behavior without forcing either subsystem
to depend on the other. The temporary diagnostic visual class is intentionally
smaller than an Entity, Scene, Mesh, or Material system; those remain future
work.
