# Gameplay Damage Intent Foundation

Hertharian v0.3.2 represents the statement “source Actor intends to apply this
damage amount to target Actor” as an ephemeral value. Damage Intent is distinct
from resolving current runtime applicability, and both are distinct from the
Health mutation that performs damage arithmetic.

```text
Source Actor → Damage Intent → explicit resolution → target Health
                                                → existing Health damage API
```

Production creates and resolves no Damage Intents in this milestone. The
module is an internal gameplay utility with no Engine state or frame work.

## Representation and Ownership

`HTHDamageIntent` contains only source and target `HTHEntityHandle` values and
a float amount. `HTHDamageResolution` contains an `applied` boolean and the
existing value-type `HTHDamageResult`. Both own nothing, retain no pointers,
allocate no memory, and need no create, destroy, Store, Registry, iterator, or
cleanup API. Their caller owns their lifetime and the runtime Stores supplied
to operations.

There is no ID, timestamp, frame, sequence, consumed flag, state machine,
queue, or persistent memory. An Intent is not one-shot: callers may explicitly
resolve the same unchanged value more than once. Each call independently
observes current Entity, Actor, and Health state, so callers are responsible
for deciding when and how often resolution is requested.

## Validity and Resolvability

For Intent `I`:

```text
Valid(I) =
    Alive(I.source)
    AND Actor(I.source)
    AND Alive(I.target)
    AND Actor(I.target)
    AND finite(I.amount)
    AND I.amount >= 0

Resolvable(I) = Valid(I) AND Health(I.target)
```

Both source and target must therefore be Actors of their exact live Entity
generations. Source Health is irrelevant. Target Health affects resolvability,
not validity: an Actor without Health is still a valid target. Neither Spatial
nor Dynamic Body is required. Self-damage is allowed without a special case.

The amount is passed unchanged after finite, non-negative validation. Negative
damage is not healing. Damage Intent does not evaluate range, orientation,
line of sight, collision, hit detection, teams, damage types, weapons, or
mitigation; any future producer decides upstream whether an Intent should
exist.

Destroying either Entity permanently invalidates the old generational handle.
Reusing the same numeric index does not rebind an old Intent to the replacement
Entity. Removing a same-generation Actor makes the Intent invalid; reattaching
it can make the unchanged Intent valid again. Removing target Health changes a
valid Intent from resolvable to non-resolvable; attaching Health can restore
resolvability.

Handles are meaningful only with the matching caller-supplied Entity Registry,
Actor Store, and Health Store. Correct Store pairing remains a caller contract;
Damage Intent introduces no Registry UUID or cross-Registry identity.

## Explicit Resolution

Validation is pure and never resolves or mutates Health. Resolution is the only
Damage Intent operation that may cause mutation, and its matrix is:

```text
!Valid
    → return false, canonical-zero result, no mutation

Valid AND !Health(target)
    → return true, applied=false, zero damage result, no mutation

Valid AND Health(target)
    → delegate once to hth_health_store_apply_damage
    → on successful delegation return true, applied=true
```

`resolution.applied` means the valid Intent reached a valid target Health
association and the Health damage operation succeeded. It does not mean a
non-zero numeric amount was applied. Amount zero with Health yields
`applied=true` and `damage.applied=0`; a target already at zero behaves the
same and does not repeat `became_zero`. Without target Health, `applied=false`.

Resolution requires non-null Intent, Registry, Actor Store, Health Store, and
result output. Invalid inputs return false after writing a canonical-zero
result when output storage is supplied. A valid but non-resolvable Intent is a
successfully processed request, not invalid input. Only after successful
delegation is `applied` set true.

Health remains the sole authority for saturation, applied amount, and
`became_zero`. Damage Intent neither accesses Health storage nor duplicates its
arithmetic. Lethal resolution leaves Entity, Actor, Health, Spatial, and Body
lifecycle untouched.

The current Engine is single-threaded. No lock or atomic claim is introduced
around runtime validation and Health delegation.

## Complexity and Scope

Validation and resolution are O(1); resolution delegates to the existing O(1)
Health operation. Allocation is zero and persistent memory is zero. There is
no iteration, automatic processing, initialization, shutdown, or per-frame
work.

Deferred scope includes a generic GameplayAction, queues, events, combat or
attack systems, weapons, projectiles, damage types, instigator metadata beyond
the source Actor handle, teams/factions, range or line-of-sight checks,
hit/miss rules, armor/resistance, Player/Enemy integration, Level declarations,
persistence, networking, scripting, and an ECS.
