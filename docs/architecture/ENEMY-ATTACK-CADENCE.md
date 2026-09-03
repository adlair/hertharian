# Enemy Attack Cadence Foundation

Hertharian v0.3.22 introduces an internal, deterministic countdown primitive
for spacing explicit Enemy attack opportunities. `HTHEnemyAttackCadence` is
caller-owned state containing exactly one scalar:

```c
typedef struct HTHEnemyAttackCadence {
    double remaining_seconds;
} HTHEnemyAttackCadence;
```

It contains no Entity or Enemy handle, target, damage, Health reference,
Store pointer, clock, interval, ready flag, or accumulated attack credit. The
caller owns both the instance and every interval supplied to it.

## Operations and State Contract

`hth_enemy_attack_cadence_reset()` writes the canonical zero state, which is
immediately ready. A null pointer is a harmless no-op. This also provides
explicit recovery from a corrupt negative or non-finite state.

`hth_enemy_attack_cadence_is_ready()` first canonicalizes a supplied output to
`false`. It rejects a null cadence, null output, and negative or non-finite
state without mutating the cadence. Valid state is ready if and only if
`remaining_seconds == 0.0`; readiness uses exact equality and no epsilon.

`hth_enemy_attack_cadence_advance()` accepts only a valid cadence and a finite,
nonnegative caller-owned simulation delta. It subtracts a partial delta and
saturates to exactly zero when the delta equals or exceeds the remaining
countdown. Zero delta is valid. Invalid state or delta returns false without
partial mutation. The function reads no platform or wall clock.

`hth_enemy_attack_cadence_commit()` accepts a finite, nonnegative interval only
while the cadence is exactly ready. It then replaces the zero state with that
interval. A zero interval is valid and leaves the cadence immediately ready.
Recommit while any positive time remains is rejected without mutation.

Fresh state therefore permits a first explicit attack opportunity. The
recommended future transaction order is: query readiness, attempt to build the
attack, and commit only after successful materialization. A failed builder must
not consume cadence. Whether later DamageIntent resolution applies Health is
independent: successful attack materialization may consume cadence even when
explicit resolution reports `applied=false` for an Actor without Health.

## Saturation, Partitions, and No Catch-Up

Advancing beyond the remaining interval produces one ready opportunity, not a
count of missed attacks. Overshoot is discarded; there is no catch-up burst,
attack-credit balance, or retained remainder. After a successful future attack,
the caller explicitly commits the next interval.

The countdown uses `double`, but exact readiness deliberately does not pretend
that all decimal partitions sum exactly in binary floating point. The v0.3.22
tests observe that `60 * (1.0 / 60.0)`, one `1.0`, four `0.25`, eight `0.125`,
and the tested mixed partition saturate to zero. `30 * (1.0 / 30.0)` leaves
`2.0816681711721685e-16` seconds and remains not ready until a later
sufficiently large positive simulation delta saturates it. This is the
deterministic consequence of the approved exact-double contract; no epsilon or
hidden tolerance is introduced.

## Deliberate Boundaries

Cadence does not inspect Enemy Target, Intent, Decision, Perception, LOS,
Attack Eligibility, Spatial, Collision, DamageIntent, damage amount, Health,
or resolution outcome. It does not depend on or mutate Registry, EnemyStore,
or any other Store. Ticking is independent of current intent: a future owner
must advance owned cadence from caller-provided simulation delta according to
that owner's explicit lifecycle rather than making the primitive read intent
or time itself.

The foundation is deliberately disconnected from production in v0.3.22.
Production owns no cadence instance, calls no cadence operation, and performs
no cadence work per frame. Generation-safe association of cadence with a
runtime Enemy and the transaction that integrates Decision, Attack Execution,
commit, and DamageIntent resolution are deferred to v0.3.23.

Every operation is O(1), uses O(1) auxiliary memory, and performs zero
allocations.
