# ADR-0038: Player Target Bridge Boundary

- Status: Accepted
- Release: Hertharian Engine v0.3.14

## Context

The local Player's physical authority is `HTHPlayerBody`, while Enemy Target,
Perception, LOS, Decision, Seek, Chase, and Pursuit foundations compose around
generation-safe Entity handles and Spatial. Migrating the Player or teaching
each Enemy subsystem a second identity model would cross several released
boundaries merely to make the current Player position targetable.

The feasibility audit established:

```text
PLAYER TARGET BRIDGE FEASIBILITY PASS
ENTITY + SPATIAL PROXY SUFFICIENT
NO PLAYER MIGRATION REQUIRED
```

Existing candidate/target consumers require only a current Entity generation
and Spatial transform; they do not require a target Actor, Enemy, DynamicBody,
Health component, Camera, or Player-specific type.

## Decision

Represent the current non-Entity physical Player to Entity-based targeting
systems through a stable caller-owned Entity + Spatial proxy synchronized from
`HTHPlayerBody`'s physical body-center target anchor, without migrating Player
ownership, duplicating Player physics, coupling Enemy AI to Camera or
presentation state, or integrating Pursuit into the production frame loop.

The anchor is `(x, y + height * 0.5, z)`, computed after Player Body validation
with operands promoted to `double` and rejected unless finite and representable
as Spatial floats. Yaw is zero. Create publishes only a complete Entity +
Spatial composition and rolls back its Entity on attach failure. Sync preserves
identity and never repairs missing composition. Destroy removes only the proxy
composition; generation-safe Enemy Target semantics invalidate incoming
relationships without a scan or cascade.

The caller owns lifecycle and ordering. v0.3.14 adds no production call site;
future integration must synchronize after Player Movement and before Enemy
Pursuit Runtime consumes the proxy.

## Consequences

Enemy systems remain generic and Player Body remains the physical authority.
Player movement changes Spatial on one stable proxy generation, so stored
targets need not be replaced. The proxy intentionally has no physics,
collision, health, Actor semantics, rendering, or persistence.

Create/Destroy inherit possible Registry/Spatial allocation and growth. A
rolled-back Entity creation can consume a generation and retained Store
capacity is not rolled back. The Bridge performs no direct allocation.

## Rejected Alternatives

- Migrate Player completely to Entity now: exceeds this compatibility milestone.
- Make Player an Actor: invents unsupported gameplay composition.
- Add DynamicBody to the proxy: duplicates Player physics authority.
- Add Health to the proxy: targetability does not require damageability.
- Target Camera or the physical eye: couples AI to view/presentation state.
- Create and destroy the proxy every frame: breaks stable target identity.
- Use an EnemyTarget union of Entity and PlayerBody: weakens the released
  generation-safe relationship contract.
- Special-case Player in Target Selection: leaks Player identity into generic
  policy.
- Add a global Player singleton inside Enemy AI: introduces hidden ownership.
- Add a PlayerTargetBridge Store: one caller-owned bridge needs no component
  storage.
- Add a Player target manager: introduces lifecycle infrastructure without a
  current need.
- Integrate automatically in v0.3.14: orchestration belongs to a later milestone.
