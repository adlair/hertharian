# Snapshot-Aware Player Revive Read-Side Foundation

Hertharian v0.3.40 adds private, disconnected snapshot-aware siblings for
Player Revive Eligibility, Target Selection, and Interaction. They consume the
released `HTHPlayerDeathSnapshot` without integrating Engine or changing
Revive Execution.

## One Policy, Two Acquisition Paths

Revive Eligibility has one private semantic policy authority. The legacy
wrapper acquires live reviver and target Death in its released deterministic
order. The snapshot wrapper resolves both current Roster identities, queries
the snapshot by PlayerSlot plus complete Entity handle, acquires live Defeat
and target ReviveWindow, and delegates to the same policy.

A current Roster identity absent from or mismatched in the supplied snapshot is
a technical population-coherence failure. It returns false with canonical
`eligible=false`; it is not ordinary ineligibility. Self, reviver dead or
defeated, target alive or defeated, and inactive Window remain valid semantic
rejections.

## Selection and Live Spatial

Legacy and snapshot-aware Target Selection share one implementation of
candidate iteration, inclusive range, three-dimensional squared distance,
nearest target, and exact-distance lower-PlayerSlot tie breaking. The snapshot
path calls snapshot-aware Eligibility and performs no live Death query.
Spatial, Defeat, and ReviveWindow remain current live state.

## Generation-Safe Interaction

An active Interaction attempt owns this exact identity:

```text
(reviver PlayerSlot, reviver complete EntityHandle,
 target PlayerSlot, target complete EntityHandle)
```

Changing any component starts a new attempt. Reusing the same PlayerSlot or
Entity index with a new generation never transfers hold progress. `{0}` remains
canonical inactive state; active state is a pointer-free, copyable value.

Legacy and snapshot-aware steps share one progress/reset/completion authority.
Every held step revalidates Eligibility. Snapshot-aware steps receive the
current frame snapshot but never retain it. A new snapshot with the same exact
participants and a dead target continues progress. Valid policy loss, input
release, range loss, Window expiry, or Defeat resets the attempt. A technical
snapshot mismatch preserves the complete pre-step state for diagnosis/retry.

Hold duration is captured at attempt start. Range remains current on every
step, while revive Health remains current on the completion step.

## Live Commit Authority

Snapshot-aware Interaction may temporarily retain a sampled dead fact after
ordinary healing or reviver death later in the same frame. Completion still
delegates to unchanged Revive Execution, which performs live Eligibility and
Death validation immediately before Health mutation. The next frame snapshot
then exposes the current state. No reconciliation is added to the read side.

Independent revivers may share one immutable snapshot but own separate attempt
states. The first successful Execution heals and cancels the target Window;
later attempts observe live ineligibility or are rejected at commit, so no
double healing occurs.

## Query Budget and Scope

For `P` current Players:

```text
FrameSnapshotAcquisitionQueries = P
SnapshotAwareReadSideAdditionalDeathQueries = 0
MutationCommitLiveDeathQueries = 2
```

Snapshot Eligibility and Interaction are O(1). Target Selection is O(P), with
`P <= HTH_MAX_PLAYERS == 4`. Auxiliary memory is O(1); heap allocation and
mutable global state are zero.

Engine has no caller, production still has one Player, and per-frame
snapshot-aware revive work remains zero. Contextual Revive Runtime
Orchestration is deferred to v0.3.41.
