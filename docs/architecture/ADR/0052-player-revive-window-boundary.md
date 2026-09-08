# ADR-0052: Keep the Revive Window a Disconnected Timer Primitive

- Status: Accepted
- Milestone: v0.3.28

## Decision

REVIVE WINDOW IS CALLER-OWNED.

REVIVE WINDOW DURATION IS CALLER/CONFIG OWNED.

REVIVE WINDOW DURATION MUST BE FINITE AND POSITIVE.

ZERO-DURATION WINDOW IS INVALID.

REVIVE WINDOW USES SIMULATION DELTA.

EXPIRY IS A ONE-SHOT ADVANCE RESULT.

EXPIRY IS NOT PERSISTENT STATE.

RESET ALSO SERVES AS CANCEL.

REVIVE WINDOW HAS NO PLAYER/HEALTH/DEATH/DEFEAT DEPENDENCY.

REVIVE WINDOW HAS NO ENTITY/ACTOR/SPATIAL DEPENDENCY.

EXPIRY DOES NOT MARK PLAYER DEFEAT DIRECTLY.

DOWNED IS NOT STORED BY REVIVE WINDOW.

REVIVE WINDOW DOES NOT PERFORM REVIVE.

PLAYER ROLE / ROSTER DEFERRED.

REVIVE ELIGIBILITY DEFERRED.

REVIVE EXECUTION DEFERRED.

DEFEAT RUNTIME INTEGRATION DEFERRED.

SESSION OUTCOME DEFERRED.

DISCONNECTED FOUNDATION.

The caller-owned state contains exactly `remaining_seconds` and `active`.
Zero initialization is inactive. Inactive is equivalent to a zero remainder;
active requires a finite positive remainder. Reset is null-safe and can recover
malformed local state. Begin, advance, and query defensively validate the local
invariants and fail transactionally when they are violated.

Begin accepts only a finite positive duration and an inactive window. Advance
accepts finite non-negative simulation delta. Inactive and zero-delta advances
are successful no-ops. Partial advance subtracts exactly; exact or overshooting
advance atomically saturates to zero, becomes inactive, and emits expiry once.
Query is pure. Outputs are canonicalized before validation.

No epsilon, catch-up, persistent expiry, allocation, Store, public API, or
production caller is introduced. Positive floating-point residue remains
active until a later sufficient delta. All operations are O(1).

## Rejected Alternatives

- Zero duration does not represent a meaningful Revive opportunity.
- Beginning while active must not restart, extend, replace, or shorten a
  window.
- An expired flag, `expiry_reported`, or lifecycle enum duplicates a transition
  already represented by the active-to-inactive advance result.
- Player/Entity handles, generation maps, and Stores prematurely introduce
  identity and roster ownership.
- Engine or Bootstrap ownership would create per-frame work before Revive is
  executable.
- Health mutation, Player Death queries, Player Defeat mutation, and Downed
  queries belong to future lifecycle composition.
- Reviver identity, eligibility, interaction, Input, range, LOS, Enemy policy,
  wall-clock timing, and Session Outcome exceed a timer primitive.

## Consequences

The foundation can be tested independently and later instantiated once Player
ownership exists. A future lifecycle adapter, not this timer, will compose the
authoritative Death snapshot, derived Downed policy, window cancellation, and
permanent Defeat transition. Until then there are zero production callers and
zero related per-frame work.
