# Enemy Intent / Decision Foundation

Hertharian v0.3.9 introduces an internal, explicit representation of what an
Enemy currently wants to do. Hertharian v0.3.18 extends that representation
with `ATTACK`. `HTHEnemyIntent` is an ephemeral value with three states:
canonical `IDLE`, whose target is the canonical invalid Entity handle, and
`PURSUE` or `ATTACK`, whose target is the current semantically valid Enemy
Target used by Decision. Intent contains only its kind and generation-sensitive
Entity handle. It is neither retained nor stored; there is no Intent Store,
Decision Store, brain, manager, cache, lifecycle, or automatic update.

`hth_enemy_decision_evaluate()` is a synchronous, observationally pure query.
Its observer must be a live Entity with current Actor, Enemy, and Spatial
associations. Health, including zero current Health, Dynamic Body, Player,
locomotion, movement state, yaw, and facing are irrelevant. Structural or
argument failure returns false with canonical `IDLE`; a valid negative gameplay
result returns true with canonical `IDLE`; and a valid positive result returns
true with `PURSUE(current_target)`.

The historical function can still return only `IDLE` or `PURSUE`. The internal
`hth_enemy_decision_evaluate_with_attack()` accepts an additional caller-owned
`attack_range` and can return all three intent kinds. As of v0.3.19, Pursuit
Runtime is its single production caller; the Decision semantics remain
unchanged.

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

## Attack-Capable Policy

The new evaluator preserves Current Target as its only target authority. Both
`perception_radius` and `attack_range` are independent, finite, nonnegative,
caller-owned scalars; neither is clamped or derived from the other. Perception
is the outer awareness gate, so an attack range larger than the perception
radius cannot produce `ATTACK` outside perception.

```text
obtain current Target
self Target -> IDLE
Enemy Perception at perception_radius
Enemy Attack Eligibility at attack_range
    eligible -> ATTACK(current Target)
    ineligible -> Enemy LOS
        clear -> PURSUE(current Target)
        blocked -> IDLE
```

Attack Eligibility remains the authority for inclusive 3D attack range and
attack LOS. Because it returns no ineligibility reason, the blocked in-range
path can perform one LOS inside Eligibility and a second fallback LOS to
distinguish `PURSUE` from `IDLE`. The maximum is therefore two LOS evaluations,
an accepted constant factor. A technical Eligibility failure propagates as a
Decision failure with canonical `IDLE`; a successful ineligible result proceeds
to fallback LOS. After the perception gate, `ATTACK` has priority over
`PURSUE`.

A missing, dead, stale-generation, or non-Spatial Target produces canonical
`IDLE` under the existing negative-result policy. In the attack-capable API,
self Target also produces `IDLE`. Neither path clears or replaces the stored
relationship. Exact range boundaries remain inclusive and zero attack range
can accept distinct colocated Entities.

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

As of v0.3.10, a caller may pass the Target from `PURSUE(target)` to the
separate Enemy Seek query. Decision does not call Seek, and Seek does not
consume Intent or execute movement.

As of v0.3.11, caller-owned orchestration may pass Seek's direction to Enemy
Chase. Decision still neither calls nor executes Chase.

As of v0.3.12, Pursuit Runtime invokes Decision independently even immediately
after Selection; `IDLE` skips Seek and Chase without clearing the Target.

As of v0.3.19, production Pursuit consumes the attack-capable variant and uses
the returned kind to select its movement branch. `ATTACK` suppresses Pursuit
Runtime movement for that frame without changing the retained DynamicBody
velocity or Target. Decision itself remains observationally pure and does not
execute an attack, create DamageIntent, mutate Health, apply cooldown, or own
movement.
