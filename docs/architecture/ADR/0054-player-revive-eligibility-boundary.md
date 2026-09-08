# ADR-0054: Keep Player Revive Eligibility a Pure Pair Policy

- Status: Accepted
- Milestone: v0.3.30

## Decision

REVIVE ELIGIBILITY USES PLAYER SLOTS.

SAME ROSTER + DIFFERENT SLOT = BASE TEAMMATE RELATION.

REVIVER ACTIVE = CURRENT PLAYER + NOT DEAD + NOT DEFEATED.

TARGET DOWNED = CURRENT PLAYER + DEAD + NOT DEFEATED + ACTIVE REVIVE WINDOW.

SELF REVIVE INELIGIBLE BY BASE POLICY.

REVIVE ELIGIBILITY HAS NO SPATIAL DEPENDENCY.

NO LOS DEPENDENCY.

NO INPUT DEPENDENCY.

ELIGIBILITY DOES NOT MUTATE HEALTH OR WINDOW.

ELIGIBILITY DOES NOT MUTATE PLAYER DEFEAT.

ELIGIBILITY DOES NOT MUTATE PLAYER ROSTER.

TECHNICAL BOOL + OUT_ELIGIBLE.

EXPLICIT CALLER-PROVIDED DEFEAT/WINDOW STATES ARE SUFFICIENT.

CALLER OWNS SLOT ↔ LIFECYCLE STATE BINDING.

STATELESS PURE POLICY QUERY.

VALIDATE ALL TECHNICAL DEPENDENCIES BEFORE POLICY SHORT-CIRCUIT.

VALID BUT ODD CROSS-STATE COMPOSITIONS RESOLVE AS POLICY-NEGATIVE.

NO REASON ENUM IN FIRST FOUNDATION.

ZERO HEAP ALLOCATION.

DISCONNECTED REVIVE ELIGIBILITY FOUNDATION.

One private query accepts two Player slots in one roster and explicit
caller-bound Defeat states plus the target ReviveWindow. It resolves both
current Players through PlayerRoster, derives Death through Player Death, and
queries Defeat and ReviveWindow through their released authorities. It never
inspects `Health.current`, `defeated`, `active`, or `remaining_seconds`
directly.

The technical return is separate from the eligibility output, which is
canonicalized before validation. Invalid, empty, stale, or missing-component
Player identities and malformed dependencies are technical failures. Once all
dependencies validate, self-pair, dead or defeated reviver, alive or defeated
target, and inactive target window are successful policy-negative results.
This validation-first order prevents an early negative predicate from hiding
a malformed later dependency.

Target alive with an active window, target defeated with an active window,
target dead with an inactive window, both Players Downed, and defeated with
positive Health remain individually valid compositions. Policy precedence
makes them ineligible rather than treating them as structural corruption.

## Rejected Alternatives

- Entity or Actor fallback would bypass explicit PlayerRoster membership.
- Team or Session IDs are unnecessary for the base single-roster domain.
- Spatial, range, LOS, Input, and interaction progress gate a future attempt;
  they do not define semantic eligibility.
- Health restoration and Window cancellation belong to Revive Execution.
- Direct field inspection would duplicate Health-derived Death, Defeat, or
  ReviveWindow authority.
- Mutating Defeat, Window, roster, Entity, or Actor would turn a policy query
  into lifecycle execution.
- Persistent Downed or Eligibility state would duplicate derived facts.
- A PlayerLifecycleState, parallel-array owner, or Store introduced only to
  shorten the signature would add ownership without a new invariant.
- A reason enum would prematurely expose internal policy branches.
- Engine or Bootstrap integration would create runtime ownership before a
  Revive path exists.

## Consequences

Eligibility is a deterministic, allocation-free, bounded `O(1)` composition
of released authorities. Defeat and Window remain caller-owned; correct
slot-to-state association is an explicit caller precondition. The foundation
has no state, Store, owner, public API, production caller, or per-frame work.

Future Revive Execution may consume an eligible pair and apply a separately
approved Health/window policy. Future Interaction and Runtime policy may add
distance, Input, hold progress, interruption, animation, HUD, and audio.
Lifecycle/Defeat Runtime Integration, Session Outcome, self-revive
capabilities, inventory, progression, respawn, and networking remain deferred.
