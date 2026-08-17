# Health / Damage Foundation

Hertharian v0.3.1 represents Health as optional gameplay state associated with
a live Actor Entity. It supplies deterministic damage and healing arithmetic;
it does not implement combat, death behavior, or gameplay specializations.

The architectural boundary is:

```text
Entity lifetime != Actor participation != Health state
                != Spatial state != Dynamic Body state
```

`HasHealth(H)` implies `HasActor(H)`, but an Actor need not have Health. Health
owns none of those other states and introduces no identity or lifetime
authority.

## Representation and Invariants

`HTHHealth` contains only `float current` and `float maximum`. A valid value
has finite fields, `maximum > 0`, and `0 <= current <= maximum`. Zero current
is valid gameplay state, not a hidden death flag. Maximum is fixed for the
lifetime of an attached value: v0.3.1 deliberately exposes no setter for it or
for arbitrary current Health.

The private Store is a generation-keyed sparse array. It starts with 64 absent
entries and doubles safely when needed. Growth validates index and allocation
arithmetic, uses transactional `realloc`, preserves existing values, and
initializes all new entries. Allocation occurs only at Store creation or
growth, never per operation or per frame.

Attach is O(1) amortized; has, get, remove, apply-damage, and apply-healing are
O(1). Full iteration is O(capacity). No operation allocates while existing
Store capacity is sufficient: Store creation and growth are the only
allocation points.

Attach requires a valid Health value, a live Entity, and a matching Actor.
Duplicate attach for the same generation fails. Get is copy-out and writes
canonical zero on failure. Remove clears Health only; it does not remove Actor,
Spatial, Dynamic Body, or Entity state.

Entity destruction makes a retained entry semantically absent. Reuse of its
index cannot inherit Health because the generation differs, and stale handles
cannot get, remove, damage, or heal the replacement. The Store retains no
Registry or Actor Store pointer: consistently pairing those authorities with
the Store is a caller contract.

Removing Actor temporarily makes same-generation Health semantically absent.
Reattaching Actor to that still-live Entity reveals the retained Health again.
This is deliberate non-cascading composition, not implicit Health removal.

## Damage and Healing

Damage accepts only a finite, non-negative amount. It computes:

```text
current' = amount >= current ? 0 : current - amount
applied  = current - current'
```

The result reports previous, current, applied, and `became_zero`.
`became_zero` is true only for a transition from positive current Health to
zero. Zero damage is valid; damage to already-zero Health applies zero and
does not repeat the transition. Exact lethal damage and overkill both saturate
at zero.

Healing also accepts only a finite, non-negative amount. It computes remaining
capacity first, then applies no more than that capacity. This avoids relying on
an unchecked `current + amount` that could overflow. The result reports
previous, current, and applied. Healing saturates at maximum and healing from
zero is permitted. Neither operation changes maximum. Invalid and stale calls
perform no mutation. A failed damage call deterministically writes a canonical
zero `HTHDamageResult` when an output is supplied according to the API
contract; failed healing does the same with `HTHHealingResult`.

## Iteration, Engine Ownership, and Scope

The value iterator scans entries in ascending Entity index and yields copies
of each handle and current Health whose Entity and Actor are both still valid
for the stored generation. It skips absent, dead, stale, and currently
non-Actor entries, performs no allocation, and stores no external pointer.
Entity, Actor, or Health mutation during iteration is unsupported.

Each Engine owns one private empty Health Store, initialized after Actor and
destroyed before Actor. Production attaches no Health and performs no Health
work per frame. Level v2 declares no Health, static World is unaffected, and
the existing Player is not migrated.

As of v0.3.2, the higher-level Damage Intent resolver may invoke Health's
existing damage operation after validating source and target Actor identity.
Health remains unaware of the source and continues to own only Health
arithmetic; Damage Intent adds no Health state or lifecycle coupling.

Deferred scope includes damage sources/types, armor, resistance,
invulnerability, regeneration, death events or systems, Player migration,
Enemy/Pickup/Projectile types, Level declarations, persistence, networking,
scripting, and a generic ECS.
