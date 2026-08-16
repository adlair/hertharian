# ADR-0022: Separate Dynamic Body State and Static Collision Resolution

- Status: Accepted
- Milestone: v0.2.9

## Context

Entity Foundation provides identity and lifetime, while Spatial provides an
optional world-space position and yaw. Runtime physical objects need shape and
velocity plus safe movement against the existing static CollisionWorld without
duplicating position or turning Entity/Spatial into a rigid-body framework.

## Decision

Store finite AABB half-extents and finite linear velocity in an independent
Dynamic Body Store keyed by Entity index and generation. Spatial remains the
only position owner and its position is the Body center. Body association
lifetime follows the live Entity, not temporary Spatial attachment: initial
attach requires Spatial, removing Spatial keeps the Body, and reattaching
Spatial to the same Entity makes that Body simulatable again.

Keep movement resolution in a separate Dynamic Collision operation. It reads
the current Spatial position and Body state, reuses CollisionWorld swept AABB
traces, removes blocked normal velocity, and writes results through the owning
Store APIs. It resolves one Body against static collision only and is not run
automatically by Engine.

## Consequences

Entity identity, Spatial state, and Dynamic Body state remain distinct. A
destroyed Entity immediately hides stale Body storage, generation reuse cannot
inherit it, and Body relocation never creates a second position authority.
High-speed static collision uses the established swept trace rather than a
discrete overlap test.

This decision does not establish a future full rigid-body architecture. It
adds no automatic gravity, grounded state, mass, forces, friction,
restitution, angular motion, depenetration, step climbing,
Dynamic-vs-Dynamic collision, broadphase, rendering, gameplay, Player
migration, Level declarations, persistence, ECS, or physics scheduler.
