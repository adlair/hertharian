# ADR-0045: Enemy Attack Execution Boundary

- Status: Accepted
- Release: Hertharian Engine v0.3.21

## Context

Enemy Decision can now express `ATTACK`, Pursuit Runtime suppresses pursuit
movement for that intent, and the Player Target Bridge supplies a valid Actor
and Health target. The released DamageIntent foundation already defines the
generic Actor-to-Actor damage request and its separate explicit resolution.
The missing boundary is a minimal way for an explicit caller to materialize
one Enemy attack without introducing cadence or runtime combat.

## Decision

Represent one explicit Enemy attack as exactly one `HTHDamageIntent` built by
`hth_enemy_attack_build_damage_intent()`. Attack Execution validates that the
source is a live, generation-current Entity with Actor and Enemy associations,
constructs the candidate from the caller's target and damage, and delegates
final validity to `hth_damage_intent_is_valid()`.

The builder canonicalizes output before validation and publishes the complete
candidate only on success. It owns no state, mutates no Store, allocates no
memory, and remains disconnected from production until attack cadence exists.

It does not resolve Health, re-evaluate geometry or policy, or depend on
HealthStore, Spatial, DynamicBody, Collision, LOS, Perception, Eligibility,
Decision, or EnemyTargetStore. A Player target is an ordinary valid Actor
target, not a special case.

## Consequences

Enemy-specific source validation composes cleanly with the released generic
DamageIntent target and amount semantics. Self-targeting, targets without
Health, zero damage, generation safety, and explicit downstream resolution
retain their existing meaning. One successful call yields one ephemeral
value and nothing else automatically.

Production still performs zero builder calls and zero DamageIntent resolution
per frame. Cadence, cooldown, persistent attack state, and runtime integration
remain separate future decisions, preventing accidental damage-per-frame.

## Rejected Alternatives

- Resolve Health inside the builder or accept a HealthStore: conflates request
  construction with the released explicit resolution boundary.
- Require Spatial, DynamicBody, Collision, LOS, Eligibility, Decision, or an
  EnemyTargetStore: rechecks policy and geometry outside their authorities.
- Add an automatic runtime call: creates uncadenced damage-per-frame.
- Add persistent attack state or an implicit cooldown: prematurely designs the
  cadence milestone.
- Special-case Player targets: duplicates the generic Actor target contract.
- Redesign DamageIntent: changes an already sufficient released foundation.
