# Player Revive Execution Foundation

Hertharian v0.3.31 defines one private, stateless operation that applies an
authorized Revive transition. It revalidates the released Eligibility policy
at mutation time, restores target Health through the existing authority, and
cancels the target ReviveWindow. It does not begin or manage an interaction.

## Operation Contract

```c
bool hth_player_revive_execute(
    const HTHPlayerRoster *roster,
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    HTHHealthStore *health,
    HTHPlayerSlot reviver_slot,
    const HTHPlayerDefeatState *reviver_defeat,
    HTHPlayerSlot target_slot,
    const HTHPlayerDefeatState *target_defeat,
    HTHPlayerReviveWindow *target_window,
    float revive_health,
    bool *out_revived);
```

The technical return and transition output are separate. The output is
canonicalized to false before validation. False return plus false output is a
technical failure; true return plus false output is a valid request whose pair
is currently ineligible; true plus true means that Health restoration and
Window cancellation both completed.

Every pointer is required. `revive_health` is a finite, strictly positive
`float` healing delta. Zero, negative, NaN, and infinite values are technical
failures even when the pair is already policy-ineligible. There is no epsilon
or arbitrary minimum. Overheal is allowed and the Health authority clamps it
to the target maximum.

## Authorization and Identity

Execution accepts the same two `HTHPlayerSlot` identities, single roster, and
explicit caller-bound Defeat/Window states as Eligibility. A prior Eligibility
result is only a snapshot: a Player can die or become defeated, a Window can
expire, or membership and component associations can become stale. Execution
therefore calls `hth_player_revive_eligibility_evaluate()` exactly once
immediately before mutation. It accepts no caller authorization boolean or
durable token and duplicates none of Eligibility's predicates.

Technical Eligibility failure becomes Execution failure without mutation.
Current ineligibility is technical success without mutation. Self-revive is
therefore inherited as a valid no-op. After an eligible result, Execution
resolves the target slot once more through PlayerRoster to obtain the current
Entity required by Health; it never reads roster entries directly.

The caller remains responsible for binding Defeat and ReviveWindow values to
their corresponding slots. Execution does not add a lifecycle aggregate or
alter the v0.3.30 Eligibility API.

## Transaction and Authorities

The exact successful order is:

```text
canonicalize output
→ validate pointers and revive_health
→ revalidate Eligibility
→ resolve target through PlayerRoster
→ apply healing through Health
→ reset/cancel target ReviveWindow
→ publish revived=true
```

Healing occurs before Window reset. `hth_health_store_apply_healing()` owns
finite Health validation, arithmetic, and maximum clamping; Execution never
writes `Health.current`. A failed healing call performs no Health mutation and
Execution leaves the Window unchanged. After successful healing,
`hth_player_revive_window_reset()` has no failure channel and canonicalizes the
valid non-null Window to inactive with zero remainder. Prevalidation,
heal-first ordering, and infallible reset are transactionally sufficient, so
no Health snapshot, direct restoration, Window undo, or transaction object is
needed.

Player Death remains derived. Positive target Health makes its authoritative
query report alive; Execution adds no dead/alive mutation. Both Defeat states,
roster membership, Player slots, Entity/Actor identity, reviver Health, and
PlayerBody remain unchanged.

On success, authoritative queries observe target Health greater than zero,
Death false, and an inactive zero-second Window. Repeating the same operation
immediately is a valid no-op: Eligibility now sees an alive target and inactive
Window, and no second healing occurs.

## Isolation and Runtime Boundary

Execution has no PlayerBody, Spatial, distance, Collision, LOS, Input, Camera,
Enemy, interaction timer, animation, audio, HUD, Session, networking,
inventory, item, charge, self-revive capability, or progression dependency.
It owns no state, Store, cache, global, allocation, or synchronization object.
The current synchronous single-threaded model makes its internal validation
and mutations contiguous.

The operation is bounded `O(1)`, uses `O(1)` auxiliary memory, and allocates
nothing. v0.3.31 is disconnected: Engine, Bootstrap, and Interaction own no
Execution state, make no call, and perform no related per-frame work.

Future scheduling retains existing frame-snapshot semantics. Execution before
a Player Death snapshot may affect that frame; execution after it is observed
by movement and targeting on the next frame. This foundation does not mutate
those snapshots or directly re-enable movement or Enemy targeting.

The source-consistent future sequence is Player Lifecycle / Defeat Runtime
Integration, Revive Interaction, and Session Outcome. Interaction may later
gate a completed attempt with distance, Input, hold progress, interruption,
and presentation without redefining this execution boundary.

As of v0.3.32, the Player Lifecycle Runtime owns the target Window and Defeat
binding, but Execution retains zero production callers. Future Interaction
must provide a controlled mutable Window binding and complete authorized
Execution before Engine's authoritative Death snapshot so successful recovery
wins before lifecycle expiry.

As of v0.3.33, disconnected Player Revive Interaction completion reaches this
operation through a private Lifecycle Runtime binding adapter. The adapter
exposes no mutable Window and preserves this operation's final revalidation.
Engine and Bootstrap remain non-callers, so production execution work is zero.

As of v0.3.38, disconnected Player Revive Configuration supplies a prototype
`25.0F` healing amount for future callers. It is not final Health or a
percentage. Execution retains its scalar API and defensive validation, and
Health remains responsible for maximum clamping.

As of v0.3.40, snapshot-aware revive read-side paths deliberately terminate at
this unchanged live-authoritative boundary. A same-frame ordinary heal or
reviver death can make sampled Death differ from current Health; this operation
still performs its two live Death queries before mutation, preventing an
invalid or duplicate revive. Multiple independent revivers therefore retain
first-valid-commit-wins behavior.
