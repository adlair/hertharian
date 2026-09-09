# ADR-0050: Exclude the Dead Player at the Bootstrap Targeting Boundary

- Status: Accepted
- Milestone: v0.3.26

## Decision

DEAD PLAYER IS NOT A VALID ENEMY TARGET CANDIDATE.

CURRENT TARGET TO DEAD PLAYER MUST BE CLEARED.

PLAYER TARGETABILITY SNAPSHOT ONCE PER FRAME.

ENGINE/BOOTSTRAP CANDIDATE POLICY.

ENEMYTARGET STORE REMAINS RELATION-ONLY.

DECISION REMAINS UNAWARE OF PLAYER DEATH.

ATTACK ELIGIBILITY REMAINS UNAWARE OF PLAYER DEATH.

ATTACK EXECUTION REMAINS UNAWARE OF PLAYER DEATH.

HEALED PLAYER AUTOMATICALLY BECOMES TARGETABLE AGAIN.

TARGET DEATH DOES NOT RESET ENEMY CADENCE.

Engine reuses its single frame-local `player_dead` result. Bootstrap combines
that snapshot with the stable Player Target Bridge handle to produce one generic
`excluded_target`. Pursuit clears a matching Current Target after cadence
advance and before Spatial early-out. Selection skips the excluded handle before
Perception, LOS, and ranking. An invalid exclusion preserves historical
behavior, and full handle equality preserves generation safety.

An alive snapshot applies to every Enemy in the frame even if an early Enemy
deals lethal damage. Exclusion starts next frame. Healing removes the exclusion
when observed by the next single snapshot; the same Player Entity can then be
reacquired without restoration callbacks or cadence reset.

## Rejected Alternatives

- Health checks or Player Death queries in Enemy Target, Selection, Pursuit,
  Decision, Eligibility, Attack Execution, or DamageIntent would duplicate the
  snapshot and spread Player policy through generic foundations.
- Filtering only Bootstrap's candidate count would leave an existing valid
  Current Target retained.
- EnemyTarget auto-clear on Health changes would make a relation Store own
  gameplay policy.
- A Decision death branch would idle without permitting normal replacement.
- Mid-frame refresh would make Enemies use different targetability snapshots.
- DamageIntent callbacks, Player Bridge destruction, generic Actor death,
  callbacks, policy objects, and filter-list frameworks exceed this milestone.

## Consequences

The Player remains structurally present but semantically untargetable while the
snapshot is dead. Every Enemy clears that exact relation during its existing
iteration; alternatives remain selectable. Cadence remains Enemy-owned and
continuous. The policy adds no allocation or scan and requires no public API.

## v0.3.35 Amendment

ADR-0059 generalizes the private singleton value to a caller-owned pointer and
count of generic Entity handles. The original zero/one Bootstrap behavior and
Death authority remain unchanged; Selection and Pursuit can now apply the same
policy to multiple handles without importing Player lifecycle state.
