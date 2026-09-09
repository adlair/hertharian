# ADR-0064: Snapshot-Aware Player Revive Read-Side

Status: Accepted

## Context

The v0.3.39 Player Death Snapshot provides one generation-safe Death sample per
current Player. Revive Eligibility, Target Selection, and Interaction still
perform repeated live Death acquisition, while Execution correctly requires
live validation before mutation.

## Decision

Hertharian v0.3.40 adds a disconnected snapshot-aware Revive read side.

- REVIVE ELIGIBILITY HAS ONE POLICY AUTHORITY.
- DEATH ACQUISITION IS SEPARATE FROM ELIGIBILITY POLICY.
- LEGACY ELIGIBILITY USES LIVE DEATH ACQUISITION.
- SNAPSHOT ELIGIBILITY USES `HTHPlayerDeathSnapshot`.
- SNAPSHOT ELIGIBILITY PERFORMS ZERO LIVE DEATH QUERIES.
- DEFEAT AND REVIVEWINDOW REMAIN LIVE.
- SNAPSHOT MISMATCH FOR A CURRENT IDENTITY IS TECHNICAL FAILURE.
- TARGET SELECTION RANKING REMAINS SINGLE-SOURCE.
- SNAPSHOT TARGET SELECTION PERFORMS ZERO LIVE DEATH QUERIES.
- INTERACTION REVALIDATES ELIGIBILITY ON EVERY HELD STEP.
- INTERACTION ATTEMPT IDENTITY IS PLAYER SLOTS PLUS COMPLETE ENTITY HANDLES.
- SLOT OR GENERATION REUSE NEVER INHERITS HOLD PROGRESS.
- SNAPSHOT INTERACTION PERFORMS ZERO LIVE DEATH QUERIES BEFORE COMMIT.
- REVIVE EXECUTION REMAINS UNCHANGED AND LIVE-AUTHORITATIVE.
- FRAME SNAPSHOT ACQUISITION QUERIES EQUAL CURRENT PLAYER COUNT `P`.
- SNAPSHOT-AWARE READ-SIDE ADDITIONAL DEATH QUERIES EQUAL ZERO.
- MUTATION COMMIT LIVE DEATH QUERIES REMAIN TWO.
- NO ENGINE INTEGRATION, HEAP, MUTABLE GLOBAL, OR PUBLIC API IS ADDED.

Legacy and snapshot-aware siblings share one implementation each for semantic
Eligibility, target ranking, and Interaction progress. Snapshot-aware APIs are
private. Defeat, ReviveWindow, Spatial, range, and completion configuration are
not snapshots.

## Consequences

The read side can share one fixed snapshot across independent revivers without
re-querying Player Death. Interaction persists exact participant identity, so
replacement Players cannot inherit hold progress. Technical snapshot
incoherence preserves state; semantic ineligibility resets it.

Same-frame Health changes can intentionally diverge from sampled Death.
Unchanged Revive Execution resolves this at the mutation boundary through live
Eligibility, preserving ordinary-healing safety and first-valid-commit-wins.

The foundation remains disconnected. Engine integration, product Interact
binding, a second production Player, and Contextual Revive Runtime
Orchestration remain deferred; the latter is the intended v0.3.41 milestone.
