# Player Target Bridge Foundation

Hertharian v0.3.14 introduced a stable Entity + Spatial proxy so existing
Entity-based Enemy targeting could observe the local `HTHPlayerBody` without
migrating Player movement ownership. Hertharian v0.3.20 evolves that same
generation-safe Entity into the Player's damage target identity:

```text
HTHPlayerBody (physical authority)
  -> Player Target Bridge target_entity
       -> Entity + Actor + Spatial + Health
```

The identity has no Enemy or DynamicBody association. It is still not the
Player's movement body: `HTHPlayerBody` remains authoritative for position,
velocity, dimensions, eye height, and grounded state. Spatial is a one-way
synchronized mirror, while `HTHHealthStore` is the sole Player Health
authority. The bridge stores only `target_entity`; it retains no component
copies, Store pointers, or Player pointer.

## Physical Target Anchor

The Spatial position remains the center of the current Player body:

```text
(player.position.x,
 player.position.y + player.height * 0.5,
 player.position.z)
```

Player position is the feet/base origin. The anchor is independent of eye
height, Camera, View Dynamics, velocity, grounded state, and Health. Yaw is
canonically `0.0`. Create and Sync validate the complete Player Body, promote
formula operands to `double`, and reject non-finite or non-`float`-
representable results before conversion. No clamp or epsilon is applied.

The bridge reads but never mutates `HTHPlayerBody`. It does not run movement,
collision, damage, healing, death behavior, or rendering.

## Internal API and Ownership

```c
bool hth_player_target_bridge_create(
    HTHPlayerTargetBridge *bridge,
    HTHEntityRegistry *entities,
    HTHActorStore *actors,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health,
    const HTHPlayerBody *player,
    HTHHealth initial_health);

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
    HTHActorStore *actors,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHHealthStore *health);
```

The caller owns the Bridge and supplies consistently paired Stores. Create
accepts only the canonical inactive state containing
`hth_entity_handle_invalid()`. It derives the Spatial anchor and delegates the
transaction to `hth_actor_spawn()` with Spatial and Health enabled and Body
disabled. The caller owns `initial_health`; the bridge defines no default.
Only a complete Entity + Actor + Spatial + Health composition is published.
Failure leaves the Bridge inactive; delegated rollback may consume an Entity
generation or retain grown Store capacity.

The DynamicBody Store is passed because the released Actor Spawn/Despawn API
requires the complete Store set. No DynamicBody is attached to the Player.

Sync retains its v0.3.14 boundary: it validates the exact live Entity and
Spatial, derives a new anchor from PlayerBody, and updates Spatial only. It
preserves the handle, generation, Actor, and Health. Damage or healing before
Sync remains unchanged afterward.

GetTarget similarly retains its targeting contract. It canonicalizes output
to the invalid handle and returns the stable handle while Entity and Spatial
are current. Actor and Health consumers validate their own component
requirements.

Destroy delegates to `hth_actor_despawn()`, whose order is Health, optional
DynamicBody, Spatial, Actor, then Entity. The Bridge becomes inactive only
after successful despawn. It does not scan incoming Enemy Target relations;
Entity generation invalidation makes them semantically absent, and index reuse
cannot retarget them to a replacement Player identity.

## Targeting and Damage Composition

Target Selection, Perception, LOS, Decision, Seek, and Pursuit continue to use
the same handle and Spatial anchor. Actor and Health add no ranking, geometry,
movement, rendering, or Enemy behavior. Enemy iteration is driven by
`HTHEnemyStore`, and Runtime Body Visualization requires DynamicBody, so the
Player identity enters neither path.

The additional associations allow the released damage contract to compose
without a mapping layer:

```text
Runtime Enemy Actor
  -> HTHDamageIntent(source=enemy, target=target_entity)
  -> Player Health in HTHHealthStore
```

Health reaching zero does not destroy the Entity, disable movement, or invoke
game-over behavior. v0.3.20 creates no production DamageIntent and applies no
damage; Enemy Attack Execution remains a later milestone.

As of v0.3.24, Player Death can read this explicit handle and derive dead from
current Health zero. As of v0.3.25, Engine performs that query before Player
Movement intent, while Bridge sync still runs afterward on the physically
resolved body. The Bridge owns no death state; its handle, generation, Entity,
Actor, Spatial, Health, and incoming target relations remain unchanged.

## Cost and Deferred Scope

Create and Destroy are O(1) amortized through Actor Spawn/Despawn; Sync and
GetTarget are O(1). The Bridge performs no direct heap allocation and owns no
Store. One independent Bridge value per Player remains compatible with future
multiplayer without adding singleton state.

Deferred scope includes Player movement migration, Player DynamicBody,
Player-specific Health APIs, dead-target policy, respawn, weapons, UI,
persistence, networking, and Level Health declarations.

As of v0.3.26, Bootstrap uses the same stable handle as a generation-safe
excluded target while the Player Death snapshot is true. The Bridge itself
remains alive, synchronized, and unaware of targetability.

As of v0.3.29, explicit Player Roster membership—not the Bridge alone—defines
the Player role. A future caller may register the Bridge's gameplay Entity;
the roster neither owns nor references the Bridge itself.
