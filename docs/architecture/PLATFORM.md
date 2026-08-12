# Platform Foundation v0.1.1

The platform layer is the engine's private boundary around operating-system
integration. Its initial backend uses SDL3 and is implemented entirely under
`engine/src/platform/`.

## Responsibilities and Encapsulation

The layer initializes SDL video, creates the `Hertharian` window at 1280x720 by
default, pumps system events, reports quit requests, exposes a monotonic
high-resolution counter and its frequency, provides millisecond sleep, and
releases its resources.

`platform.h` is private to the engine target. Only `platform_sdl3.c` includes
SDL headers or stores an `SDL_Window`. Public headers use an opaque
`HTHPlatform` declaration, so no SDL type crosses the engine API boundary.

## Lifecycle and Ownership

`hth_engine_init` supplies window configuration and creates one platform
instance. That instance owns the SDL window. Each `hth_engine_frame` pumps
events; a quit or window-close event sets `engine.running` to false through the
engine layer. `hth_engine_shutdown` then destroys the window and shuts SDL down.
Failed initialization releases any resources acquired before the failure.

## Timing Foundation

The platform API exposes SDL's monotonic performance counter and counter
frequency, plus millisecond sleep. A 16-millisecond sleep currently prevents
the bootstrap loop from busy-spinning. This is a temporary bootstrap throttle,
replaced by real frame timing in v0.1.2; it is not a definitive frame limiter.

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
SDL_VIDEODRIVER=dummy ./build/engine/hertharian-engine --frames 3
```

CTest sets the dummy video driver for its platform lifecycle test.

SDL3 selects its video backend naturally. The platform layer does not set
`SDL_VIDEODRIVER`; desktop testing can leave it unset or explicitly request
`wayland` or `x11` through the environment.

## Wayland Presentation Boundary

Native Wayland requires a client to present an initial buffer before the
compositor can map a window visually. Platform Foundation creates and
explicitly shows the SDL window, but deliberately does not create a renderer,
OpenGL context, or temporary software surface merely to present that buffer.
Initial presentation belongs to Renderer Bootstrap.

The v0.1.1 window was visually validated through X11/XWayland. Native Wayland
was validated for SDL initialization, window creation, event-loop operation,
and clean shutdown; its visual presentation will be validated when Renderer
Bootstrap supplies the first buffer. No X11 fallback is selected in code.

## Deliberately Excluded

v0.1.1 does not add a renderer, OpenGL context, shaders, world or BSP support,
audio, playable input, bindings, mouse capture, gamepad support, networking,
filesystem, console, cvars, game code, gameplay, or sophisticated frame timing.

The implementation is original project code. ioquake3 was studied only as an
architectural reference; no source implementation was copied.
