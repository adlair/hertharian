# ADR-0021: Separate Optional Spatial State from Entity Identity

- Status: Accepted
- Milestone: v0.2.8

## Context

Entity Foundation supplies stable runtime identity and lifetime but deliberately
stores no state. Future dynamic objects need optional world-space placement
without expanding Entity handles, coupling identity to storage addresses, or
prematurely introducing physics, rendering, gameplay, or a generic component
architecture.

## Decision

Store optional world-space position and yaw in an independent internal Spatial
Store indexed by Entity handle index. Each present entry records the owning
generation. Every operation receives the Entity Registry explicitly and
requires the Registry to report the handle alive; the Store does not retain or
own the Registry and Entity remains ignorant of Spatial.

Transforms contain only finite `HTHVec3` position and finite, unnormalized yaw
in radians around world Y. Access is by copy. The Store begins at 64 entries,
grows safely by doubling, and iterates valid live associations in ascending
index order.

## Consequences

Identity remains small and stable, Entities may exist without Spatial state,
and Store relocation cannot invalidate borrowed transform pointers because
none are exposed. Destroyed-Entity entries may remain physically stored but
are inaccessible; generation validation prevents a reused index from
inheriting them, and explicit attach replaces stale storage without callbacks
or garbage collection.

Spatial remains runtime-only state, not persistent identity. This decision
adds no ECS commitment, component API, Player migration, Level syntax,
hierarchy, physics, collision, renderer integration, gameplay behavior,
thread-safety, or mutation-safe iteration.
