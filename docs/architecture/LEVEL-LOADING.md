# Level Loading Foundation

v0.2.3 connects the Resource and World boundaries without making either one
responsible for the other:

```text
levels/bootstrap.hthlevel
        ↓
Resource System → caller-owned raw bytes
        ↓
size-aware Level parser
        ↓
owned HTHLevelDescription
        ↓
Level-to-World build
        ↓
HTHWorld finalize
        ↓
Collision and Renderer
```

Resource System knows only the canonical Resource ID and raw bytes. The Level
parser receives only `const unsigned char *` plus a size; it has no filesystem
or Resource dependency. The builder receives a description and exercises the
existing World mutation/finalization API. World remains ignorant of storage,
resource IDs, parsing, and persistence.

## Ownership and Transactionality

Parsing converts token slices immediately into numeric values, flags, and
visual enums stored in a dynamically growing `HTHLevelDescription`. It retains
no pointer into `HTHResourceData`. Engine therefore releases the resource blob
as soon as parsing finishes, before World construction. After a successful
build, Engine destroys the description; the finalized World owns its own
object array and remains valid independently of both earlier objects.

Parse and build operations are transactional. A syntax, allocation, World
validation, or finalization failure releases partial storage and exposes no
half-built description or World. Diagnostics contain a one-based line and
column plus a bounded reason; Engine adds the canonical Resource ID when it
prints the failure.

## Engine Lifecycle

Engine creates Resources, synchronously loads
`levels/bootstrap.hthlevel`, parses and builds World, then continues the
existing Collision, Player, Platform, and Renderer initialization. Missing,
malformed, unsupported, or World-invalid content makes initialization fail
cleanly. There is deliberately no compiled-C fallback.

Headless mode follows the same Resource → Level → World → Collision/Player
path and disables only Renderer. The build-generated absolute development
Resource Root remains independent of the current working directory but is not
a binary-relocatable packaging solution.

As of v0.2.5, Level format v2 stores explicit collision and render shapes in
the intermediate description. The builder transfers both to World, whose
shape/flag validation remains authoritative. Historical format v1 is rejected
rather than upgraded.

## Current Limits

The loader is synchronous, single-threaded, read-only, and uncached. Level
description storage has no arbitrary small object cap, but the current
Collision backend still rejects more than 16 collidable objects rather than
truncating them. Version 2 supports only static objects with shared AABB
bounds, the existing flags and diagnostic visual classes, explicit AABB/none
collision, box/wedge/none render shapes, and one spawn.

There is no BSP, polygon geometry, entities, dynamic objects, materials,
textures, audio, multiple-level management, hot reload, serialization, map
compiler, or final install-root discovery. Editing the asset is observed on
the next process start without recompiling; it is not watched during runtime.
