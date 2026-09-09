# ADR-0059: Generalize Dead-Target Exclusion at the Enemy Boundary

- Status: Accepted
- Milestone: v0.3.35

## Decision

MULTI-PLAYER DEAD TARGET EXCLUSION FOUNDATION IS DISCONNECTED.

EXCLUSION REMAINS GENERIC `HTHEntityHandle` POLICY.

EXCLUSIONS ARE CALLER-OWNED POINTER PLUS COUNT.

TARGET SELECTION AND PURSUIT RUNTIME ACCEPT ZERO TO MANY EXCLUSIONS.

PURSUIT CLEARS CURRENT TARGET IF ANY EXCLUSION MATCHES.

MEMBERSHIP USES FULL GENERATION-SAFE HANDLE EQUALITY.

INVALID OR STALE KEYS, DUPLICATES, AND ORDER ARE INNOCUOUS.

DEATH REMAINS PLAYER TARGETING AUTHORITY; DEFEAT REMAINS ORTHOGONAL.

RANKING, PERCEPTION, LOS, TIES, AND NON-PLAYER ELIGIBILITY REMAIN FROZEN.

ALL EXCLUDED IS VALID NO-TARGET.

NO EXCLUSION STORE, HEAP ALLOCATION, ENGINE MULTI-PLAYER INTEGRATION, OR FAKE
SECOND PLAYER.

Selection checks exclusion membership before candidate Spatial work. Pursuit
checks its Current Target after cadence advance and before the Spatial
early-out, then forwards the same list to Selection. A null pointer with zero
count and a non-null pointer with zero count are valid; a null pointer with
positive count is a technical failure before mutation.

Bootstrap mechanically converts its one authoritative local Death snapshot to
zero exclusions while alive or a one-element stack list while dead. The Engine
continues to construct one candidate and does not use Player Runtime
Population.

## Consequences

The boundary can express independently snapshotted death for multiple gameplay
Players without making generic Enemy code query PlayerRoster,
PlayerLifecycleRuntime, Health, Death, Defeat, or Revive state. Exclusion
membership costs `O(N*E)` in Selection and `O(E)` for a Pursuit Current Target,
uses `O(1)` auxiliary memory, and allocates nothing.

## Rejected Alternatives

- A Player-specific adapter or roster lookup in Enemy AI would couple generic
  targeting to one role and lifecycle authority.
- Prefiltering candidates alone would fail to clear an already-persisted dead
  Current Target.
- An ExclusionStore, callback predicate, tag/layer framework, or copied list
  would add ownership and infrastructure beyond the bounded requirement.
- Index-only comparison would incorrectly exclude a replacement generation.
- Engine multi-Player composition or batched Death snapshots would prematurely
  integrate later cooperative runtime work.
