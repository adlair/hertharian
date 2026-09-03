# Enemy Attack Execution / DamageIntent Foundation

Hertharian v0.3.21 introduces the internal builder that materializes one
explicit Enemy attack request as exactly one `HTHDamageIntent`. It is a
builder-only boundary: it validates the Enemy-specific source, delegates the
released Actor-to-Actor DamageIntent contract, and publishes a value. It does
not resolve or execute that value automatically.

```c
bool hth_enemy_attack_build_damage_intent(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHEntityHandle enemy,
    HTHEntityHandle target,
    float damage,
    HTHDamageIntent *out_intent);
```

## Contract

The source must be the current generation of a live Entity with Actor and
Enemy associations. The target must be the current generation of a live
Actor. A Player damage-target Actor, another Enemy Actor, and a generic Actor
are equally valid. Self-targeting remains valid because the released
DamageIntent contract permits it. A live non-Actor Entity is rejected.

`damage` is caller-owned and copied exactly. Zero and finite nonnegative
values are accepted; negative and non-finite values are rejected. The builder
constructs a local candidate and delegates final validation to
`hth_damage_intent_is_valid()`, rather than duplicating that foundation's
amount or target rules.

When output storage is supplied, every call first writes the canonical value:

```text
source = invalid Entity handle
target = invalid Entity handle
amount = 0
```

`false` means the arguments, Enemy source, target Actor, or amount did not
satisfy the contract, and the output remains canonical. `true` means exactly
one complete candidate was published. One call creates one value; there is no
queue, implicit retry, retained request, or persistent attack state.

## Deliberate Boundaries

Attack Execution has no HealthStore input and does not inspect or mutate
Health. Source Health is irrelevant, including zero Health. A target without
Health is valid and produces an Intent; if a caller later explicitly passes
that value to `hth_damage_intent_resolve()`, the released no-Health result is
successful processing with `applied=false`.

The builder has no Spatial, DynamicBody, Collision, LOS, Perception, Attack
Eligibility, Decision, or EnemyTargetStore dependency. It performs no range,
geometry, facing, policy, target-selection, or movement work. It mutates no
Registry or Store, allocates nothing, owns no state, and runs in O(1) time and
O(1) auxiliary memory.

Downstream composition remains explicit:

```text
Enemy + Actor target + damage
    -> hth_enemy_attack_build_damage_intent
    -> HTHDamageIntent
    -> nothing automatically

optional explicit caller/test
    -> hth_damage_intent_resolve
    -> target Health when present
```

Tests demonstrate Player-target resolution, damage to another Enemy, and the
released no-Health behavior for a generic Actor. Those demonstrations do not
make resolution part of this module.

## Runtime Status and Roadmap

The builder remained deliberately disconnected through v0.3.22. As of
v0.3.23, Pursuit Runtime uses the separate caller-owned
`ENEMY-ATTACK-CADENCE.md` primitive to gate explicit builder attempts. There is
no damage-per-frame, windup, recovery, or builder-owned rate-of-fire policy.

```text
v0.3.20  Player Damage Target Identity          RELEASED
v0.3.21  Enemy Attack Execution / DamageIntent  RELEASED
v0.3.22  Enemy Attack Cadence Foundation        RELEASED
v0.3.23  Enemy Attack Runtime Integration       CURRENT
```

As of v0.3.23, production Pursuit Runtime calls this unchanged builder only for
a ready `ATTACK`, commits cadence, then immediately resolves the stack-local
DamageIntent. Builder failure is technical and does not consume cadence; a
non-Actor ATTACK target therefore fails runtime rather than becoming a silent
no-op. See `ENEMY-ATTACK-RUNTIME-INTEGRATION.md`.
