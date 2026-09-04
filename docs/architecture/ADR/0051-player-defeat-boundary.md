# ADR-0051: Keep Player Defeat Explicit and Separate from Player Death

- Status: Accepted
- Milestone: v0.3.27

## Decision

PLAYER DEATH AND PLAYER DEFEAT ARE DISTINCT.

DOWNED STATE DEFERRED.

PERSISTENT PLAYER DEFEAT STATE REQUIRED.

CALLER-OWNED PLAYER DEFEAT STATE.

DEFEAT TRANSITION CAN BE EXPLICITLY MARKED NOW.

EXPLICIT RESET IS VALID FOUNDATION SEMANTICS.

DEFEAT ORTHOGONAL TO CURRENT HEALTH.

FUTURE REVIVE REMAINS COMPATIBLE.

PER-PLAYER DEFEAT MODEL IS FUTURE-COOP COMPATIBLE.

DISCONNECTED FOUNDATION.

Represent Defeat with one caller-owned boolean. Zero initialization and reset
mean not defeated. Mark changes false to true and is idempotent for true. Reset
changes either value to false. Query canonicalizes its output before
validation. No operation reads or mutates Player Death or Health.

Defeated with positive Health is valid: Health describes current numerical
life while Defeat records a persistent lifecycle result until explicit reset.
Likewise, zero Health without Defeat is valid and does not automatically mean
Downed. No automatic Health-zero-to-Defeat edge is established.

The state contains no Entity mapping. A caller that associates an instance
with a different logical Player identity must reset it first. Multiple callers
may own independent instances, preserving future cooperative composition.
Production owns no instance and makes no call in this milestone.

## Rejected Alternatives

- Game Over and Session Outcome conflate a per-Player fact with session policy.
- A PlayerStore, Defeat Store, Entity handle, or generation map prematurely
  introduces dynamic Player ownership and roster infrastructure.
- ACTIVE/DOWNED/DEFEATED, PlayerLifecycle, or PlayerStatus enums freeze states
  whose triggers and timers remain undefined.
- Health-owned Defeat duplicates or contaminates numeric life authority.
- Bridge-owned, PlayerBody-owned, Actor-owned, or Engine-owned Defeat mixes the
  fact with identity adaptation, physics, generic participation, or technical
  orchestration.
- Automatic mark on Health zero blocks future Downed/Revive policy and
  conflicts with released reversible Player Death semantics.
- ReviveWindow, Downed behavior, timers, automatic reset, and per-frame
  integration exceed this foundation.

## Consequences

Defeat adds a persistent fact that Player Death does not provide while leaving
all existing runtime behavior unchanged. Healing never clears it, mark never
kills, and reset never heals. The primitive is private, allocation-free, O(1),
and independently instantiable. Future lifecycle policy may decide when to
mark/reset it; future Session Outcome may aggregate it across a roster.
