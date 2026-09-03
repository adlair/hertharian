# Enemy Foundation

Hertharian v0.3.4 represents Enemy as an internal, runtime-only gameplay role
associated with an existing Actor. Enemy answers only whether a live Actor has
that specialized role. Its identity is the existing `HTHEntityHandle`; there
is no Enemy handle, ID, object, class hierarchy, kind, type, or definition.

The invariant is:

```text
HasEnemy(E) implies Alive(E) and HasActor(E)
```

It does not imply Spatial, Dynamic Body, or Health. Actor Spawn describes an
Entity's initial runtime composition, while Enemy independently classifies the
gameplay role of an already-created Actor. A zeroed `HTHActorSpawnSpec` can
therefore create an Actor-only Entity that is subsequently marked Enemy.

## Store and Lifetime

The opaque Enemy Store is a dedicated presence-only Store. Its dynamic array
is indexed by Entity index, and each entry contains only the Entity generation
and a presence flag. Entity Registry remains the sole generation and lifetime
authority. The Store retains no Registry, Actor Store, Entity pointer, or
component pointer; correct Store/Registry pairing remains a caller contract.

Attach requires valid Enemy, Entity, and Actor Stores plus a live, same-
generation Actor. Duplicate attach fails without mutation. Attach neither
requires nor creates Spatial, Dynamic Body, or Health. An explicit attach for
a current generation may replace a stale entry left at a reused Entity index.

`HasEnemy` requires the Entity to be alive, the stored generation to match,
and Actor to remain present for that generation. Removing Actor therefore
hides the retained Enemy association without cascading. Reattaching Actor to
the same live generation reveals it again. Destroying Entity hides the stale
entry immediately, and a replacement generation never inherits Enemy without
an explicit attach.

Remove validates the exact live Entity generation but deliberately does not
require Actor. It can clean up a retained association after Actor removal.
It clears Enemy only: Entity, Actor, Spatial, Dynamic Body, and Health remain
unchanged. Conversely, removing any optional component does not affect Enemy,
and Health at zero remains compatible with Enemy. No lifecycle cascade or
death processing is implied.

## Storage and Iteration

The Store starts with 64 absent entries and grows deterministically by doubling
to 128, 256, and onward as required. Growth guards Entity index range,
capacity progression, multiplication, and allocation failure while preserving
existing associations and initializing new entries. Allocation occurs only at
Store creation and growth. Attach is amortized O(1), has and remove are O(1),
and iteration is O(capacity).

The value iterator yields `HTHEntityHandle` copies in ascending Entity index.
It filters absent entries, stale generations, dead Entities, and entries whose
Actor is currently absent. Spatial, Dynamic Body, and Health are irrelevant to
iteration. Mutation of Entity, Actor, or Enemy state during iteration is
unsupported; the iterator is neither a snapshot nor mutation-safe.

## Engine Ownership and Deferred Scope

Each Engine owns one private empty Enemy Store. It is initialized after Actor
and destroyed before Actor. Production creates zero Enemy associations and
performs no Enemy iteration, update, logging, AI, movement, targeting, attack,
damage, death, rendering, or other per-frame work in v0.3.4.

Deferred scope includes canonical Enemy composition, kinds or species,
definitions or prefabs, spawn helpers, AI and behavior state, perception
beyond the v0.3.5 radius query, target selection, locomotion, combat and damage
generation, death and drops, teams or factions, Level declarations, resources,
rendering, persistence, networking, scripting, and generic ECS machinery.

As of v0.3.5, explicit Enemy target relationships and pure radius-based
spatial perception are separate foundations documented in
`ENEMY-TARGETS.md`, `ENEMY-PERCEPTION.md`, and ADR-0028. They do not change the
presence-only Enemy contract described here.

As of v0.3.9, explicit Enemy Decision requires this presence together with
Actor and Spatial, but still adds no state or behavior to the Enemy Store.

As of v0.3.10, Enemy Seek uses the same structural Enemy authority plus an
explicit Spatial Target to derive direction without adding Enemy state.

As of v0.3.11, Enemy Chase additionally requires DynamicBody and applies an
explicit desired direction through generic Dynamic Collision without adding
state to the Enemy Store.

As of v0.3.12, the caller-driven Pursuit Runtime iterates this Store in
deterministic Entity-index order and composes existing pursuit capabilities.

As of v0.3.13, Enemy Runtime Population provides a caller-driven canonical
Entity+Actor+Enemy+Spatial+DynamicBody+Health composition without changing the
flexible base Enemy association contract.

As of v0.3.15, the production Engine owns one private bootstrap Runtime Enemy
and composes Population with the Player Target Bridge and Pursuit Runtime once
per applicable simulation frame. The bootstrap integration and its current
limitations are documented in `ENEMY-PURSUIT-ENGINE-INTEGRATION.md`.

As of v0.3.16, Engine explicitly selects that same handle for a private,
read-only runtime-body presentation adapter. Enemy remains unaware of Renderer;
see `RUNTIME-BODY-VISUALIZATION.md` and ADR-0040.

As of v0.3.17, the separate `ENEMY-ATTACK-ELIGIBILITY.md` query may validate an
explicit Enemy through this Store before composing Perception and LOS. It adds
no Enemy state and has no production call site.

As of v0.3.18, `ENEMY-INTENT-DECISION.md` adds an `ATTACK` intent and a
attack-capable Decision variant that composes Perception, Attack Eligibility,
and fallback LOS.

As of v0.3.19, Pursuit Runtime consumes that attack-capable Decision. `ATTACK`
suppresses pursuit movement for the frame while preserving the Target and
stored velocity; no attack execution, damage, cooldown, or additional Enemy
state is introduced.
