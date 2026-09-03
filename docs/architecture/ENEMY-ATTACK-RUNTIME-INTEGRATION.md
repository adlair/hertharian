# Enemy Attack Runtime Integration

Hertharian v0.3.23 is the first release in which production Enemy `ATTACK`
intent changes target Health. It composes the released Decision, Attack
Cadence, Attack Execution, DamageIntent, and Health foundations without
duplicating their policy or arithmetic.

## Ownership and Runtime Composition

`HTHEnemyAttackCadenceStore` is a private, direct-index Store. Each attached
Runtime Enemy generation owns exactly one `HTHEnemyAttackCadence`; the Store
holds only generation, presence, and that cadence payload. Attach requires the
current Entity + Actor + Enemy composition and resets remaining time to zero,
so a new Runtime Enemy is immediately ready. Duplicate attach fails.

Every lookup validates the current generation in O(1). A stale handle cannot
observe or mutate a replacement generation, despawn removes the association
before the Enemy marker disappears, and a reused Entity index receives fresh
ready state only after explicit attach. Mutable pointers are used only within
the current synchronous runtime call.

Canonical Runtime Enemy composition is now:

```text
Entity + Actor + Enemy + Spatial + DynamicBody + Health + AttackCadence
```

Runtime Population includes cadence attach in its spawn transaction and rolls
back the complete new Actor/Enemy if it fails. Despawn requires and removes the
cadence association before removing Enemy and the remaining Actor composition.
A missing cadence on a Runtime Enemy is a technical structural failure; the
frame loop never lazily allocates or repairs one.

The Engine owns one cadence Store alongside its other private gameplay Stores.
It initializes that Store before bootstrap Enemy creation, despawns the Enemy
while the Store remains alive, then destroys the Store. Reinitialization starts
with fresh state. `HTHEnemy` remains a payload-free role marker, and bootstrap
state still stores only the Player Target Bridge and Enemy handle.

## Runtime Order

The historical `hth_enemy_pursuit_runtime_step()` name is retained to minimize
churn, but its dispatch responsibility now includes attack execution. Global
dependencies and finite nonnegative perception radius, attack range, chase
speed, damage, interval, and effective simulation delta are validated before
Enemy iteration. Global validation failure mutates nothing.

For every Enemy in deterministic ascending Store order:

```text
generation-safe cadence lookup
cadence advance exactly once with effective double simulation delta
existing Spatial/Target Selection checks
Decision at most once
  IDLE   -> no movement, no attack
  PURSUE -> existing Seek -> Chase -> Dynamic Collision
  ATTACK -> readiness -> build DamageIntent -> commit cadence -> resolve
```

Cadence therefore advances while no target exists, while policy produces
`IDLE`, during `PURSUE`, and before an `ATTACK` readiness query. Missing
Spatial retains its historical skip after cadence advances. Target changes,
target destruction, LOS loss, and perception loss neither reset nor pause the
Enemy-owned cooldown.

The Engine passes the same already-clamped simulation delta used by Player
movement. Cadence consumes it as `double`; only the released Chase API receives
the necessary `float` conversion. Cadence never reads wall-clock time.

## ATTACK Transaction

An `ATTACK` frame never calls Seek, Chase, Dynamic Collision, or writes Enemy
Spatial/velocity. If cadence is not ready, it is a successful no-op. If ready,
runtime builds at most one stack-local DamageIntent with the Decision's exact
target and caller-owned damage, commits the caller-owned interval, and resolves
the intent immediately through the existing Health authority.

Builder failure does not commit or resolve and returns technical failure.
Commit precedes resolution so Health can never change while cadence remains
unconsumed. A successful resolution with `applied=false` (for a valid Actor
without Health) is a successful attack and consumes cadence. A technical
resolution failure returns false without refund: the logical attack was
already emitted. No Health rollback or direct Health arithmetic is added.

Current Selection and Decision legitimately permit a live Spatial non-Actor
target. If such a target reaches `ATTACK`, the released builder rejects it;
v0.3.23 treats this as a technical runtime failure, with no cadence commit and
no Health mutation. There is no Player special case.

Zero damage and zero interval are valid. Even with interval zero, overshoot, or
a large valid delta, one Enemy can emit at most one attack per runtime step.
There is no readiness loop, catch-up balance, queue, retry, windup, recovery,
or target-specific cooldown. Multiple Enemies own independent cadence and may
each attack the same target once in deterministic Enemy order. On a later
Enemy failure, already committed earlier-Enemy effects remain and later
Enemies are untouched.

## Bootstrap Policy and Boundaries

Bootstrap owns temporary caller policy:

```text
attack damage   = 10.0
attack interval = 1.0 second
Player Health   = 100 / 100
```

The first eligible attack can immediately produce `100 -> 90`; pre-expiry
steps do not attack, and the next allowed attack produces `90 -> 80`.
Cooldown keeps advancing in pursuit or idle states. Reaching Health zero only
saturates Health: it does not despawn, clear targeting, disable movement, or
introduce death/game-over behavior. Headless and graphical simulation execute
the same gameplay path; no HUD, animation, sound, or other visible feedback is
added.

The attack path performs zero heap allocations: cadence lookup, readiness,
DamageIntent construction, commit, and resolution are O(1). Store allocation
occurs only at creation/attach capacity growth. Overall runtime remains
`O(C + E*(M*N + N))` for Enemy Store capacity `C`, Enemies `E`, candidates
`M`, and static obstacles `N`, with only O(E) constant cadence/attack work
added. No public API, asset, `hthlevel 2`, or `hthmaterial 1` change is part of
v0.3.23.
