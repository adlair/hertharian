# ADR-0017: Material and Texture Boundary

- Status: Accepted
- Milestone: v0.2.4

## Context

Renderer previously mapped World diagnostic visual classes directly to
hardcoded colors. That coupled bootstrap content identity to backend appearance
and could not demonstrate external image decoding or GPU texture ownership.

## Decision

World visual classes are temporarily resolved outside Renderer to external
versioned material Resource IDs. Resource System supplies raw bytes; a
size-aware Material parser produces an owned base color and optional texture
Resource ID. Referenced image bytes are decoded independently into caller-owned
RGB8 image data, then graphical Renderer initialization uploads them to
backend-owned OpenGL textures.

Renderer consumes only resolved draw data and performs no Resource lookup,
format parsing, or filesystem I/O. Material and Image parsers perform no I/O
and expose no OpenGL types. Headless initialization validates the complete
material/image graph while skipping GPU work. Missing or malformed content has
no compiled palette or texture fallback.

## Consequences

Appearance changes are observed on the next process start without rebuilding.
`hthlevel` format version 1 and World remain unchanged, so the explicit
visual-class-to-material bridge is temporary. PPM P3, fixed per-face UVs, one
texture unit, nearest filtering, repeat wrapping, and no mipmaps are sufficient
for this bootstrap.

There is no generic asset or texture cache, PNG decoder, material registry,
PBR, lighting, transparency system, mesh abstraction, or direct Level material
reference. Those remain separate future decisions.
