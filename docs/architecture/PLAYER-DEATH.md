# Player Death Foundation

Hertharian v0.3.24 defines Player death as a pure query over the current
Health of the explicit Player gameplay identity supplied by the caller:

```text
PlayerDead(P) iff Health(P).current == 0.0
```

This definition applies only after validating that `P` is the current live
Entity generation and has both Actor and valid Health associations. A missing
Actor, missing Health, dead or stale Entity, invalid dependency, or null output
is a technical failure rather than an alive/dead classification.

## Identity and Authority

The intended handle is `HTHPlayerTargetBridge.target_entity`, whose released
composition is Entity + Actor + Spatial + Health. The caller identifies that
handle as the Player; the query does not scan Entity Registry or introduce a
Player role or Store. Health is the sole authority for life amount. PlayerBody
remains the independent authority for physical state and is neither passed nor
read.

Player Death depends only on Entity, Actor, and Health. It does not depend on
Spatial, PlayerBody, DamageIntent, EnemyTarget, movement, Input, Camera,
Renderer, Platform, or time. It allocates and mutates nothing.

## Internal Query Contract

```c
bool hth_player_death_is_dead(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHHealthStore *health,
    HTHEntityHandle player,
    bool *out_dead);
```

When an output pointer is supplied, the function writes `false` before any
validation. Failure returns `false` and leaves that canonical output. Success
returns `true` and writes whether valid current Health is exactly `0.0F`.
There is no epsilon: every valid finite positive Health value means alive.
Health with maximum zero is structurally invalid under the existing Health
contract and cannot be classified.

Health lookup inherits Entity-generation and Actor validation from
`HTHHealthStore`, so a stale handle cannot observe a replacement generation.
Repeated calls against unchanged state are deterministic. The query is O(1),
uses O(1) auxiliary memory, and performs zero allocations.

## Reversible Current-State Semantics

Death is derived each time and has no persistent bool, enum, latch, history,
timestamp, or event. Initial valid zero Health is immediately dead; exact
lethal damage and overkill become dead through Health's existing saturation.
Further damage at zero leaves the query true. Healing from zero to any positive
Health makes the query false again. This is current-state semantics, not a
revive system.

`HTHDamageResult.became_zero` remains a result of one damage operation and is
not Player Death authority: initial Health may already be zero and later
healing may restore positive Health.

## Runtime Integration

v0.3.24 has no production caller and adds no per-frame work. As of v0.3.25,
Engine queries this foundation once before movement intent construction. Death
suppresses voluntary movement and jump by selecting the released disabled
intent, while the complete Player Movement step continues gravity, friction,
momentum, collision, grounding, and landing.

A dead Player target remains a live Entity with its Actor, Spatial, Health,
stable handle, and incoming EnemyTarget relations intact. Input, mouse-look,
Camera, View Dynamics, Bridge sync, rendering, and Enemy attack behavior remain
active. Healing above zero restores normal control through the next applicable
query. See `PLAYER-DEATH-RUNTIME-INTEGRATION.md` and ADR-0049. Target filtering,
game over, respawn, revive, Downed, corpse, animation, audio, HUD, persistence,
and multiplayer death remain deferred.

As of v0.3.26, the same once-per-frame result also drives the Player-specific
targeting policy described in `DEAD-PLAYER-TARGETING-POLICY.md`. The Player
identity persists, but matching EnemyTarget relations are cleared and the
handle is excluded from acquisition beginning with the first dead snapshot.
