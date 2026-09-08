# ADR-0057: Keep Player Revive Interaction a Per-Reviver Spatial Hold

- Status: Accepted
- Milestone: v0.3.33

## Decision

DISCONNECTED PLAYER REVIVE INTERACTION FOUNDATION.

REVIVER SLOT + TARGET SLOT.

CALLER SUPPLIES TARGET PLAYERSLOT.

REVIVE INTERACTION REQUIRES SPATIAL PROXIMITY.

3D WORLD-SPACE POSITION DISTANCE <= CALLER CONFIG RANGE.

NO LOS IN FIRST REVIVE INTERACTION FOUNDATION.

NO FACING REQUIREMENT.

CALLER PASSES SEMANTIC HELD BOOLEAN.

HOLD-TO-REVIVE.

CALLER/CONFIG OWNED FINITE POSITIVE DURATION.

PER-REVIVER ATTEMPT ELAPSED SIMULATION SECONDS.

HOLD DURATION CAPTURED PER ATTEMPT.

CURRENT RANGE CONFIG APPLIES EACH STEP.

CURRENT REVIVE HEALTH APPLIES AT COMPLETION.

RELEASE / RANGE LOSS / ELIGIBILITY LOSS / TARGET CHANGE RESET PROGRESS.

MOVEMENT DOES NOT CANCEL INITIAL FOUNDATION.

DAMAGE DOES NOT CANCEL INITIAL FOUNDATION.

INDEPENDENT ATTEMPTS; FIRST EXECUTION SUCCESS WINS.

INTERACTION REVALIDATES ELIGIBILITY DURING ATTEMPT.

COMPLETION DELEGATES TO HTH_PLAYER_REVIVE_EXECUTE; EXECUTION REVALIDATION
REMAINS FINAL AUTHORITY.

EXECUTION TECHNICAL FAILURE PRESERVES PRE-STEP INTERACTION STATE.

INTERACTION/EXECUTION BEFORE AUTHORITATIVE DEATH SNAPSHOT.

TECHNICAL BOOL + OUT_REVIVED + STATE QUERY.

CALLER-OWNED ZEROABLE INTERACTION STATE.

PRODUCTION REVIVE INTERACTION REMAINS DISCONNECTED.

One caller-owned value tracks only one reviver's active target slot, elapsed
simulation seconds, captured duration, and activity bit. It caches no Entity
or position. Every held step resolves current roster identities and Spatial
anchors, applies inclusive squared 3D range using double intermediates, and
delegates lifecycle semantics to the released authorities. Policy cancellation
resets progress; technical failure preserves the complete pre-step value.

Lifecycle Runtime gains only two private binding adapters. They validate slots
before indexing the fixed arrays and delegate exactly once to Eligibility or
Execution. They add no predicates, expose no mutable Window, and do not change
Roster, Defeat, Window, or `was_dead` ownership. Completion is provisional
until Execution returns; valid no-op and success reset the attempt, while
technical failure leaves it retryable.

## Consequences

The foundation remains deterministic, allocation-free, `O(1)`, and fully
testable with multiple locally composed test Players. Current production has
one Player, so Engine owns no interaction state and performs no interaction or
revive-execution work. A later coop runtime will own one attempt per
controllable Player, provide target acquisition and a semantic held action,
and schedule completion before the authoritative Death snapshot.

## Rejected Alternatives

- A generic Interaction framework, target scan/ranking, camera ray, physical
  key binding, or Engine caller would invent absent production architecture.
- Entity-handle identity or cached positions would duplicate stable PlayerSlot
  identity and stale current Spatial state.
- LOS, facing, Camera, PlayerBody, movement interruption, and non-lethal damage
  provenance are not required by the first semantic boundary.
- Shared progress or a target lock is unnecessary because Execution's final
  revalidation makes the first successful reviver win.
- Pause or decay adds state and policy beyond deterministic cancellation.
- Direct Health access, Defeat mutation, Window mutation, or mutable Window
  exposure would bypass released authorities.
- Self-revive, inventory, upgrades, networking, Session Outcome, Game Over,
  respawn, presentation, and production multiplayer belong to later work.
