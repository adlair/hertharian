# Entity Identity and Lifetime Foundation

Hertharian v0.2.7 defines an Entity narrowly as a live generational handle in
an Engine-owned `HTHEntityRegistry`. This is runtime identity and lifetime
bookkeeping only. It is not an object pointer, gameplay actor, component
container, or static World object.

## Handles and Registry Scope

An `HTHEntityHandle` is the value pair `(index, generation)`. Equality compares
both fields. The canonical invalid handle is `(UINT32_MAX, 0)`;
`UINT32_MAX` is never an allocatable index and generation zero is never live.
A handle is alive only when its index is in range, its generation is nonzero,
and the referenced Registry slot is alive with exactly the same generation.

Handles are meaningful only relative to the Registry that issued them.
Numerically equal handles from different Registries do not establish shared
identity. Handles also are not persistent content, savegame, or network IDs;
any such identity will require a separate future concept.

## Creation, Destruction, and Reuse

A Registry starts empty with 64 free slots at generation 1. Fresh creation is
deterministic and initially assigns indices `0`, `1`, `2`, and so on. Creation
pops a free slot in constant time, marks it alive, increments `live_count`, and
returns its current generation. If no free slot exists, storage doubles from
64 to 128 to 256 and onward, bounded by the representable `uint32_t` index
space, safe `size_t` allocation arithmetic, and available memory. Growth may
relocate internal storage, but index/generation handles remain stable.

Destroying a matching live handle decrements `live_count`, increments the
slot generation, and pushes the slot onto the LIFO free list. Reuse therefore
produces a different handle. Old handles remain stale, and a stale destroy
cannot destroy the newer occupant. Invalid, out-of-range, generation-zero,
wrong-generation, stale, and already-destroyed handles are rejected without
Registry mutation.

Generation never wraps. Destroying a live slot at generation `UINT32_MAX`
retires that slot permanently instead of producing generation zero. Retired
slots remain part of allocated capacity but are neither live nor reusable and
never enter the free list. Retirement prevents a sufficiently old stale handle
from becoming valid again through `uint32_t` wrap. Registry destruction
releases its slot array even if live Entities remain; handles own no memory.

Creation is O(1) when a free slot exists, destruction, `is_alive`, and
`live_count` are O(1), and growth is occasional O(capacity). Allocation occurs
only for the Registry and slot-array growth—never once per Entity.

## Iteration

The small value iterator scans live slots in ascending index order. It yields
only currently live handles and writes the invalid handle when exhausted or
when valid output storage is supplied to a failing call. Traversal is
O(capacity), owns no Registry memory, and can be restarted explicitly.
Creating or destroying Entities while an iteration is active is unsupported;
the iterator provides no mutation-safe traversal guarantee.

## Engine and World Boundary

Each Engine instance owns exactly one private Registry, initialized after the
selected Level has produced its finalized static World and destroyed before
that World during shutdown. The ownership container co-locates those states,
but their models remain separate:

```text
Engine
├── immutable static HTHWorld objects
└── runtime HTHEntityRegistry identities
```

Level v2 declares no Entities, and neither `bootstrap` nor `selection_test`
creates any. Production therefore begins and remains at zero live Entities in
v0.2.7; tests are the first consumer of creation. The existing Player is not
an Entity and no index is reserved for it.

The Registry stores no Transform, position, physics, collision, rendering,
material, resource, gameplay type, name, behavior, component, or relationship
state. There is no ECS decision, component model, Actor hierarchy, Entity
update/render/collision pass, serialization, or public Entity API in this
milestone.

Spatial, Dynamic Body, and Actor state reference an Entity handle while
remaining separate from identity rather than being embedded in the handle or
Registry slot. Spatial owns optional world-space position and yaw; Dynamic
Body owns optional physical shape and velocity; as of v0.3.0, the presence-only
Actor association identifies gameplay participation. Each validates handles
against the Entity Registry without changing Entity identity or lifetime.
