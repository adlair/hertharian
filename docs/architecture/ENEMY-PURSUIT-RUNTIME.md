# Enemy Pursuit Runtime Loop Foundation

Hertharian v0.3.12 introduces a caller-driven orchestrator that composes the
released Enemy pursuit foundations over the current Enemy set. It adds no AI
policy: Target Selection chooses, EnemyTarget persists, Decision determines
`IDLE` or `PURSUE`, Seek derives direction, and Chase applies movement through
Dynamic Collision.

The internal `hth_enemy_pursuit_runtime_step()` API receives all existing
Entity, Actor, Enemy, Spatial, DynamicBody, EnemyTarget, and CollisionWorld
authorities, an explicit caller-owned candidate array, perception radius,
chase speed, and delta time. A null candidate pointer is valid only when the
count is zero. All dependencies and finite nonnegative scalars are validated
before Enemy iteration, so global validation failure produces no mutation.

```c
bool hth_enemy_pursuit_runtime_step(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    HTHEnemyTargetStore *targets,
    const HTHCollisionWorld *collision_world,
    const HTHEntityHandle *candidates,
    size_t candidate_count,
    float perception_radius,
    float chase_speed,
    float delta_seconds);
```

## Deterministic Orchestration

Runtime uses the existing Enemy iterator directly, preserving ascending Entity
index order without a Registry scan, temporary list, sorting, or retained
iterator state. For each Enemy it performs:

```text
require Spatial or skip
query current semantic Target
if missing: Target Selection over explicit candidates
query current Target again
if still missing: skip
Decision using current Target
if IDLE: skip
Seek using the PURSUE Target
if DynamicBody is absent: skip
Chase using caller speed and delta time
```

A valid Target is never reselected merely because a better candidate exists.
It remains persisted when Decision returns IDLE due to range or LOS and resumes
pursuit when conditions recover. A destroyed or stale Target becomes
semantically invisible; Selection may replace it, but Runtime never clears a
relation automatically when no replacement exists.

Selection retains ownership of candidate validation, self exclusion,
duplicates, ranking, Perception, LOS, and deterministic ties. Decision always
rechecks policy independently after Selection. Seek remains the sole steering
geometry authority, and Chase remains the sole movement application authority.
Runtime writes neither Spatial position nor Body velocity and does not call
Dynamic Collision directly.

An Enemy without Spatial is skipped. An Enemy without DynamicBody may still
acquire and preserve a Target, run Decision and Seek, then validly skip Chase;
attaching a Body later allows pursuit with the same Target. Zero speed, zero
delta time, and colocated zero direction are valid calls. Point LOS may be
clear while the swept Body is physically blocked; Dynamic Collision's body
volume result remains authoritative.

## Ownership, Failure, and Cost

The candidate array is caller-owned. Runtime does not allocate, copy, sort,
mutate, or retain it. The orchestrator performs no direct heap allocation and
uses `O(1)` auxiliary storage, while delegated existing Store operations retain
their own allocation and growth semantics.

The step is not globally transactional. Global precondition failure occurs
before iteration and causes no mutation. A delegated technical failure stops
at that deterministic Enemy; effects already committed for lower-index
Enemies remain, and later Enemies are untouched. Chase retains its own
per-Enemy rollback contract.

For `E` Enemies, `M` candidates, and `N` static obstacles, worst-case composed
work is `O(E*M*N)`: Selection dominates with `O(M*N)`, while Decision and Chase
inherit `O(N)` and Seek is `O(1)`. Existing Targets or empty candidate sets may
reduce actual work.

Runtime has no scheduler, manager, brain, timing state, navigation, facing,
gravity, attack, or combat policy. It neither migrates Player nor creates a
Player target bridge or Enemy population. Production contains zero Pursuit
Runtime calls and performs zero such work per frame in v0.3.12. As of v0.3.13,
caller-driven Enemy Runtime Population can supply canonical runtime Enemies;
candidate sourcing and production integration remain caller responsibilities.

As of v0.3.15, the production Engine invokes Runtime once per applicable
simulation frame with the synchronized Player proxy as its single stack-local
candidate. Engine ownership, ordering, shared physical delta, and cleanup are
documented in `ENEMY-PURSUIT-ENGINE-INTEGRATION.md`.

As of v0.3.17, Enemy Attack Eligibility is not part of this loop. Eligible
Enemies continue the existing pursuit behavior because attack execution and
its orchestration remain deferred.
