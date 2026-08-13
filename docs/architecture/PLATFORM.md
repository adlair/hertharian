# Platform Foundation v0.1.1

The platform layer is the engine's private boundary around operating-system
integration. Its initial backend uses SDL3 and is implemented entirely under
`engine/src/platform/`.

## Responsibilities and Encapsulation

The layer initializes SDL video, creates the `Hertharian` window at 1280x720 by
default, pumps system events, reports quit requests, exposes a monotonic
high-resolution counter and its frequency, provides nanosecond sleep, and
releases its resources.

v0.1.6 also exposes a private boolean relative-mouse operation backed by the
window-specific SDL3 API. It neither exposes SDL types nor adds keyboard grab,
manual cursor visibility, explicit mouse grab, or pointer warping. Engine owns
the capture policy; Platform only attempts the requested OS state transition
and reports failure without aborting.

The optional `--debug-fps-input` development flag enables focused diagnostics
without changing event translation. Platform reports the initial SDL mouse
device inventory, relative motion and device ID, relative-mode/focus state, and
button origin. Engine adds capture transitions, focus changes, translated HTH
delta, discarded transition motion, and resulting controller angles. Normal
execution prints none of this detail. Device IDs are diagnostic observations,
not configured or hardcoded policy; any selected ID is discovered dynamically
from the current SDL inventory.

## Relative Motion Source Selection

Some SDL X11/XWayland environments expose separate absolute and explicit
relative-pointer devices. Platform enumerates SDL mice and dynamically selects
a device whose name has the stable `xwayland-relative-pointer` prefix, without
depending on its generated suffix or numeric ID. While window-relative mode is
active and that explicit source is known, only motion from the selected source
crosses into `HTHPlatformEvent`; other SDL motion remains visible in debug logs
but is not translated.

Mouse add/remove events refresh this private selection. If enumeration fails or
no explicit relative-pointer device exists, Platform preserves generic SDL
semantics and translates mouse motion without source filtering. Native Wayland,
native X11, single-mouse systems, and other SDL backends therefore do not depend
on XWayland device naming. No SDL ID or device name crosses Platform.

As of v0.1.3, graphical mode creates a resizable OpenGL-capable window. Platform
owns that SDL window and provides private context/presentation services, while
Renderer owns graphics context lifetime and all rendering decisions.

`platform.h` is private to the engine target. Only `platform_sdl3.c` includes
SDL headers or stores an `SDL_Window`. Public headers use an opaque
`HTHPlatform` declaration, so no SDL type crosses the engine API boundary.

## Lifecycle and Ownership

`hth_engine_init` supplies window configuration and creates one platform
instance. That instance owns the SDL window. Each `hth_engine_frame` pumps
events and translates them into engine-owned event types. A quit event sets
`engine.running` to false through the engine layer. `hth_engine_shutdown`
disables active relative mode before Renderer and Platform teardown, then
destroys the window and shuts SDL down. Failed initialization releases any
resources acquired before the failure.

## Timing Foundation

The platform API exposes SDL's monotonic performance counter and counter
frequency, plus nanosecond sleep. v0.1.2 uses these primitives through the
engine-owned timing system documented in `TIMING.md`.

Frames are printed only for finite `--frames N` runs. Unlimited runs remain
quiet instead of continuously filling standard output.

## Running

Run until the window is closed:

```bash
./build/engine/hertharian-engine
```

Run a deterministic number of frames:

```bash
./build/engine/hertharian-engine --frames 3
```

For CI or another environment without a display:

```bash
./build/engine/hertharian-engine --headless --frames 3
```

CTest uses this explicit headless path for its platform lifecycle test.

SDL3 selects its video backend naturally. The platform layer does not set
`SDL_VIDEODRIVER`; desktop testing can leave it unset or explicitly request
`wayland` or `x11` through the environment.

## Wayland Presentation Boundary

Native Wayland requires a client to present an initial buffer before the
compositor can map a window visually. Platform Foundation creates and
explicitly shows the SDL window, but deliberately does not create a renderer,
OpenGL context, or temporary software surface merely to present that buffer.
Renderer Bootstrap now presents a real double-buffered OpenGL framebuffer,
allowing SDL3 to commit the buffer required for native Wayland mapping.

The v0.1.1 window was visually validated through X11/XWayland. Native Wayland
was validated for SDL initialization, window creation, event-loop operation,
and clean shutdown. v0.1.3 is prepared to validate native Wayland presentation
through its first real framebuffer. No X11 fallback is selected in code.

## Deliberately Excluded

The platform foundation does not add world or BSP support, audio, semantic
bindings, keyboard capture, gamepad support, networking, filesystem, console,
cvars, game code, or gameplay.

The implementation is original project code. ioquake3 was studied only as an
architectural reference; no source implementation was copied.
