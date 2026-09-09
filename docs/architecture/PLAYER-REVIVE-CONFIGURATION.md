# Player Revive Configuration Foundation

Hertharian v0.3.38 introduces a private, disconnected value that groups the
product tuning required by the released Player Revive foundations. It is
caller-owned, copyable by value, allocation-free, and has no runtime consumer.

## Shape and Validation

```c
typedef struct HTHPlayerReviveConfig {
    double revive_window_duration_seconds;
    float revive_range;
    double hold_duration_seconds;
    float revive_health;
} HTHPlayerReviveConfig;
```

These are the only current fields. The type contains no pointer, `HTHKey`,
Player identity, runtime state, policy selector, or ownership relationship.
Controls and local profiles own the separate Interact binding; this module
does not choose a product key.

`hth_player_revive_config_is_valid()` rejects a null pointer and requires every
field to be finite and strictly positive. `{0}` is invalid/unconfigured; it
does not request implicit defaults. The validator does not mutate its input,
clamp values, supply fallbacks, or impose arbitrary balance limits. Matching
finite positive extremes such as `FLT_MIN`, `FLT_MAX`, `DBL_MIN`, and
`DBL_MAX` are therefore technically valid. This answers whether values can be
supplied safely to existing foundations, not whether they are good balance.

The deterministic return-by-value default factory supplies these prototype /
initial tuning values:

| Field | Type | Initial value |
| --- | --- | ---: |
| `revive_window_duration_seconds` | `double` | `10.0` seconds |
| `revive_range` | `float` | `2.0F` world units |
| `hold_duration_seconds` | `double` | `2.0` seconds |
| `revive_health` | `float` | `25.0F` healing amount |

No authoritative numeric revive values existed before v0.3.38. These values
are tunable prototype defaults, not permanent balance, ABI, protocol, format,
or architectural invariants. A ten-second window provides a practical rescue
opportunity while retaining urgency; a two-world-unit range requires clear
proximity without claiming physical meters; a two-second hold prevents an
instant revive and preserves combat exposure. `revive_health` is an absolute
healing amount, not resulting Health or a percentage. Health adds the amount
and clamps the result to the target maximum.

## Temporal and Consumer Semantics

The config stores no captured attempt state. A current range affects the next
Target Selection and every held Interaction step. Interaction captures hold
duration into its existing `required_seconds` when an attempt starts or
retargets, so later config changes do not rewrite that active attempt. The
current revive-health value on the completion frame reaches Execution. Window
duration applies when a new cooperative downed episode begins and does not
overwrite an already-active window.

Released foundations retain scalar APIs and defensive validation. A future
orchestrator may extract the four fields for ReviveWindow/Lifecycle, Target
Selection, Interaction, and Execution, but those primitives do not accept a
config pointer. This module owns no Store, manager, registry, global mutable
state, lifecycle, heap allocation, runtime instance, or per-frame work. Both
validation and default construction are `O(1)` time and `O(1)` auxiliary
memory.

The caller-owned boundary remains compatible with future per-Player,
character, difficulty, or authoritative-network selection without
implementing any such system now. Before contextual Revive orchestration, the
next architectural prerequisite is a Multi-Player Death Snapshot / Frame
State audit so all consumers can share authoritative per-Player frame state.
