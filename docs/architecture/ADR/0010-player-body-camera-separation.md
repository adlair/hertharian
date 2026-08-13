# ADR-0010: Separate Player Body from Camera

- Status: Accepted
- Milestone: v0.1.7

## Context

The v0.1.6 FPS controller provided useful free-camera navigation, but direct
WASD translation of `HTHCamera` made view state the apparent physical player.
Gravity, collision, grounding, and future gameplay or networking cannot have a
clear owner if the camera is also the collidable object.

## Decision

Hertharian Engine separates physical player state from view/camera state.

- Player Body owns physical position, velocity, dimensions, eye height, and
  grounded state.
- FPS Camera Controller owns view yaw/pitch and capture behavior.
- Input produces movement intent; Player Movement applies it to Player Body.
- Collision operates on Player Body, never Camera.
- After resolved physical movement, Camera follows the body through its eye
  offset while retaining independent orientation.

These systems remain internal during bootstrap; this decision does not create
a public player or gameplay entity API.

## Consequences

Camera is no longer Player, and renderer view state is downstream of resolved
physical state. A future gameplay model, simulation policy, or networking layer
can evolve without tying physics ownership to rendering. v0.1.7 gains only a
local variable-delta AABB movement foundation; jumping, Quake-style movement,
fixed ticks, BSP, and gameplay entities remain later decisions.
