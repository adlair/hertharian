# Renderer Architecture

The graphical frame path remains:

```text
Engine → Renderer frontend → OpenGL backend → Platform presentation → SDL3
```

Platform owns the SDL window and opaque graphics-context services. The renderer
frontend owns a backend and consumes an engine camera. It obtains framebuffer
pixel dimensions, builds HTH Model/View/Projection matrices, and gives those
matrices to the backend. The OpenGL backend owns context, pipeline, geometry,
uniform locations, and draw state. Public headers expose neither SDL nor
OpenGL.

## 3D Bootstrap Frame

Each drawable frame clears color, depth, and stencil, submits several instances
of one static local-space cube through the MVP shader path, and presents. The
clear color is `(0.05, 0.02, 0.08, 1.0)`. A per-draw `u_color` uniform assigns
bootstrap diagnostic colors: dark gray floor, blue-gray walls, violet corner,
green 0.20 step, gold 0.30 exact-limit step, orange-red 0.60 ledge, pink-red
generic box, and cyan/purple corridor references. These colors distinguish
manual physics test cases and are explicitly not Hertharian final art
direction. Depth testing uses `GL_LESS` as the baseline 3D semantic.

The frontend refreshes View from the engine camera before drawing. Since v0.1.7,
the backend builds private model matrices from the bootstrap Collision World
bounds. v0.1.8 expands those temporary references with a corridor, inside
corner, low step, exact-limit platform, high ledge, and box. This direct
Collision-to-Renderer bootstrap dependency is temporary. Physical AABBs remain
the source of truth; Renderer associates colors through a small parallel table
indexed only for this known bootstrap layout. Future World, Scene, and Material
systems will replace this temporary coupling. No such abstraction is introduced
in v0.1.8.

Rendering and presentation happen before work timing is measured, so both are
included in `frame_work_seconds`.

## Resize, HiDPI, and Minimization

Logical window and framebuffer pixel-size events both trigger a framebuffer
query. The pixel dimensions update the viewport and camera projection, keeping
aspect correct when logical and pixel sizes differ. A 0×0 framebuffer is
treated as temporarily non-drawable; render and present are skipped without
division by zero, then resume on a valid resize.

## Context and Function Loading

Graphical mode requests OpenGL 3.3 Core, double buffering, 24 depth bits, and
8 stencil bits. Programmable-pipeline entry points plus the two required matrix
uniform functions are resolved after the context is current and stored per
backend instance. Bootstrap calls exported by the system OpenGL library remain
linked through `OpenGL::GL`; no external loader is added.

Renderer requests swap interval zero because engine timing owns the current
60 FPS pacing policy. Failure is a warning, not a fatal error.

## Headless and Graphical Validation

`--headless` creates no window or graphics context and disables Renderer, while
math and camera remain usable without a GPU:

```bash
./build/engine/hertharian-engine --headless --frames 3
```

Graphical validation may use the naturally selected SDL backend or explicitly
request Wayland/X11. No backend is hardcoded and no fallback is implemented.

## Deliberately Excluded

The renderer still excludes public mesh/shader/buffer resources, textures,
materials, lighting, world/BSP rendering, scene graphs, animation, gameplay,
and camera input or movement.
