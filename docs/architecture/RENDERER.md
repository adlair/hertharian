# Renderer Bootstrap v0.1.3

Renderer Bootstrap introduces the first graphical frame path:

```text
Engine → Renderer frontend → OpenGL backend → Platform presentation → SDL3
```

## Boundary and Ownership

The Renderer frontend owns its backend and delegates resize and frame work.
The OpenGL backend owns the graphics context and render state. Platform remains
the owner of the SDL window and SDL lifecycle; its private graphics services
configure the window, manage the opaque native context handle, report pixel
dimensions, and swap buffers. Public headers expose neither SDL nor OpenGL.

Platform must configure graphics attributes before creating the window. In
graphical mode it requests an OpenGL-capable, resizable, double-buffered window
with 24 depth bits and 8 stencil bits. Renderer then requests and validates an
OpenGL 3.3 Core context. Higher compatible versions are accepted.

## Bootstrap Frame

Each graphical engine frame sets the current pixel-sized viewport, clears
color, depth, and stencil buffers, and presents the double-buffered result. The
clear color `(0.05, 0.02, 0.08, 1.0)` is a renderer-bootstrap diagnostic, not
final art direction. No shaders, triangle, geometry, textures, or scene exist.

Rendering and buffer presentation happen before work timing is measured, so
both are included in `frame_work_seconds`.

## Resize and HiDPI

Logical window resize updates engine window state. Logical resize and separate
framebuffer pixel-size events both cause Renderer to query the current pixel
dimensions from Platform and update the OpenGL viewport. The context is not
recreated, and logical size is never assumed to equal framebuffer size.

## VSync and Pacing

Renderer requests swap interval zero because engine timing owns the current
60 FPS pacing policy. Failure to disable VSync is reported as a warning and is
not fatal while the context remains usable. A future policy may coordinate
display synchronization and engine pacing explicitly.

## Headless Mode

`--headless` is known before Platform initialization. Platform initializes the
event foundation without a visible or OpenGL-capable window, and Renderer is
disabled. This supports CI and logic tests without pretending that a failed GL
context is headless operation:

```bash
./build/engine/hertharian-engine --headless --frames 3
```

This is only a headless lifecycle foundation, not a dedicated server.

## Graphical Validation

The SDL-selected backend and explicit native backends can be exercised with:

```bash
./build/engine/hertharian-engine
SDL_VIDEODRIVER=wayland ./build/engine/hertharian-engine
SDL_VIDEODRIVER=x11 ./build/engine/hertharian-engine
```

Each compatible graphical run should show the resizable `Hertharian` window
filled by the dark-purple diagnostic framebuffer and should close cleanly. No
automatic X11 fallback or Wayland-specific rendering workaround exists.

## Function Loading

The bootstrap uses only OpenGL functions exported by the system OpenGL library
and links through CMake's `OpenGL::GL` imported target. No extension loader is
needed for viewport, clear state, buffer clear, error query, or driver strings.
Later milestones that need modern extension entry points can use Platform's
private context boundary without exposing native types.

## Deliberately Excluded

v0.1.3 excludes shaders, triangles, buffers, textures, models, cameras,
materials, lighting, world/BSP rendering, postprocessing, Vulkan, SDL Renderer,
gameplay, and fixed simulation timing.
