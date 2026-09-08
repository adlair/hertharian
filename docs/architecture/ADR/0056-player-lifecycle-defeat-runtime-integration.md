# ADR-0056: Integrate Player Lifecycle and Defeat Through Stable Player Slots

- Status: Accepted
- Milestone: v0.3.32

## Decision

ENGINE-OWNED HTHPLAYERLIFECYCLERUNTIME.

PLAYER SLOT INDEXES DEFEAT + REVIVE WINDOW STATE.

PER-SLOT WAS_DEAD HISTORICAL EDGE MEMORY REQUIRED.

DOWNED REMAINS DERIVED; NO PERSISTENT DOWNED FLAG.

LIFECYCLE CONSUMES EXISTING AUTHORITATIVE DEATH SNAPSHOT.

PLAYER DEATH QUERY REMAINS EXACTLY ONCE PER FRAME.

BEGIN WINDOW ON ALIVE→DEAD EDGE ONLY IN COOP.

NEWLY BEGUN WINDOW IS NOT ADVANCED UNTIL NEXT LIFECYCLE STEP.

WINDOW EXPIRY WHILE STILL DEAD MARKS PERMANENT DEFEAT.

CONTINUING DEAD + INACTIVE WINDOW RESOLVES TO DEFEAT.

ALIVE PLAYER CANCELS ANY ACTIVE REVIVE WINDOW.

LIFECYCLE RECONCILES HEALTH>0 AS ALIVE REGARDLESS HEALING SOURCE.

DEFEAT IS PERSISTENT AND NOT CLEARED BY HEALTH RECOVERY.

SOLO PLAYER IMMEDIATELY DEFEATED.

REVIVE WINDOW DURATION IS CALLER/CONFIG OWNED.

REGISTER CURRENT BRIDGE PLAYER INTO PRODUCTION LIFECYCLE ROSTER.

ENGINE STORES RETURNED LOCAL PLAYERSLOT; SLOT 0 NOT SEMANTIC.

REVIVE EXECUTION REMAINS DISCONNECTED UNTIL INTERACTION EXISTS.

FUTURE REVIVE INTERACTION COMPLETES BEFORE AUTHORITATIVE DEATH SNAPSHOT.

SESSION OUTCOME DEFERRED.

Engine owns one private fixed runtime containing the identity-only Roster and
parallel Defeat, ReviveWindow, and `was_dead` values for four stable slots.
Registration and unregistration mediate roster mutations and canonicalize the
associated lifecycle values, preventing state transfer on slot reuse. The
existing Bridge Entity is registered after its Entity + Actor composition is
complete, and Engine retains the returned local slot without hardcoding zero.

One slot step receives Engine's existing `player_dead` snapshot. It validates
the current roster membership and simulation delta before mutation. Solo fresh
death marks Defeat directly. Cooperative fresh death requires a caller-owned
finite positive duration, begins one Window, and consumes no delta that step.
Continuing Downed advances once; expiry or an already inactive opportunity
marks Defeat. Alive and defeated reconciliation cancel meaningless active
Windows, while only alive commits `was_dead=false`. Failed processing commits
no new historical snapshot.

Expiry uses prevalidated runtime-owned state, then Window advance followed by
the released Defeat mark. The mark can fail only for null state, which is
structurally unreachable for an indexed owned array; rollback is unnecessary.

The Death snapshot remains before movement and Enemy Runtime. Lifecycle,
movement gating, and dead-target exclusion consume the same value, preserving
the one-frame observation of Enemy damage. Future authorized Revive Execution
must occur before that snapshot, allowing successful healing and Window reset
to win before lifecycle expiry without a second Death query or phase enum.

## Consequences

Current production has one Player and therefore follows immediate solo Defeat
without inventing a Revive duration. Cooperative state is fully represented
and tested but has no production population or Interaction. Defeat is now real
persistent runtime state but has no movement, targeting, Session Outcome, or
presentation authority.

PlayerRoster, Player Death, Player Defeat, ReviveWindow, Eligibility, Execution,
Health, PlayerBody, movement, targeting, and Enemy foundations remain unchanged.
The runtime adds no public API, heap allocation, PlayerBody array, Input, LOS,
range, networking, or lifecycle event framework.

## Rejected Alternatives

- Lifecycle fields inside PlayerRoster would violate its identity-only role.
- A persistent Downed bool or lifecycle phase enum would duplicate derived
  authorities.
- Querying Death inside lifecycle would duplicate the released frame snapshot.
- A solo Window or bootstrap duration would contradict policy or invent balance.
- Advancing a new Window immediately would shorten the configured opportunity.
- Restarting an inactive continuing episode would erase expiry/cancellation.
- Clearing Defeat on positive Health would violate its persistent orthogonality.
- Healing provenance, revive tokens, automatic Execution, and a two-phase
  runtime add no required invariant to this milestone.
- Hardcoded slot zero or `HTHPlayerBody[4]` would conflate lifecycle capacity
  with current physical Player ownership.
