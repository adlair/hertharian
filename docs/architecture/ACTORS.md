# Gameplay Actor Foundation

Hertharian v0.3.0 represents gameplay participation as an optional Actor
association over a live runtime Entity. Actor answers only whether an Entity
participates in the gameplay domain. It is not a new identity, base class,
Player, Enemy, Pickup, Projectile, renderable, physical object, AI agent, or
container for other state.

The architectural boundary is:

```text
Entity identity != Actor association != Spatial state != Dynamic Body state
```

## Identity and Presence

Actor uses `HTHEntityHandle` directly and introduces no Actor handle, UUID, or
generation authority. For `H = (index, generation)`, `HasActor(H)` is true
exactly when the Entity Registry reports `H` alive, the index fits the Actor
Store, its entry is present, and the stored generation matches. Entity remains
the sole runtime identity and lifetime authority.

Actor is presence-only in v0.3.0. An entry contains only a generation and a
presence flag. There is no payload, kind/type enum, type string, name, health,
team, AI state, callback, transform, physics state, or render state.

Attach requires only a live Entity and rejects a duplicate current
association. It does not require or create Spatial or Dynamic Body state. A
live Entity may therefore be an Actor without Spatial or Body, while another
Entity may have Spatial and Body without being an Actor. Composition order is
otherwise determined by each independent Store's own contract.

Remove clears only the matching Actor association. It neither destroys the
Entity nor removes Spatial or Dynamic Body state. Conversely, removing
Spatial or Body leaves Actor unchanged. Dynamic Collision continues to depend
only on Entity, Spatial, Body, and the static Collision World; removing Actor
does not prevent a remaining Spatial+Body pair from simulating.

## Lifetime and Generations

Destroying an Entity makes its Actor semantically absent immediately even if
the raw Actor entry remains physically stale. No callback, observer, cascade,
garbage collector, or periodic cleanup is required. A later Entity that reuses
the index does not inherit the association because its generation differs.
Explicit attach replaces that stale entry, and operations using the old handle
cannot remove the newer Actor.

The Actor Store does not retain an Entity Registry pointer or encode Registry
identity. Callers must consistently pair a Store with the Registry whose
handles populated it; numerically equal handles from unrelated Registries do
not imply shared identity.

## Storage and Iteration

The Store begins with 64 deterministic absent entries and doubles safely to
128, 256, and onward when a referenced Entity index requires it. Growth checks
index representation and allocation multiplication, uses transactional
`realloc`, and initializes every new entry. Allocation occurs only for Store
creation or Store growth, never once per Actor. Attach, has, and remove are
amortized O(1); growth and iteration are O(capacity), and memory is
O(capacity).

The value iterator yields `HTHEntityHandle` values for present, live,
generation-matching Actors in ascending Entity index. It does not require
Spatial or Dynamic Body state, skips dead/stale entries without mutating them,
and performs no heap allocation. Entity or Actor mutation during iteration is
unsupported; there is no snapshot or mutation-safe cursor machinery.

## Engine and Deferred Scope

Each Engine owns one private empty Actor Store, initialized after Dynamic Body
and destroyed before it. Production creates zero Actors and performs no Actor
iteration, ticking, behavior, rendering, or physics work per frame. The
current Player remains outside Entity/Actor/Spatial/Body composition, and
Level v2 declares no Actors.

This foundation adds no Actor taxonomy, Enemy, Pickup, Projectile,
health/damage, faction/team, AI, behavior callbacks, rendering ownership,
automatic physics integration, spawn framework, Actor resource format, names,
persistent IDs, savegames, networking, scripting, cascade lifecycle, or ECS.
Those concerns remain deferred rather than being encoded into Actor presence.
