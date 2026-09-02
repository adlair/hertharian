# Player Target Bridge Foundation

Hertharian v0.3.14 lets existing Entity-based Enemy targeting foundations
observe the local physical Player without migrating Player ownership. The
Player remains an `HTHPlayerBody`; a stable, caller-owned
`HTHPlayerTargetBridge` owns the handle of a separate target proxy. The proxy
is not the Player and has exactly this composition:

```text
Entity + Spatial
```

It has no Actor, Enemy, DynamicBody, Health, gameplay identity, collision
body, rendering state, or presentation state. `HTHPlayerTargetBridge` is an
internal value containing only `target_entity`. Its canonical inactive state
stores `hth_entity_handle_invalid()` and callers must establish that state
before Create.

## Physical Target Anchor

The proxy position is the center of the current Player body:

```text
(player.position.x,
 player.position.y + player.height * 0.5,
 player.position.z)
```

Player position is the feet/base origin, so this anchor is independent of eye
height, Camera, View Dynamics, velocity, and grounded state. Proxy yaw is
canonically `0.0`. The bridge validates the complete Player Body through its
existing authority, promotes every formula operand to `double`, rejects a
non-finite or non-`float`-representable result, and only then converts the
anchor to Spatial's `float` representation. It does not clamp or apply an
epsilon.

The bridge reads but never mutates `HTHPlayerBody`. It does not duplicate
Player physics, run collision, infer movement, or derive its anchor from
Camera/presentation state.

## Internal API and Ownership

```c
bool hth_player_target_bridge_create(
    HTHPlayerTargetBridge *bridge,
    HTHEntityRegistry *entities,
    HTHSpatialStore *spatial,
    const HTHPlayerBody *player);

bool hth_player_target_bridge_sync(
    HTHPlayerTargetBridge *bridge,
    const HTHEntityRegistry *entities,
    HTHSpatialStore *spatial,
    const HTHPlayerBody *player);

bool hth_player_target_bridge_get_target(
    const HTHPlayerTargetBridge *bridge,
    const HTHEntityRegistry *entities,
    const HTHSpatialStore *spatial,
    HTHEntityHandle *out_target);

bool hth_player_target_bridge_destroy(
    HTHPlayerTargetBridge *bridge,
    HTHEntityRegistry *entities,
    HTHSpatialStore *spatial);
```

The caller owns the Bridge value and supplies the Registry and Spatial Store;
the Bridge retains no Store pointers. Supplying the same corresponding
Registry/Spatial pair throughout its lifecycle is therefore a caller contract.
Create accepts only a canonical inactive
Bridge. A current or stale non-invalid handle fails without replacement or
mutation. It validates the body and anchor, creates an Entity, attaches its
Spatial, then publishes the handle. Failure after Entity creation destroys
that Entity, leaving the Bridge inactive and no live partial composition.
That rollback may consume an Entity generation, and delegated Registry or
Spatial growth can retain enlarged capacity; the transaction promises
semantic composition rollback, not allocator-state rollback.

Sync requires the exact live Entity generation and its current Spatial. It
validates the new body and anchor before replacing the transform, preserves
the stable Entity handle, and performs no allocation directly. Failure leaves
the Bridge and existing transform unchanged. Missing components, dead/stale
handles, or reused indices are errors: Sync neither repairs nor recreates the
proxy.

GetTarget canonicalizes its output to the invalid handle before validation and
returns the exact stable handle only while both Entity and Spatial are current.
It does not expose an unvalidated stale target.

Destroy requires the same exact current Entity + Spatial composition. It
removes Spatial before destroying the Entity and canonicalizes the Bridge only
after success. Inactive, incomplete, dead, or stale Bridges fail rather than
affecting a replacement generation. It does not scan or clear incoming Enemy
Target relationships. Once the proxy Entity dies, existing Target semantics
already make those exact-generation relationships absent; index reuse cannot
revive them.

## Composition with Enemy Foundations

Entity + Spatial is sufficient because existing systems already accept a live,
spatial non-Actor candidate or target:

- Target Selection can choose the proxy from an explicit candidate array.
- Perception measures its Spatial position.
- LOS traces to that position.
- Enemy Target stores the exact Entity generation.
- Decision validates perception and LOS for the stored handle.
- Seek derives direction to its Spatial position.
- Chase remains concerned only with applying Enemy movement.
- Pursuit Runtime can consume the proxy handle without a Player special case.

Moving the Player followed by Sync updates Spatial on the same Entity. An
existing Enemy Target relationship therefore remains valid while subsequent
Decision, Seek, and Pursuit work observes the new anchor.

All integration remains caller-driven in v0.3.14. Production creates zero
Player Target Bridges, performs zero Bridge Sync calls, supplies no Player
candidate to Pursuit Runtime, and performs zero additional per-frame work.
The intended future v0.3.15 order is documented, not implemented:

```text
Input
  -> Player Movement
  -> Player Target Bridge Sync
  -> Enemy Pursuit Runtime
  -> View / Renderer
```

## Cost and Deferred Scope

Create and Destroy are O(1) amortized through existing stores; Sync and
GetTarget are O(1). The Bridge itself performs zero direct `malloc`, `calloc`,
`realloc`, or `free` calls. Delegated Registry/Spatial operations retain their
documented creation and growth allocation behavior.

This foundation adds no Player migration, Player Actor, target manager,
Player singleton in Enemy AI, new Store, Camera target, physical-eye target,
per-frame proxy recreation, production orchestration, combat, networking,
persistence, or generic ECS bridge.
