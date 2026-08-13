# ADR-0009: Camera Controller Boundary

- Status: Accepted
- Milestone: v0.1.6

## Context

Interactive FPS navigation must interpret physical input and update view state,
but a mathematical camera must remain useful for spectator, cinematic, debug,
or externally controlled views. Relative pointer capture is also an operating-
system concern that must not leak SDL into either abstraction.

## Decision

Camera stores mathematical view state. Input interpretation and FPS navigation
belong to a separate controller. Platform-specific mouse capture remains behind
Platform.

Engine owns the bootstrap controller and coordinates capture requests between
HTH Input, the controller's logical capture state, and Platform. The controller
consumes HTH key/button/delta state and Timing delta; it never invokes Platform,
SDL, Renderer, or OpenGL. Renderer receives only the resulting camera/matrices.

## Consequences

- `HTHCamera` remains reusable independently of FPS controls.
- Controller orientation and movement are unit-testable without a window, GPU,
  SDL, or OpenGL.
- SDL relative-mode behavior stays isolated in the Platform backend.
- Capture failure leaves logical capture disabled and the application running.
- A confirmed relative-mode transition clears accumulated delta and Input
  suppresses relative motion through the next complete event-pump frame. This
  is a backend-neutral transition invariant, not device-ID filtering.
- Platform may dynamically select an SDL device explicitly identified as a
  relative-pointer source and filter other motion only while relative mode is
  active. If no explicit source exists, generic SDL translation remains intact.
  IDs and backend device names stay private to Platform.
- Future player movement, collision, gravity, semantic bindings, networking,
  and other camera controllers remain separate design work.
- Click-to-capture, Escape-to-release, WASD, sensitivity, speed, and safety
  clamp are temporary bootstrap policy rather than durable gameplay contracts.
