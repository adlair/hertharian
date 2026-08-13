# ADR-0011: Collision Trace Contract

- Status: Accepted
- Milestone: v0.1.8

## Context

The v0.1.7 axis-separated solver coupled player displacement directly to the
current static obstacle representation. It could provide basic blocking but
did not make impact time or surface normals first-class, limiting robust slide
and deliberate step movement and tying future collision evolution to Player.

## Decision

Player Movement consumes Collision through internal swept-volume trace queries
instead of resolving or iterating `CollisionWorld` obstacle storage. A trace
has explicit start/end and mins/maxs, and reports earliest impact fraction,
end position, surface normal, `start_solid`, and `all_solid`.

The current Collision backend performs swept AABB queries against static AABB
storage. Player Movement owns velocity clipping, temporary collision planes,
ground probes, and explicit step selection. Collision traces remain geometric
and player-agnostic.

## Consequences

- Player Movement is decoupled from backend geometry storage.
- Impact fraction and normal become reusable collision results.
- Multi-plane slide and deliberate step-up/step-down are possible without
  implicit horizontal elevation.
- A future BSP backend can implement equivalent query semantics without
  exposing BSP to Player Movement.
- Collision remains internal; no plugin/backend framework or public physics API
  is introduced.
- Initial penetration is reported but general depenetration remains absent.
