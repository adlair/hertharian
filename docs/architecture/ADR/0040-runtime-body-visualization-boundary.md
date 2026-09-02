# ADR-0040: Runtime Body Visualization Boundary

- Status: Accepted
- Release: Hertharian Engine v0.3.16

## Context

The production bootstrap Enemy introduced in v0.3.15 is a live Entity with
Spatial and DynamicBody state and moves through Enemy Pursuit Runtime, but the
Renderer consumes only retained static World draws. The OpenGL backend already
owns canonical BOX geometry, model/color uniforms, an opaque shader path, and a
depth buffer. What is missing is a frame-time presentation input.

## Decision

Represent the explicitly selected bootstrap Enemy by extracting its current,
generation-safe Entity + Spatial + DynamicBody state through the private
`runtime_body_visual` adapter. The adapter produces one stack-local
`HTHRendererTransientDraw` containing BOX, a model matrix, and the copied opaque
white base color from `materials/bootstrap/none.hthmat`.

The model is `Translation(position) * RotationY(yaw) *
Scale(2*half_extents)`. The authoritative Enemy handle remains
`bootstrap_enemy_pursuit.enemy`; no visual identity is cached. Engine extracts
after Pursuit and Camera preparation and synchronously supplies zero or one
transient draw to Renderer. Static World draws remain retained and execute
first; transient draws execute second in the same shader/depth-tested opaque
frame before presentation.

The adapter returns READY, NOT_RENDERABLE, or ERROR. Dead/stale Entities and
missing Spatial/Body are valid non-renderable presentation states. Invalid
arguments or unrepresentable visual data are technical errors. All gameplay
inputs are const, all Store access uses generation-safe APIs, and neither
Renderer nor OpenGL receives gameplay Stores.

Headless skips extraction entirely while retaining the same Enemy simulation.
No persistent visual state, per-frame allocation, Store scan, GPU upload,
initialization, or visualization cleanup is introduced.

## Consequences

The moving Runtime Enemy becomes visible as a solid white `0.60 x 1.80 x 0.60`
bootstrap box. Shared BOX resources, shaders, uniforms, depth testing, and
presentation remain authoritative. Walls occlude the Enemy naturally, and the
draw reflects post-Pursuit Spatial without a retained one-frame-old transform.

Yaw is observed exactly but remains zero because Pursuit does not own facing.
Future nonzero yaw, especially with unequal X/Z extents, may make an oriented
visual diverge from the axis-aligned collision AABB; resolving that is a later
physics/presentation decision.

## Rejected Alternatives

- Renderer state in Enemy or Actor: reverses the simulation/presentation
  dependency.
- Renderer querying EntityRegistry, EnemyStore, SpatialStore, or BodyStore:
  couples rendering to gameplay discovery and ownership.
- Render every Spatial, Body, or Enemy: would expose proxies and establish an
  unapproved global renderability rule.
- Reuse `HTHRendererStaticDraw`: its init-time retained bounds contract would
  freeze the Enemy transform.
- Rename or rebuild the static rendering architecture: unnecessary for one
  sibling transient submission.
- Fake static World Enemy: duplicates identity and cannot track simulation.
- Scene graph, ECS renderer, retained runtime node, visual component, transform
  cache, or renderer handle: persistent machinery is unnecessary.
- Enemy model, prefab, loader, skin, texture, animation, or velocity-derived
  facing: outside the visualization foundation.
- Render the Player proxy: it is semantic target identity, not presentation.
- Per-frame allocation or GPU geometry upload: the stack draw and shared BOX
  already satisfy the requirement.
