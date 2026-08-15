# Material Foundation

v0.2.4 moves bootstrap appearance from Renderer constants into external,
versioned material resources. A material describes how a surface is shaded; a
texture is optional image data consumed by that material. Neither concept is a
filesystem path, a World object, or a GPU object.

## Material Resource Format

Bootstrap materials use canonical Resource IDs under
`materials/bootstrap/` and strict text format version 1:

```text
hthmaterial 1
base_color r g b a
texture <resource-id|none>
```

The declarations and their order are mandatory. Components use finite,
locale-independent decimal floats in the inclusive range 0 through 1. The
texture value is either the reserved word `none` or a copied Resource ID
validated by the existing Resource contract. Spaces, tabs, LF, CRLF, bare CR,
and `#` comments are accepted. A BOM, embedded NUL, unsupported version,
unknown or duplicate field, malformed value, and trailing token are errors.

The alpha component is preserved and sent to the shader, but v0.2.4 provides
no blending or transparency guarantee. Bootstrap materials use alpha 1.

## Boundaries and Ownership

The size-aware parser consumes a byte pointer and size and produces an owned
`HTHMaterialDescription`. It performs no I/O and knows neither Resource System
nor Renderer/OpenGL. A texture Resource ID is copied, so the description
survives release of its source `HTHResourceData`.

The temporary bootstrap material layer eagerly maps every
`HTHWorldVisualClass` to exactly one material Resource ID, loads and parses all
materials, and decodes every referenced image. This bridge is internal and
contains IDs only, never colors. Engine resolves visible World objects into
renderer-ready bounds, base colors, and optional image views. Renderer sees no
Resource ID or visual-class mapping. The same resolved material can be applied
to BOX or WEDGE geometry; primitive selection does not change material format
1.

Headless initialization performs the same material loads, parses, texture
loads, and image decodes as graphical initialization; it skips only GPU upload
and drawing. Missing or malformed content fails initialization without the old
hardcoded color fallback. All work happens during initialization, with no
per-frame I/O, decoding, or allocation.

## Current Scope

There is no registry, cache, deduplication, refcounting, lazy loading, hot
reload, PBR, lighting, blending, sampler resource, or direct material field in
`hthlevel` v2. Duplicate texture upload is acceptable in this bootstrap.
Future Level formats may reference materials directly and future content
pipelines may replace the visual-class compatibility bridge.
