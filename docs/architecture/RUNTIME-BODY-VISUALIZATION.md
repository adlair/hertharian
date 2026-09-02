# Runtime Body Visualization Foundation

Hertharian v0.3.16 introduces a private presentation boundary for the one
bootstrap Runtime Enemy. Simulation remains authoritative: Engine supplies the
existing explicit Enemy handle, and `runtime_body_visual` observes its current
Entity, Spatial, and DynamicBody state to build one stack-local renderer draw.
Enemy, Actor, Spatial, and DynamicBody acquire no rendering responsibility.

```text
bootstrap Enemy handle
  -> generation-safe Entity validation
  -> Spatial copy + DynamicBody copy
  -> HTHRendererTransientDraw
  -> Renderer frontend
  -> existing OpenGL opaque/depth-tested path
```

The adapter receives const gameplay Stores and performs no discovery, Store
scan, allocation, mutation, pointer retention, or transform caching. Entity
Registry, Spatial, and DynamicBody lookups are O(1). The Player Target Bridge
proxy is never selected by production and lacks DynamicBody, so it is not
renderable through this boundary.

## Extraction Result

`HTHRuntimeBodyVisualResult` distinguishes three outcomes:

- `READY` means a live generation-matching Entity has current Spatial and
  DynamicBody state and produced a valid draw.
- `NOT_RENDERABLE` means the Entity is dead/stale or Spatial/DynamicBody is
  absent. This is a successful presentation no-op; Engine submits no runtime
  draw and does not repair gameplay state.
- `ERROR` means required arguments, color, scale, or the resulting transform
  are structurally invalid. Engine reports this technical boundary and stops
  the run loop for normal cleanup.

Store APIs validate generation before copying components. Reusing an Entity
index therefore cannot make an old handle visualize the replacement lifetime.
No backing Store pointer enters Renderer.

## Transient Draw Contract

`HTHRendererTransientDraw` is private and contains only a Geometry primitive,
model matrix, and copied base color. It is a sibling of, not a replacement for,
`HTHRendererStaticDraw`. Static World draws remain resolved during Renderer
initialization and retained by the backend. Runtime draws exist only for the
synchronous `hth_renderer_frame()` call and are never retained.

The current visual uses the immutable built-in BOX, whose local bounds are
`[-0.5,+0.5]` on every axis. Its model matrix is:

```text
Translation(Spatial.position)
  * RotationY(Spatial.yaw)
  * Scale(2 * DynamicBody.half_extents)
```

For the bootstrap half-extents `(0.30, 0.90, 0.30)`, the resulting dimensions
are `(0.60, 1.80, 0.60)`. At current yaw zero, the rendered volume exactly
matches the axis-aligned body centered on Spatial.position. Spatial yaw is
copied as-is; facing is not derived from velocity, target, or Player state.

If a future body combines nonzero yaw with unequal horizontal half-extents, the
oriented visual may diverge from the current axis-aligned collision AABB. This
foundation deliberately adds neither oriented collision nor facing policy.

## Appearance and GPU Resources

Engine resolves the existing `materials/bootstrap/none.hthmat` through the
bootstrap Material authority. Its opaque white base color is copied into the
draw; its texture is `none`. This is temporary bootstrap visualization, not
final Enemy art direction.

OpenGL reuses the already-uploaded BOX VAO/VBO/EBO, shader program, model and
base-color uniforms, and existing depth state. Retained static draws execute
first, then transient opaque draws with texture use disabled, then presentation.
No shader, pass, texture, geometry upload, blending, lighting, shadow, model,
animation, or Enemy asset is added. `GL_DEPTH_TEST` with `GL_LESS` naturally
occludes the Enemy behind World geometry.

## Frame and Lifecycle Boundary

The production order is:

```text
Player Movement
  -> Player Target Bridge Sync
  -> Enemy Pursuit Runtime
  -> View Dynamics / Camera
  -> post-Pursuit runtime-body extraction
  -> static World draws
  -> transient Enemy draw
  -> present
```

Extraction therefore observes the same frame's resolved Enemy position and
cannot leave a retained ghost at its old position. Only graphical frames run
the adapter. Headless frames continue Player Movement, bridge synchronization,
and Pursuit without extraction or Renderer work; Enemy existence never depends
on graphics.

The visualization needs no initialization, shutdown function, renderer object,
scene node, or Engine state. Shared Geometry follows Renderer lifetime, the
Material set follows existing Engine storage lifetime, and the stack draw is
discarded after submission. Enemy despawn and all simulation cleanup remain the
v0.3.15 lifecycle.

## Deliberately Excluded

There is no global Entity/Spatial/Body/Enemy scan, generic ECS renderer, scene
graph, render component, retained runtime object, interpolation, multiple
production runtime visuals, Player proxy visual, Enemy model/prefab/definition,
animation, visual facing, attack/combat, dynamic-vs-dynamic collision, Level or
Material format change, or public rendering API. Future callers may reuse the
explicit-handle adapter without changing these boundaries.
