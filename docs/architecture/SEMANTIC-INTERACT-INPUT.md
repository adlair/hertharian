# Semantic Interact Input Foundation

Hertharian v0.3.37 introduces a private, disconnected semantic Interact action.
It projects one caller-owned physical `HTHKey` binding from final `HTHInput`
state into a gameplay-facing value:

```c
typedef struct HTHGameplayActionState {
    bool down;
    bool pressed;
    bool released;
} HTHGameplayActionState;
```

Interact is a general gameplay action, not a Revive-specific action. Gameplay
owns the projection; Input remains the authority for physical keyboard state,
and Platform remains unaware of gameplay meaning. No product key is frozen:
the caller supplies the binding to every query, and a different valid binding
takes effect immediately without captured configuration or persistent action
state.

## Exact Projection Contract

For one valid binding, `down`, `pressed`, and `released` map directly to
`hth_input_key_down`, `hth_input_key_pressed`, and
`hth_input_key_released`. The query consumes the final observable Input
snapshot and does not rebuild edges. Consequently the canonical sequence is:

```text
key down       -> { down=true,  pressed=true,  released=false }
next held frame -> { down=true,  pressed=false, released=false }
key up         -> { down=false, pressed=false, released=true  }
next frame     -> { down=false, pressed=false, released=false }
```

Input latches edges for the frame. Down then up may therefore produce
`{false, true, true}`; down/up/down and a same-frame release/repress may produce
`{true, true, true}`. These are valid snapshots, and the semantic projection
preserves them exactly.

A repeat key-down establishes Input's `down` state but never creates a new
`pressed` edge. This also applies when a repeat restores `down` after keyboard
release reconciliation. Focus loss inherits Input's release normalization, so
a held action cannot remain stuck; focus gain alone creates neither `down` nor
`pressed`. Relative mouse capture transitions do not redefine keyboard action
state. All focus, repeat, reconciliation, and frame-latch safety remains in
Input rather than being duplicated here.

Writable output is canonicalized to all false before validation. A null Input,
null output, `HTH_KEY_UNKNOWN`, `HTH_KEY_COUNT`, or a value outside the valid
key domain is a technical failure. Invalid binding is not an unbound policy;
unbound configuration remains deferred.

## Frozen Boundary

The projection is stateless and pure with respect to Input. It owns no store,
global, allocation, target, consumer, action arbitration, or consumption
policy. It performs `O(1)` work and uses `O(1)` auxiliary memory. It has no
direct Platform, SDL, X11, Wayland, Player, PlayerSlot, Spatial, Revive,
Camera, Enemy, Collision, Renderer, or networking dependency.

v0.3.37 intentionally supports exactly one keyboard binding per query. It does
not aggregate multiple keys, mouse buttons, or gamepad controls. Aggregate
release semantics can require persistent aggregate state: if A and B are down,
releasing A while B remains down need not mean the semantic action was
released. A future rebinding/gamepad resolver may preserve this action-state
shape while defining that policy. No general binding system, command frame,
serialization, controls menu, or gamepad implementation is introduced now;
the state may later feed a command-frame representation.

## Disconnected Future Composition

Engine has no Interact caller, owns no semantic action state, and performs no
per-frame semantic Interact work. Current production Player count and all
Revive behavior remain unchanged.

A future contextual gameplay router may compose:

```text
final HTHInput -> Interact state -> contextual router
-> Revive Target Selection -> Revive Interaction -> Revive Execution
```

That orchestrator must cancel/reset a future active interaction when Interact
is not down. When Interact is down but Target Selection returns
`HTH_PLAYER_SLOT_INVALID`, it must not blindly call the existing held Revive
Interaction step with that invalid candidate. A valid selected target may be
passed onward under the released Revive contracts. Context choice, target
selection, consumption/arbitration, product binding, multiplayer ordering, and
production integration remain explicit readiness gaps outside v0.3.37.

Player Revive Configuration, introduced as a disconnected private foundation
in v0.3.38, deliberately contains no `HTHKey` and chooses no product Interact
binding. Controls configuration and Revive gameplay tuning remain separate
inputs to future contextual orchestration.
