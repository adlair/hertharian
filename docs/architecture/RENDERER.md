# Renderer Architecture

The graphical frame path remains:

```text
Engine → Renderer frontend → OpenGL backend → Platform presentation → SDL3
```

Platform owns the SDL window and opaque graphics-context services. The renderer
frontend owns a backend and consumes the final engine camera plus resolved
static draw inputs at initialization. It obtains framebuffer pixel dimensions,
builds HTH Model/View/Projection matrices, and gives draw data and
matrices to the backend. The OpenGL backend owns context, pipeline, geometry,
uniform locations, and draw state. Public headers expose neither SDL nor
OpenGL.

As of v0.2.0, Engine composes that Camera from the physical eye plus View
Dynamics position/FOV offsets before submission. Renderer remains unaware of
View Dynamics and simply uses the resulting view and effective radian FOV.

## 3D Bootstrap Frame

Each drawable frame clears color, depth, and stencil, submits several instances
of one static local-space cube through the MVP/UV shader path, and presents.
The clear color is `(0.05, 0.02, 0.08, 1.0)`. Each draw supplies an externally
resolved base color and optional RGB8 image. The fragment shader multiplies a
texture sample by the base color when a texture exists and otherwise uses the
base color alone. These materials and textures are bootstrap diagnostics, not
Hertharian final art direction. Depth testing uses `GL_LESS`.

The frontend refreshes View from the engine camera before drawing. As of
v0.2.4, Engine and the Material layer resolve visible World objects before they
reach Renderer. Renderer contains no visual-class palette or Resource lookup;
it receives AABB/base-color/optional-image draw records once during
initialization. The OpenGL backend builds private model matrices and uploads
textures synchronously. It includes neither World nor Collision headers, and
GPU texture names never leave the backend.

The static cube stores position plus UV. Every face maps the full `[0,1]`
square independently; consistent mirroring is accepted for this bootstrap.
Textures use one texture unit, `GL_NEAREST` minification/magnification,
`GL_REPEAT`, and no mipmaps. Upload temporarily sets `GL_UNPACK_ALIGNMENT` to
1 for tightly packed RGB rows and restores the previous value afterward.
Renderer shutdown deletes every texture before destroying the graphics
context.

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
8 stencil bits. Programmable-pipeline entry points, texture-unit selection,
and required uniform functions are resolved after the context is current and stored per
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

The renderer still excludes public mesh/shader/buffer/material/texture APIs,
filesystem loading, material parsing, lighting, world/BSP rendering, scene
graphs, animation, gameplay, and camera input or movement. It supports only
resolved bootstrap base color plus one optional texture per draw.
