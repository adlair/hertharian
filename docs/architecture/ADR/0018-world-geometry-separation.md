# ADR-0018: Separate Collision and Render Geometry

- Status: Accepted
- Milestone: v0.2.5

## Context

Through v0.2.4 each visible World AABB implicitly became a rendered cube.
Although flags separated visibility from collision participation, shape
semantics remained coupled and could not represent non-box visual geometry
without changing collision.

## Decision

Static World objects carry explicit collision and render shapes over shared
spatial bounds. Collision supports `none` and `aabb`; rendering supports
`none`, `box`, and `wedge`. Flags must agree with shapes. Collision extraction
uses only bounds, collision shape, and the collidable flag. Render extraction
uses only bounds, render shape, the visible flag, and the existing visual-class
material bridge.

Level format v2 persists both shape properties and rejects historical v1.
Built-in immutable indexed BOX and WEDGE topology lives in a CPU-only Geometry
library. Renderer uploads one GPU mesh per primitive and no longer infers a
cube from collision state.

## Consequences

Visible-only render geometry and collision-only objects are valid. The
bootstrap wedge is deliberately pass-through; slope and triangle collision do
not exist. BOX/WEDGE remain built-in, object rotation and external meshes are
absent, and level v2 still uses bootstrap visual classes. Collision, render,
mesh, and eventual BSP representations can now evolve independently.
