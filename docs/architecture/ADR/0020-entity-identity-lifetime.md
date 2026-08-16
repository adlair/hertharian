# ADR-0020: Introduce Generational Runtime Entity Identity

- Status: Accepted
- Milestone: v0.2.7

## Context

Hertharian has immutable static World objects and a separately implemented
Player, but no safe identity/lifetime primitive for future dynamic runtime
objects. Pointer identity would couple identity to storage location, while a
reused plain index could make stale references alias a later object.

## Decision

Use an Engine-owned, internal Entity Registry that issues registry-scoped
handles containing a slot index and generation. Fresh generations start at
one. Destroying a live Entity increments its generation before LIFO free-list
reuse, so stale handles fail exact validation. Dynamic slot storage begins at
64 and doubles safely when all reusable slots are occupied; handles remain
stable if that storage relocates.

The invalid handle is `(UINT32_MAX, 0)`. Generation zero is never live, and a
slot destroyed at generation `UINT32_MAX` is retired permanently rather than
wrapping and potentially reviving a stale handle. The Registry owns all slot
storage, handles own no memory, and there
is no per-Entity allocation. Live iteration is deterministic in ascending
index order and does not support concurrent Registry mutation.

Static World objects remain distinct from runtime Entities. Engine creates one
empty Registry but no production Entity in this milestone. The current Player
remains separate, and Level format v2 is unchanged.

## Consequences

Stale handles are detectable, reusable slots do not revive old identities,
and handles survive Registry storage relocation without pointer identity.
Generation exhaustion has a defined no-wrap result, at the cost of permanently
losing an exhausted slot. Registry destruction can release all identity
storage even when live handles remain.

This decision establishes identity and lifetime only. It does not select an
ECS, inheritance, component architecture, Actor hierarchy, Transform model,
physics/render/gameplay integration, or persistent content identity. Those
decisions remain deferred, as do Player migration and Level-declared Entities.
