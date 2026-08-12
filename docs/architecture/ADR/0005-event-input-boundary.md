# ADR-0005: Native Events Are Translated Before Input Processing

## Status

Accepted

## Context

Platform backends receive native window, keyboard, and mouse events. Allowing
native event types to reach engine control or input state would couple those
systems to SDL3 and make another platform backend difficult to introduce.
Events also describe occurrences, while input queries require persistent and
per-frame state derived from those occurrences.

## Decision

Platform backends translate native events into engine-owned event types. Input
state is derived from those engine events. Native SDL event and input types
must not cross the platform boundary.

The SDL3 backend emits one `HTHPlatformEvent` at a time. The engine handles
quit and window resize events, then passes events to Input. Input begins each
frame by clearing transient edges, mouse delta, and wheel movement while
preserving held keys and buttons. Focus loss clears held input to prevent
stuck state.

Physical keys and mouse buttons use engine-owned enums. Semantic actions and
bindings are separate future layers.

## Consequences

- SDL event, scancode, keycode, and mouse constants remain backend-private.
- Input logic can be tested using HTH events without SDL or human interaction.
- `down`, `pressed`, and `released` have explicit frame semantics.
- New native backends must translate into the same event vocabulary.
- Relative mouse mode and semantic gameplay actions remain deferred.
