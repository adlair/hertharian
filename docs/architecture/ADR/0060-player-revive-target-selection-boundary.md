# ADR-0060: Player Revive Target Selection Boundary

## Status

Accepted for Hertharian Engine v0.3.36.

## Context

Player Revive Interaction accepts an explicit candidate PlayerSlot but does
not discover one. The released Roster and Lifecycle Runtime now provide enough
current identity and policy authority to define discovery without Input,
Camera, LOS, a production cooperative runtime, or new persistent state.

## Decision

Player Revive Target Selection is a private, disconnected,
lifecycle-bound pure query. There is no separate selection core, Lifecycle
adapter, new Lifecycle accessor, or new Lifecycle state. Its output identity
is `HTHPlayerSlot`; normal no-target is technical success plus
`HTH_PLAYER_SLOT_INVALID`, while every writable technical-failure path leaves
that same canonical output.

The query resolves a valid current reviver from Lifecycle Runtime's Roster and
requires its Health and Spatial associations. A dead or defeated but otherwise
valid reviver produces successful no-target policy. Invalid, empty, stale, or
missing-Spatial revivers fail technically.

The selector scans the fixed current capacity `HTH_MAX_PLAYERS`, skips sparse
slots and self, and calls `hth_player_lifecycle_runtime_can_revive` for every
current non-self candidate. Eligibility remains the sole revive-policy
authority. Technical candidate failures abort the query; valid policy
ineligibility skips the candidate. Candidate Spatial is queried only after
Eligibility: an ineligible candidate needs none, while an eligible candidate
without valid Spatial is a technical failure.

The caller owns a finite, strictly positive revive range. Ranking uses
world-space Euclidean 3D positions, promotion to `double` before subtraction,
and finite squared distance/range calculations. The boundary is inclusive.
No `sqrt` or epsilon is used. The nearest eligible in-range Player wins, with
an explicit lower-PlayerSlot rule for an exact tie.

Each call recomputes from current state. There is no target memory,
reservation, lock, ownership claim, or hysteresis, so multiple revivers may
select the same target and target changes take effect immediately. The query
has no Input, Camera, facing, FOV, LOS, Collision, Interaction-state,
Execution, Enemy, networking, or Engine dependency; it mutates nothing,
allocates nothing, uses `O(1)` auxiliary memory, and runs in `O(P)` for
`P <= 4`.

## Consequences

Engine gains no integration, runtime consumer, second Player, or per-frame
selection work. Interaction and Execution remain unchanged and uninvoked by
this query.

In a four-current-Player fixture the selector makes at most three
`can_revive` calls. Current Eligibility performs two Death queries per call,
so that path performs six Death queries. This is foundation-acceptable bounded
work and a production-integration concern. A future snapshot-aware path is
required before production integration.

Future orchestration must also distinguish successful no-target from a valid
target. It must not pass INVALID blindly to a held v0.3.33 Interaction step,
whose existing candidate contract rejects it. No Interaction redesign belongs
to this milestone.
