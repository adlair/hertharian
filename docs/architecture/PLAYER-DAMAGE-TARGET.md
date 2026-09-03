# Player Damage Target Identity Foundation

Hertharian v0.3.20 gives the local Player one generation-safe Entity identity
that is both the existing Enemy target and a valid target for the released
Actor-based Damage Intent contract.

```text
HTHPlayerBody
  physical authority: position, velocity, dimensions, movement state
        |
        | body-center synchronization
        v
Player Target Bridge target_entity
  Entity + Actor + Spatial + Health
```

The same handle carries all four associations. There is no second Player
Entity and no proxy-to-damage mapping. Enemy and DynamicBody are deliberately
absent. Adding Actor marks gameplay participation only; Enemy behavior still
requires EnemyStore, generic dynamic collision still requires DynamicBody,
and runtime visualization still requires DynamicBody.

## Authorities

`HTHPlayerBody` remains the sole movement and physical-state authority.
Spatial is only its synchronized body-center mirror and never writes movement
back. `HTHHealthStore` is the sole Player Health authority; neither PlayerBody
nor the Bridge caches current or maximum Health.

The bridge receives a caller-owned valid `HTHHealth` during Create. Production
bootstrap supplies `100/100` as temporary bootstrap tuning, not final game
balance, public configuration, or Level data.

## Lifecycle and Generations

Create delegates Entity + Actor + Spatial + Health construction and rollback
to Actor Spawn with DynamicBody disabled. Destroy delegates current optional
component removal and Entity destruction to Actor Despawn. The Bridge stores
only the handle and publishes or invalidates it after the corresponding
transaction succeeds.

Spatial Sync changes no Actor or Health state. Damage changes no PlayerBody,
Spatial, Entity identity, Enemy Target relation, or attacker state. Health
zero remains a live Entity + Actor + Spatial + Health composition and has no
automatic death meaning.

Destroying the Entity makes old Damage Intents, Health accesses, and incoming
Enemy Target relations semantically stale. Recreating a Bridge uses a new
generation; an old relation cannot revive against it.

## Released Damage Composition

The composition is now structurally valid using unchanged foundations:

```text
Runtime Enemy Actor
  -> HTHDamageIntent { source=enemy, target=player, amount }
  -> hth_damage_intent_resolve
  -> HTHHealthStore[player]
```

v0.3.20 proves this identity boundary in tests. As of v0.3.21, Enemy Attack
Execution can explicitly build a valid DamageIntent targeting this same
handle, and tests can resolve it through existing Health. Production still
builds or resolves no DamageIntent and adds no cadence, death, or game-over
behavior.

As of v0.3.23, the production bootstrap Enemy uses that same handle as the
Decision and DamageIntent target. Ready ATTACK frames resolve caller-owned
10-point damage through Health; cadence prevents damage-per-frame. Health zero
continues to have no death, despawn, target-clear, or PlayerBody meaning.
