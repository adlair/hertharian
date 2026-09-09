# ADR-0058: Keep Player Runtime Population Stateless and Disconnected

- Status: Accepted
- Milestone: v0.3.34

## Decision

PLAYER RUNTIME POPULATION FOUNDATION IS DISCONNECTED.

PLAYER IDENTITY DOES NOT REQUIRE PLAYERBODY.

PLAYER IDENTITY DOES NOT REQUIRE INPUT.

PLAYER IDENTITY DOES NOT REQUIRE CAMERA.

COOP-READY GAMEPLAY PLAYER =
ENTITY + ACTOR + SPATIAL + HEALTH + ROSTER.

PLAYER RUNTIME POPULATION IS STATELESS.

PLAYERROSTER REMAINS THE ONLY SLOT→ENTITY AUTHORITY.

PLAYERLIFECYCLERUNTIME REMAINS LIFECYCLE-STATE OWNER.

POPULATION REUSES ACTOR SPAWN/DESPAWN.

SPAWN PUBLISHES PLAYERSLOT ONLY AFTER FULL COMPOSITION + LIFECYCLE REGISTER.

LIFECYCLE REGISTER FAILURE ROLLS BACK VIA ACTOR DESPAWN.

DESPAWN RESOLVES ENTITY THROUGH ROSTER.

DESPAWN PREVALIDATES BEFORE LIFECYCLE UNREGISTER.

NO PLAYERSTORE.

NO ADOPT API.

NO BULK RESET.

PLAYER TARGET BRIDGE REMAINS LOCAL ADAPTER.

BRIDGE PLAYER AND GENERIC PLAYERS MAY COEXIST.

NO PLAYERBODY ARRAY.

NO INPUT/CAMERA/RENDERER/NETWORKING DEPENDENCY.

The private module accepts caller-owned Stores, Lifecycle Runtime, and a
Spatial-plus-Health spawn specification. It owns no persistent value or
parallel identity mapping. Spawn composes through Actor Spawn, registers
through Lifecycle Runtime, and publishes only the resulting PlayerSlot. A
registration failure rolls the Actor composition back. Despawn resolves the
current generation-safe Entity from the roster, validates the required
composition, unregisters lifecycle state, and delegates teardown to Actor
Despawn.

## Consequences

The foundation can compose up to `HTH_MAX_PLAYERS` gameplay identities without
pretending that every Player has a physical PlayerBody, Input source, or
Camera. Slot reuse inherits canonical lifecycle state, and Bridge and generic
Players can share roster identity while retaining separate creation and
destruction APIs. Engine has no caller, second production Player, or per-frame
population work in v0.3.34.

Future cooperative runtime work must establish multiple authoritative Death
snapshots, Enemy dead-target exclusion policy, Revive Target Selection,
semantic Interact input, and product revive values before production
integration.

Explicitly deferred items are:

- MULTI-PLAYER DEATH SNAPSHOT BATCH;
- MULTI-PLAYER ENEMY CANDIDATE / DEAD TARGET EXCLUSION;
- PLAYER REVIVE TARGET SELECTION;
- SEMANTIC INTERACT INPUT ACTION;
- REVIVE PRODUCT CONFIG VALUES;
- REAL ADDITIONAL PLAYER SOURCE;
- PLAYER REPRESENTATION / CONTROL;
- PLAYER COOP RUNTIME / REVIVE INTERACTION INTEGRATION.

## Rejected Alternatives

- A PlayerStore or population-owned slot map would duplicate PlayerRoster.
- Persistent ownership or provenance metadata is unnecessary for the current
  caller contract.
- Adopt and bulk-reset APIs would add lifecycle policy not required by one
  transactional spawn/despawn boundary.
- Creating PlayerBody, Input, Camera, DynamicBody, rendering, or networking
  state would conflate gameplay Player identity with future local/remote
  runtime capabilities.
- Replacing Actor Spawn/Despawn or mutating Store internals directly would
  duplicate released composition authority.
