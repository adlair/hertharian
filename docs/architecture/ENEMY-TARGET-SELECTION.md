# Enemy Target Selection Foundation

Hertharian v0.3.8 provides one explicit, synchronous gameplay policy operation
that selects at most one target for an Enemy from a caller-provided candidate
array. The array is `const`, remains caller-owned, is never retained, copied,
sorted, or modified, and may contain duplicates. Selection performs no global
Entity or World scan and has no production caller or per-frame work.

The observer must be a live Entity with current Actor, Enemy, and Spatial
associations. A candidate needs only a live exact Entity generation and a
Spatial association; it need not be an Actor, Enemy, Player, Health owner, or
Dynamic Body. Invalid, dead, stale, and Spatial-less candidates are skipped.
Self is excluded by Selection even though the underlying Perception and LOS
queries independently permit it. Health values, including zero, and Dynamic
Body presence are irrelevant. Enemy candidates remain eligible because no
faction policy exists.

## Eligibility and Ranking

For a structurally valid operation, candidate `C` is eligible exactly when:

```text
C != Enemy
AND EnemyPerception(Enemy, C, radius)
AND EnemyLOS(Enemy, C)
```

The caller-supplied radius must be finite and nonnegative. Selection first
performs cheap handle, Spatial, and self checks, then calls the released radius
Perception query. LOS is called only when Perception succeeds, preserving the
short-circuit and avoiding an unnecessary static-world trace. Selection does
not duplicate either query and never calls Collision directly.

All eligible candidates are ranked before any Target mutation. The minimum 3D
squared distance wins. Coordinate values are promoted to `double` before
subtraction, all ranking arithmetic uses `double`, and no square root is
computed. An exact equal-distance tie is resolved by lower Entity index; there
is no epsilon and generation is not a ranking key. Consequently list order is
irrelevant and duplicates are harmless.

## Results and Target Transactionality

`out_selected` is required and canonicalized to the invalid Entity handle
before validation. Null dependencies, a null array with positive count, an
invalid radius, or an invalid observer are operation failures: the function
returns false and preserves Target. A null array with zero count is a valid
empty set.

A valid operation with no eligible winner returns true, leaves the selected
output invalid, and preserves any existing Target. There is no automatic clear
when the current target is absent, invalid for this candidate set, occluded, or
outside the radius. Current Target receives no preference, lock, or hysteresis.

After complete ranking, a winner causes exactly one call to the released
EnemyTarget set API. Set failure returns false with invalid selected output;
success returns the exact complete winner handle. A better winner replaces the
current Target, while selecting the already-current winner is idempotent.

## Cost and Deferred Scope

For `M` supplied candidates, `K` candidates passing Perception, and `N` static
CollisionWorld obstacles, candidate filtering and ranking cost is
`O(M + K*N)`, worst-case `O(M*N)`, with `O(1)` auxiliary memory and no
candidate-list allocation. The final released EnemyTarget set operation may
grow its Store storage and therefore retains its existing allocation and
capacity-growth cost. Selection owns no Store, Engine state, cache,
initialization, shutdown, retained pointer, logging, or automatic invocation.

Global discovery, broadphase queries, Player preference, Actor-only policy,
factions, Health scoring, threat, aggro, randomness, sticky targets, target
lifecycle, memory, AI, chase, combat, Level declarations, persistence,
networking, scripting, and ECS remain deferred.

As of v0.3.9, Enemy Decision may consume the relationship produced by an
explicit Selection call. Decision never invokes Selection or receives its
candidate array, and Selection never produces or executes an Intent.
