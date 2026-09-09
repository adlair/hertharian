# Player Runtime Population Foundation

Hertharian v0.3.34 adds a private, disconnected composition boundary for
creating and destroying coop-ready gameplay Players. It does not integrate a
second production Player or add per-frame work.

## Player Composition

A gameplay Player created through this boundary is:

```text
Entity + Actor + Spatial + Health + Roster membership
```

Player identity does not require `HTHPlayerBody`, Input, or Camera. Population
does not attach `HTHDynamicBody`; the DynamicBody Store is passed only because
the released Actor Spawn/Despawn authority consumes the complete Store set.
The module has no Renderer, networking, Enemy, Collision, Revive Interaction,
Session, inventory, or progression dependency.

`HTHPlayerRuntimeSpawnSpec` is caller-owned and contains only the initial
Spatial transform and Health. It defines no product defaults. The foundation
is stateless: there is no PlayerStore, ownership array, duplicate slot-to-Entity
map, adopt API, or bulk reset. `HTHPlayerRoster`, owned by
`HTHPlayerLifecycleRuntime`, remains the sole PlayerSlot-to-Entity authority;
the lifecycle runtime remains the owner of Defeat, Revive Window, and
historical death-edge state.

## Spawn Transaction

Spawn canonicalizes the output slot to `HTH_PLAYER_SLOT_INVALID`, validates
its dependencies and specification, and delegates Entity creation plus Actor,
Spatial, and Health attachment to `hth_actor_spawn()`. Only after that complete
composition exists does it register the Entity through
`hth_player_lifecycle_runtime_register()`.

The PlayerSlot is published only after both phases succeed. If lifecycle
registration fails—for example because all `HTH_MAX_PLAYERS` slots are
occupied—the complete Actor composition is rolled back exactly once through
`hth_actor_despawn()`. No live Entity or component association is retained.
The released Actor authority may consume an Entity generation or retain Store
capacity during a failed transaction; those implementation effects are not a
published Player.

Capacity is exactly `HTH_MAX_PLAYERS`. Sparse roster slots and complete Entity
index-plus-generation handles retain their existing semantics. Registration
canonicalizes all lifecycle state for the selected slot, so reuse cannot
inherit Defeat, Revive Window, or `was_dead` state from an earlier Player.

## Despawn Transaction

Despawn accepts a PlayerSlot, resolves the authoritative current Entity through
the lifecycle runtime's roster accessor and `hth_player_roster_get_slot()`, and
prevalidates the required Actor, Spatial, and Health composition. Invalid,
empty, stale-generation, or incomplete membership fails before mutation.

After prevalidation it unregisters the slot through the lifecycle authority,
which clears membership and resets slot-bound state, then delegates component
removal and Entity destruction to `hth_actor_despawn()`. In the current
synchronous fixed-store model, the prevalidated teardown has no intervening
mutation point. Callers must use this operation only for Players created by
this population boundary; it is not a provenance-discovering generic Player
destructor.

## Bridge Coexistence

The Player Target Bridge remains the special local adapter between the
existing physical `HTHPlayerBody` and gameplay Entity systems. A Bridge Player
and generic population-created Players may coexist in one roster, but Bridge
creation/destruction continues through its own API. Population neither owns
nor synchronizes a Bridge and does not create a PlayerBody array.

## Cost and Deferred Integration

The module allocates no memory directly and owns no persistent state. Actor
Spawn/Despawn retain their amortized constant Store behavior; roster
registration is bounded `O(P)` for `P <= HTH_MAX_PLAYERS`, and slot resolution
and teardown are bounded `O(1)` for the frozen capacity.

Engine and Bootstrap make zero population calls in v0.3.34. Real cooperative
integration still requires a multi-Player authoritative Death-snapshot
architecture; current Engine carries one local boolean snapshot and one local
PlayerBody. Hertharian v0.3.35 supplies the disconnected generic Enemy
exclusion-list boundary and proves it with four population-created Players in
tests, but adds no production Population consumer. Revive Target Selection, a
semantic Interact action, and product-owned revive range, duration, and
restored-Health values remain prerequisites for a real coop runtime, not work
hidden in this foundation.
