# ADR-0036: Enemy Pursuit Runtime Loop

- Status: Accepted
- Milestone: v0.3.12

## Context

Target Selection, explicit EnemyTarget, Decision, Seek, and Chase are released
independent capabilities. A reusable boundary is needed to invoke them in the
approved order over current Enemies without moving their responsibilities or
inventing production population and scheduling that do not yet exist.

## Decision

Compose the released capabilities through an explicit caller-driven pursuit
step using the deterministic Enemy iterator. Preserve every semantically valid
Target and invoke Selection only when none exists. Candidate arrays, radius,
speed, delta time, Stores, and invocation frequency remain caller-owned.

Decision independently evaluates every current Target, Seek alone derives
direction, and Chase alone applies movement. Missing Spatial skips an Enemy;
missing DynamicBody skips only Chase. Global validation is mutation-free, while
a delegated technical failure stops iteration with already committed earlier
Enemy effects preserved.

## Rejected Alternatives

- Selecting the nearest or best Target every step would cause target thrashing.
- Clearing Targets on lost LOS or range would replace persistence with hidden
  memory policy.
- Scanning all Entities for candidates would violate explicit caller ownership.
- An Engine-owned AI manager, EnemyBrain Store, generic FSM, Behavior Tree,
  GOAP, or Utility AI would add state and architecture beyond orchestration.
- A scheduler, think queue, fixed AI rate, or automatic frame-loop integration
  would introduce production execution prematurely.
- Player migration or a Player proxy/bridge belongs to a later capability.
- Level actor population belongs to the next Runtime Population milestone.
- Pathfinding, navigation, facing, gravity, attack, and combat are unrelated
  policies and remain deferred.

## Consequences

The orchestrator is stateless, uses `O(1)` auxiliary storage, and performs no
direct heap allocation; delegated Stores preserve their own allocation
contracts. Worst-case composed cost is `O(E*M*N)`. Production behavior remains
unchanged because v0.3.12 adds no automatic caller.
