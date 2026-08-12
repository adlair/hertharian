# ADR-0006: Rendering APIs Stay Behind a Renderer Backend

## Status

Accepted

## Context

The engine now needs to produce and present graphical frames. Coupling engine
or future game code directly to OpenGL or SDL graphics types would make the
first backend inseparable from rendering policy and from native-window setup.
Platform must participate in context creation without taking responsibility
for rendering decisions.

## Decision

Rendering APIs are backend implementation details. Game and public engine APIs
must not depend directly on OpenGL or SDL graphics types.

The Renderer frontend owns backend lifecycle and frame requests. Its first
backend targets OpenGL 3.3 Core. Renderer owns the graphics context and render
state; Platform owns the SDL window and provides private services to create,
bind, destroy, and present a context and to query framebuffer pixel size.
Platform does not issue rendering commands.

Headless mode is explicit before Platform initialization. It creates no
graphics-capable window or Renderer and does not treat graphics failure as a
headless implementation.

## Consequences

- SDL graphics types remain confined to the Platform backend.
- OpenGL types and calls remain confined to the Renderer backend.
- OpenGL 3.3 Core is the initial backend baseline, not a permanent restriction
  on future renderer implementations.
- Renderer shutdown precedes destruction of the Platform window.
- Tests can exercise the headless lifecycle without a GPU or display server.
