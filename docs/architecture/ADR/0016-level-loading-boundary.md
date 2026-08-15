# ADR-0016: Level Loading Boundary and Versioned Text Format

- Status: Accepted
- Milestone: v0.2.3

## Context

The bootstrap World was compiled into C even after Resource System established
a storage boundary. Keeping persistent level content in production code would
require recompilation for every content edit and would conflate serialized
input with the finalized runtime representation.

## Decision

External level content is identified by a canonical Resource ID and loaded by
Resource System as raw bytes. A size-aware Level parser converts those bytes
into an owned intermediate `HTHLevelDescription`. A separate builder uses the
existing World API and World finalization to produce the runtime `HTHWorld`.

The first asset is `levels/bootstrap.hthlevel`, using strict text format
version 1. Resource does not interpret level syntax, Level performs no
filesystem I/O, and World knows neither Resources nor persistence. Engine
orchestrates ownership and provides no fallback to compiled geometry.

## Consequences

Bootstrap content can change between process runs without rebuilding the
engine. Parser tests operate directly on bounded byte arrays, while a content
integration test exercises Resource → Level → World and verifies historical
World values. Syntax and format versions can evolve separately from engine
versions, and future storage backends remain below Resources.

Format version 1 deliberately mirrors only the current static AABB World.
There is no Level cache, serializer, editor, materials, entities, BSP, map
compiler, runtime reload, or multiple-level system. A future authoring and
compiled-map pipeline may replace this minimal representation without moving
filesystem knowledge into typed loaders or World.
