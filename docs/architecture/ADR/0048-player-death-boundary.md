# ADR-0048: Derive Player Death from Current Health

- Status: Accepted
- Milestone: v0.3.24

## Context

Production Enemy attacks can reduce the Health attached to the Player Target
Bridge identity to zero. Health already defines zero as valid, saturates damage
there, permits healing back to a positive value, and remains the sole authority
for life amount. No runtime system yet consumes Player death consequences.

## Decision

PLAYER DEATH IS DERIVED FROM HEALTH ZERO.

Provide one internal, pure `hth_player_death_is_dead()` query. The caller
supplies the explicit Player gameplay handle created and owned by Player Target
Bridge. The query requires its current Entity generation, Actor association,
and valid Health association, then classifies exactly `Health.current == 0.0F`
as dead. It canonicalizes output to false before validation and rejects missing
or stale structure without mutation.

Health remains the sole life authority. No Player role, Player Store, Player
Death Store, state allocation, bool, enum, latch, history, or event is added.
Lookup is generation-safe, deterministic, allocation-free, and O(1).

HEALING ABOVE ZERO RESTORES ALIVE STATE. This follows directly from querying
current Health and does not establish revive behavior.

TARGETING POLICY DEFERRED BUT IDENTITY PRESERVED. The Player Entity, Actor,
Spatial, Health, stable handle, and EnemyTarget relations remain intact.

MOVEMENT POLICY DEFERRED. PlayerBody, movement, Input, Camera, and rendering
remain unchanged.

DISCONNECTED PLAYER DEATH FOUNDATION. Production has zero callers and performs
zero Player Death work per frame in v0.3.24.

## Rejected Alternatives

- A PlayerDeath Store, persistent dead latch, Player Store, or dead flag in
  Health would duplicate current Health authority and require reset policy.
- A death flag in Player Target Bridge would mix identity adaptation with
  gameplay lifecycle.
- PlayerBody death state would couple Health semantics to physical movement.
- Using `HTHDamageResult.became_zero` would miss initial-zero identities and
  would retain an operation transition rather than query current state.
- Automatic target clearing, Enemy dead-target filtering, Player despawn,
  movement or Input suppression, and death Camera behavior are runtime policy.
- Respawn, revive, Downed, and game-over behavior require lifecycle decisions
  outside this foundation.
- Calling the query from Engine, bootstrap, or Enemy runtime without a consumer
  would add purposeless per-frame work.

## Consequences

Any valid positive Health value means alive, including very small positive
values; no epsilon is used. Initial zero, exact lethal damage, and overkill mean
dead. Repeated damage at zero remains harmless saturated damage. Healing above
zero immediately changes the derived classification to alive.

The module depends only on Entity, Actor, and Health and exposes no public API.
A later Player Death Runtime Integration milestone may consume the query and
define movement, targeting, Input, Camera, or game-over policy without changing
this Health authority boundary.
