# ADR-0002: Engine and Game Are Separate Architectural Layers

## Status

Accepted

## Decision

Engine technology and game-specific behavior are maintained as separate
architectural layers.

The engine provides reusable systems such as:

- platform abstraction
- renderer
- world representation
- BSP support
- collision
- filesystem
- input
- audio
- networking
- client/server infrastructure

The game layer provides:

- players and character classes
- weapons
- monsters
- AI
- generators
- pickups
- doors and keys
- level progression
- campaign logic
- game rules

## Goal

The engine must not become inseparably coupled to the first game built
with it.
