# ADR-0013: Separate Physical Eye from View Dynamics

- Status: Accepted
- Milestone: v0.2.0

## Context

Through v0.1.9 Camera position followed the resolved physical eye directly.
This made steps physically correct but visually abrupt and left no explicit
owner for subtle landing, walking, or speed presentation. Implementing those
effects in PlayerBody, Locomotion, Collision, or Renderer would mix visual
experience with physical truth or backend rendering.

## Decision

Hertharian separates the physical eye/camera anchor from transient visual View
Dynamics.

- PlayerBody and its physical eye remain authoritative.
- View Dynamics consumes observations rather than whole physical subsystems.
- Step, landing, bob, and FOV offsets are visual and cannot modify physics.
- Player Movement supplies a temporary landing result with pre-resolution
  downward speed.
- Camera position and FOV are recomposed from physical/base sources each frame.
- Mouse orientation remains immediate and receives no dynamic filtering.
- Renderer consumes final Camera state and knows nothing about View Dynamics.

## Consequences

Physical locomotion and collision results remain unchanged while view motion
can be tuned and tested independently without a GPU. Independent per-instance
configuration can later support accessibility reductions and character view
profiles. The milestone does not add a general spring camera, camera shake,
orientation dynamics, settings UI, character system, or gameplay landing
event.
