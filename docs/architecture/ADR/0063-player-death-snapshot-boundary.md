# ADR-0063: Player Death Snapshot Boundary

Status: Accepted

## Context

The fixed Player Roster supports four current, potentially sparse Players, but
the production Engine still acquires one local Player Death boolean. Future
multi-Player read-side consumers need a coherent observation without replacing
the released Health/Player Death authority or weakening live safety checks at
mutation commit.

## Decision

Hertharian v0.3.39 adds the private, disconnected
`HTHPlayerDeathSnapshot`, a caller-owned fixed-size value containing one
presence flag, complete `HTHEntityHandle`, and sampled Death boolean per
`HTHPlayerSlot`.

- PLAYER DEATH SNAPSHOT IS A FRAME OBSERVATION, NOT A NEW AUTHORITY.
- DEATH AUTHORITY REMAINS PLAYER DEATH / HEALTH.
- SNAPSHOT IS DEATH-SPECIFIC, NOT GENERIC PLAYER FRAME STATE.
- SNAPSHOT IS A CALLER-OWNED FIXED-SIZE VALUE TYPE.
- CAPACITY IS `HTH_MAX_PLAYERS`.
- ENTRY IDENTITY USES PLAYER SLOT + COMPLETE ENTITY HANDLE.
- GENERATION MISMATCH NEVER REUSES OLD FACT.
- SPARSE SLOTS ARE PRESERVED.
- ZERO SNAPSHOT IS CANONICAL EMPTY.
- SNAPSHOT IS IMMUTABLE AFTER BUILD.
- NO FRAME ID OR TIMING DEPENDENCY.
- BUILD QUERIES DEATH EXACTLY ONCE PER CURRENT PLAYER.
- BUILD IS TRANSACTIONAL.
- EMPTY ROSTER IS VALID.
- NO PARTIAL REFRESH.
- NO HEAP, STORE, HISTORY, OR MUTABLE GLOBAL STATE.
- DEFEAT / REVIVE WINDOW / SPATIAL REMAIN LIVE STATE.
- FRAME READ-SIDE CONSUMERS SHARE SNAPSHOT FACTS AFTER FUTURE INTEGRATION.
- REVIVE EXECUTION RETAINS LIVE MUTATION-TIME REVALIDATION.
- CORRECTNESS OVERRIDES UNIVERSAL ONCE-PER-FRAME QUERY PURITY.
- NO ENGINE INTEGRATION IN v0.3.39.

Build canonicalizes output to empty, scans the fixed Roster slots without
compaction, resolves each occupied current identity, and delegates exactly once
to `hth_player_death_is_dead()`. It publishes its local result only after all
entries succeed. Invalid current composition or malformed Roster state rejects
the entire build.

Query takes a slot and expected full handle. Technical argument failures return
false. Absence or identity mismatch returns semantic success with no applicable
fact; it never reuses an old generation's Death value and never means alive.

## Consequences

Snapshot acquisition performs exactly `P` Player Death queries for `P` current
Players, with no query for empty slots. Build is bounded O(P) for
`P <= HTH_MAX_PLAYERS == 4`, query is O(1), storage is fixed
O(HTH_MAX_PLAYERS), and heap allocation is zero.

Health, spawn, despawn, and slot-reuse mutations after build do not alter the
old observation. A whole-value rebuild observes current authority. The future
sampling boundary remains after Input/orientation/delta and before Lifecycle,
Movement, and Enemy Pursuit, but this release creates no runtime consumer or
per-frame work.

Future snapshot-aware Eligibility, Target Selection, and Interaction work must
share one policy authority. Revive Execution remains live-authoritative before
Health mutation to cover state changes after sampling and first-reviver-wins.
