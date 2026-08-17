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

## Keyboard Release Reconciliation

Window-system transitions can make an SDL key-up event unavailable even after
the physical key has been released. Platform retains a private set of
translated keys it has already reported as down. After fully draining the SDL
event queue, it compares only that set with the active backend observation and
emits a single normalized HTH key-up for each stale entry. Normal key events
remain the source of pressed edges and the primary source of released edges.
Each newly reported key-down must pass one backend observation before a later
observed-up discrepancy is allowed to normalize its release. This prevents a
temporarily lagging X11 snapshot from cancelling a fresh post-interruption
press during the same logical input update, while a genuinely missing release
is still recovered on the following observation.

The reconciliation policy is backend-independent and never synthesizes
key-down, polls into Input directly, grabs the keyboard, uses a timeout, or
names a desktop environment. Focus loss clears Platform's reported set as
Input clears held state; focus gain does not infer presses from observation.
Each normalized release traverses the ordinary HTH event and Input route. A
late real key-up is idempotent and a later real key-down starts a new press.
If SDL subsequently reports a repeat key-down for a still-held key, that event
re-establishes HTH held state but does not synthesize a `pressed` edge.
The single-observation confirmation is state-based rather than a timer,
threshold, key-specific rule, or synthesized key-down.

The X11 backend can keep both its SDL event state and SDL keyboard snapshot
stale when a release is intercepted outside the application. Builds with Xlib
therefore use a private observer that reuses SDL's
`SDL_PROP_WINDOW_X11_DISPLAY_POINTER` connection and calls `XQueryKeymap`.
SDL's backend passes the native X keycode through `SDL_KeyboardEvent.raw`, so
the observer records that value alongside the translated SDL scancode instead
of assuming a numeric offset. The X11 server bitmap is the independent
authority for recovering exclusively lost releases; SDL's cached state remains
diagnostic in this path. An X11-up/SDL-cached-down pair is explicitly a
disagreement, not proof that both sources concur. Debug mode reports both
states, reconciliation arming, its normalized action, and the HTH logical
state before and after event processing.

Xlib is a private, target-scoped optional dependency. The observer is compiled
only when X11 development support is available and activated only when SDL's
current video driver is X11. Native Wayland never invokes it.

## Relative Motion Source Selection

Some SDL X11/XWayland environments expose separate absolute and explicit
relative-pointer devices. Platform enumerates SDL mice and dynamically selects
a device whose name has the stable `xwayland-relative-pointer` prefix, without
depending on its generated suffix or numeric ID. While window-relative mode is
active and that explicit source is known, only motion from the selected source
crosses into `HTHPlatformEvent`; other SDL motion remains visible in debug logs
but is not translated. A foreign-source motion in this state also establishes
an observed re-entry discontinuity. Its payload arms a one-event compensation
discard but never produces logical movement. Platform discards exactly the
next motion from the selected relative source as transition compensation, then
accepts the following selected-source motion immediately.

Accepted motion always translates SDL `xrel/yrel` one-to-one to HTH `dx/dy`.
Platform does not retain, integrate, or reconstruct motion across events. The
`xwayland-relative-pointer` name selects which source is permitted while
relative mode is active; it does not imply delta-of-delta delivery or select a
different numerical interpretation. Relative-mode and selected-device changes
clear pending compensation. Focus and mouse-focus changes cancel it. The policy
does not inspect magnitude or sign and has no timing, resolution, or camera
dependency.

Mouse add/remove events refresh this private selection. If enumeration fails or
no explicit relative-pointer device exists, Platform preserves generic SDL
semantics, clears pending compensation, and translates each `xrel/yrel`
one-to-one without source filtering. Native Wayland,
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
