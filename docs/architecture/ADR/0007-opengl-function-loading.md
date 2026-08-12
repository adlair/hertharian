# ADR-0007: OpenGL Entry Points Are Loaded Per Context

## Status

Accepted

## Context

The programmable OpenGL pipeline uses entry points that cannot be assumed to
be available through direct system linkage on every supported platform. SDL3
can resolve them for the current context, but Renderer must remain independent
of SDL and public engine APIs must remain independent of both technologies.

## Decision

OpenGL entry points beyond the bootstrap set are loaded through the private
Platform graphics procedure lookup and stored per renderer/context instance.
OpenGL loading details must not leak into public engine APIs.

Platform translates its native SDL3 procedure lookup into an engine-private
generic function pointer. The OpenGL backend converts that pointer to the
calling-convention-aware types supplied by the system OpenGL headers. Loading
occurs after the context is current and before any loaded entry point is used.
Only the shader, program, vertex-array, buffer, attribute, and draw functions
required by v0.1.4 are loaded.

## Consequences

- Required entry points are validated for the active context and backend.
- Missing functions cause clean Renderer initialization failure.
- Function-table lifetime matches context lifetime and supports future reload
  if context recreation is introduced.
- Platform remains unaware of rendering commands and OpenGL types.
- No external OpenGL loader dependency is introduced at this stage.
