# ADR-0044: Player Damage Target Identity

- Status: Accepted
- Release: Hertharian Engine v0.3.20

## Context

The Player Target Bridge introduced in v0.3.14 exposes the local
`HTHPlayerBody` as one stable Entity + Spatial target. Enemy Attack Intent can
now select that handle, but released `HTHDamageIntent` requires both source
and target to be Actors, and resolution reaches Health only on the same target
handle. The v0.3.14 proxy intentionally had neither association.

## Decision

Evolve the existing Bridge-owned Entity into the canonical Entity-based
Player damage target by attaching Actor and Health while retaining the same
Spatial and generation-safe handle:

```text
Entity + Actor + Spatial + Health
```

Reuse Actor Spawn with Spatial and Health enabled and DynamicBody disabled;
reuse Actor Despawn for teardown. `HTHPlayerBody` remains the physical and
movement authority, Spatial remains a one-way body-center mirror, and
`HTHHealthStore` is the only Player Health authority. The Bridge continues to
store only `target_entity`.

## Consequences

Enemy targeting and future damage use one handle without a mapping layer.
DamageIntent and Health semantics remain unchanged. Player receives no Enemy
or DynamicBody association, enters no Enemy loop, gains no generic movement or
rendering, and has no automatic death behavior at zero Health.

Production supplies temporary bootstrap Health of `100/100`, but creates and
resolves no DamageIntent in this release. Attack execution and cadence remain
separate future decisions.

Destroying the identity invalidates incoming Enemy Target relations and old
Damage Intents through existing generation checks; recreation cannot revive
them. Independent Bridge instances retain a path toward multiple Players.

## Rejected Alternatives

- A second Player damage Entity: duplicates identity and requires a stale-
  prone proxy mapping.
- Health fields in PlayerBody: creates a second damage model and couples
  movement state to gameplay Health.
- Player DynamicBody migration: duplicates or replaces the released Player
  movement authority without need.
- Weakening DamageIntent to accept non-Actor targets: breaks its released
  Actor-to-Actor semantic contract.
- A PlayerHealth module or Store: duplicates the generic Health authority.
- Renaming Player Target Bridge: adds migration churn without clarifying the
  deliberately narrow target-identity responsibility.
