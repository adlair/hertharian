# ADR-0001: Upstream Source Trees Are Immutable

## Status

Accepted

## Context

The project uses the original Quake III Arena source release and ioquake3
as architectural and implementation references.

Direct development inside these repositories would make it difficult to
distinguish upstream code from project-specific development.

## Decision

All repositories stored under `upstream/` are immutable reference trees.

Project development occurs exclusively outside `upstream/`.

The main architectural layers are:

- `engine/` — project engine implementation.
- `game/` — game-specific behavior and systems.
- `maps/` — level representations and generated maps.
- `assets/` — original project assets.
- `tools/` — development and content pipeline tools.

## Consequences

- Upstream history remains pristine.
- Project-specific changes are clearly identifiable.
- Upstream provenance can be audited.
- Licensing boundaries are easier to understand.
- Updating or comparing against future upstream revisions is simpler.
