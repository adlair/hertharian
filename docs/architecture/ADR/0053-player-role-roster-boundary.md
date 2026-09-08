# ADR-0053: Define Player Role Through a Fixed Caller-Owned Roster

- Status: Accepted
- Milestone: v0.3.29

## Decision

PLAYER ROLE = EXPLICIT PLAYER ROSTER MEMBERSHIP.

PLAYER ROSTER IS CALLER-OWNED.

CALLER-OWNED FIXED ROSTER.

MAX PLAYERS = 4 IS FROZEN IN FOUNDATION.

STABLE SPARSE PLAYER SLOTS.

NO AUTOMATIC COMPACTION.

PLAYER SLOT AND ENTITY HANDLE ARE DISTINCT IDENTITIES.

REGISTERED PLAYER MUST BE LIVE ENTITY + ACTOR.

SPATIAL NOT REQUIRED BY PLAYER ROLE.

HEALTH NOT REQUIRED BY PLAYER ROLE.

ROSTER DOES NOT KNOW PLAYERBODY.

ROSTER USES BRIDGE ENTITY IDENTITY ONLY.

ROSTER DOES NOT OWN PLAYER DEFEAT STATE.

ROSTER DOES NOT OWN REVIVE WINDOW STATE.

SAME ROSTER IMPLIES BASE COOP TEAM/SESSION MEMBERSHIP.

ZERO HEAP ALLOCATION.

DISCONNECTED PLAYER ROSTER FOUNDATION.

The private roster has four entries containing one Entity handle and one
structural occupancy bit each. Zero initialization is empty and no derived
count is stored. Valid slots are zero through three; four is the invalid output
sentinel. Registration validates live Entity+Actor, rejects full-handle
duplicates, and deterministically chooses the lowest free slot. Unregister is
slot-based, membership-only, and can clean a stale entry without Registry or
Actor Store access.

Entries never compact. Iteration is a fixed ascending slot scan with no
iterator object. Get and find validate that an occupied association remains a
current live Entity+Actor. Full handles prevent a replacement generation from
inheriting membership. Stale handles remain occupied until explicit cleanup;
they are distinct from malformed local handles and exact duplicate entries.

## Rejected Alternatives

- An Entity-indexed PlayerStore does not provide stable session-slot order.
- A dynamic vector or linked list adds allocation and growth without value for
  the fixed four-Player product limit.
- Omitting the occupancy bit would make zero initialization depend on a second
  noncanonical invalid Entity representation.
- A stored count duplicates state that can be derived by scanning four slots.
- Automatic compaction changes Player slot identity after unrelated removal.
- Embedding or referencing PlayerBody or Player Target Bridge conflates role
  membership with physical or proxy ownership.
- Embedding Health, Death, Defeat, Revive Window, activity, connection, Team,
  Session, network, character, inventory, or progression state turns the
  identity roster into an unrelated runtime aggregator.
- Engine integration would combine foundation validation with a multiplayer
  ownership and frame-loop migration.

## Consequences

Player is now distinguishable from a generic Actor while Entity, Actor,
PlayerBody, Bridge, Health, and lifecycle authorities remain separate. A
future owner can associate per-Player systems by stable slot, add explicit
avatar rebind for Respawn, evaluate Revive Eligibility within one roster, and
aggregate Session Outcome without changing this identity boundary.

The foundation remains private and disconnected. It allocates nothing, has no
production caller, and performs no per-frame work.
