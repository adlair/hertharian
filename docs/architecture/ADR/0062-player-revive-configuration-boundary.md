# ADR-0062: Player Revive Configuration Boundary

Status: Accepted

## Context

The released Revive pipeline accepts four caller-owned gameplay scalars across
ReviveWindow/Lifecycle, Target Selection, Interaction, and Execution. Product
tuning needs one coherent source without coupling these mechanical primitives
to controls, runtime state, or a configuration service.

## Decision

Hertharian v0.3.38 adds a private `HTHPlayerReviveConfig` value containing
exactly `double revive_window_duration_seconds`, `float revive_range`,
`double hold_duration_seconds`, and `float revive_health`.

- REVIVE GAMEPLAY CONFIG IS SEPARATE FROM CONTROL BINDINGS.
- PLAYER REVIVE CONFIG IS A CALLER-OWNED VALUE TYPE.
- CONFIG HAS EXACTLY FOUR CURRENT FIELDS.
- REVIVE WINDOW DURATION BELONGS TO THE SAME REVIVE GAMEPLAY CONFIG.
- ZERO CONFIG IS INVALID / UNCONFIGURED.
- VALIDATOR CHECKS TECHNICAL CONTRACTS ONLY.
- VALIDATOR REQUIRES FINITE STRICTLY POSITIVE VALUES.
- FOUNDATION PRIMITIVES RETAIN DEFENSIVE VALIDATION.
- DOWNSTREAM APIS REMAIN SCALAR-ORIENTED.
- NO CONFIG STORE.
- NO GLOBAL MUTABLE CONFIG.
- NO ENGINE INTEGRATION.
- NO RUNTIME CONSUMERS.
- NO HEAP.
- PROTOTYPE DEFAULT WINDOW = 10.0 SECONDS.
- PROTOTYPE DEFAULT RANGE = 2.0F WORLD UNITS.
- PROTOTYPE DEFAULT HOLD = 2.0 SECONDS.
- PROTOTYPE DEFAULT REVIVE HEALTH = 25.0F HEALING AMOUNT.
- DEFAULTS ARE INITIAL TUNING, NOT PERMANENT BALANCE.

`revive_health` is an absolute healing amount; it is neither final Health nor
a percentage. The config contains no `HTHKey`, attempt progress, window
remainder, Player identity, target, input state, or policy field. No product
Interact key is selected.

## Consequences

The validator is pure, rejects null/zero/non-finite/non-positive input, and
adds no arbitrary upper bound. The default factory is deterministic and owns
the four production literals in one source. Both operations are `O(1)`, use
`O(1)` auxiliary memory, and allocate nothing.

The module remains disconnected. Engine owns no config instance, runtime
consumer count and per-frame work are both zero, and existing Revive APIs and
their defensive validation are unchanged. A future orchestrator may choose a
caller-owned config and pass extracted scalars; it must first address the
Multi-Player Death Snapshot / Frame State boundary.
