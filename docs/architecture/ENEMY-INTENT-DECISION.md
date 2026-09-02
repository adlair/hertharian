# Enemy Intent / Decision Foundation

Hertharian v0.3.9 introduces an internal, explicit representation of what an
Enemy currently wants to do. `HTHEnemyIntent` is an ephemeral value with two
states: canonical `IDLE`, whose target is the canonical invalid Entity handle,
and `PURSUE`, whose target is the current semantically valid Enemy Target used
by Decision. Intent is neither retained nor stored; there is no Intent Store,
Decision Store, brain, manager, cache, lifecycle, or automatic update.

`hth_enemy_decision_evaluate()` is a synchronous, observationally pure query.
Its observer must be a live Entity with current Actor, Enemy, and Spatial
associations. Health, including zero current Health, Dynamic Body, Player,
locomotion, movement state, yaw, and facing are irrelevant. Structural or
argument failure returns false with canonical `IDLE`; a valid negative gameplay
result returns true with canonical `IDLE`; and a valid positive result returns
true with `PURSUE(current_target)`.

## Current Target Policy

Decision consumes only the current Enemy Target relationship. A missing,
dead, or stale-generation Target produces `IDLE`. Decision never clears,
replaces, acquires, or otherwise mutates that relationship; it accepts no
candidate array, scans no Registry or World, and never calls Target Selection.
Selection answers who should be targeted, while Decision answers what the
Enemy should currently want given its existing target.

The complete policy and evaluation order are:

```text
validate arguments and observer
obtain current Target
Enemy Perception at the caller-owned radius
Enemy LOS
PURSUE(current Target)
```

Every other valid path produces `IDLE`. Radius must be finite and nonnegative;
zero retains the existing Perception semantics. Perception precedes LOS so an
out-of-radius Target avoids a static CollisionWorld trace. Decision delegates
all radius geometry to Enemy Perception and all occlusion semantics to Enemy
LOS; it never calculates distance or calls Collision/Trace directly.

## Purity, Cost, and Deferred Execution

Decision mutates no Store or World, allocates nothing, retains no pointer, and
depends on neither time, frame number, randomness nor previous Intent.
Unchanged inputs therefore produce the same complete value. Evaluation is
`O(1)` when it stops before LOS and `O(N)` when LOS examines `N` static
CollisionWorld obstacles, with `O(1)` auxiliary memory.

Production has zero Decision calls and performs zero Decision work per frame
in v0.3.9. Intent execution, Seek, Chase, movement, velocity, facing,
navigation, attacks, DamageIntent generation, combat, death policy, memory,
FSMs, Behavior Trees, GOAP, Utility AI, persistence, networking, and scripting
remain deferred.
