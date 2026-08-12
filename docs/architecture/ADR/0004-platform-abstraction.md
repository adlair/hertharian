# ADR-0004: SDL3 Is Encapsulated by the Platform Layer

## Status

Accepted

## Context

The engine needs operating-system integration for window creation, system
events, monotonic timing, and sleeping. SDL3 provides these capabilities on the
current Linux target and can support future platforms, but coupling the engine
API directly to SDL types would make the dependency difficult to replace and
would spread platform details into unrelated subsystems.

## Decision

SDL3 is an implementation detail of the platform layer and must not leak into
the public engine API.

The private `src/platform/platform.h` defines the engine-owned platform
interface and an opaque `HTHPlatform` state. The initial SDL3 backend owns the
SDL window and provides initialization, event pumping, shutdown, monotonic
counter and frequency queries, and nanosecond sleep.

The engine owns one platform instance. Initialization failure is returned to
the engine after partial resources are released. A quit or window-close event
is returned to the engine, which stops its run loop and performs normal
shutdown; the platform layer never terminates the process directly.

On native Wayland, creating and showing a window is not sufficient for visual
mapping: the client must present an initial buffer. v0.1.1 deliberately does
not create a renderer or temporary presentation surface, so native Wayland is
validated for window creation and event-loop integration only. Initial buffer
presentation belongs to Renderer Bootstrap.

## Consequences

- Public headers contain no SDL headers, symbols, or types.
- SDL3 can be replaced or complemented by another backend behind the private
  platform interface.
- Window and SDL lifetime follow the central engine lifecycle.
- SDL3 remains a required build dependency for the initial backend.
- Rendering and playable input remain outside the platform foundation.
- Visual validation for v0.1.1 uses X11/XWayland; native Wayland visual
  presentation is deferred until Renderer Bootstrap presents the first buffer.
