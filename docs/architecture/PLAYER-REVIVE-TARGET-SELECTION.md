# Player Revive Target Selection Foundation

Hertharian v0.3.36 adds a private, disconnected query that selects the nearest
currently revivable Player. It is lifecycle-bound, stateless, and pure: the
caller supplies a current reviver `HTHPlayerSlot` and a revive range, and the
query returns either a current target `HTHPlayerSlot` or the canonical
`HTH_PLAYER_SLOT_INVALID`. Engine has no caller and production still contains
exactly one Player.

## Result and Failure Contract

The output is canonicalized to `HTH_PLAYER_SLOT_INVALID` before any validation
that can fail. A valid query with no target returns true plus INVALID; a found
target returns true plus its PlayerSlot. Null dependencies, an invalid, empty,
stale, or otherwise non-current reviver, missing reviver Health or Spatial,
malformed current roster membership, and other technical failures return false
with INVALID preserved.

A technically current reviver that is dead or defeated is policy-ineligible,
not structurally invalid. The query therefore succeeds with no target. Empty
roster slots are normal and skipped. Full Entity handles are resolved from the
current roster on each call, so sparse membership and generation-safe slot
reuse are supported without caching Entity identity.

## Eligibility and Spatial Ordering

For each current non-self candidate, the selector calls
`hth_player_lifecycle_runtime_can_revive`. This Lifecycle adapter and the
released Player Revive Eligibility foundation remain the sole policy authority
for reviver death/defeat and candidate death/defeat/window state. The selector
does not duplicate those rules. A technical `can_revive` failure aborts the
whole query; a valid ineligible result merely omits that candidate.

Eligibility is evaluated before candidate Spatial. Consequently an alive,
defeated, or inactive-window candidate does not require Spatial. A candidate
that passed Eligibility must have valid current Spatial or the query fails,
because it cannot be ranked. Reviver Spatial is resolved exactly once.

In a four-current-Player query, self is skipped and at most three Eligibility
calls occur. Each call currently performs two Player Death queries, so the
maximum is six Death queries. That bounded multiplicity is acceptable for this
disconnected foundation, but is a production-integration concern. A future
snapshot-aware cooperative runtime must avoid conflicting with Engine's
authoritative once-per-frame Player Death snapshot before adding a consumer.

## Range and Ranking

The caller owns `revive_range`; the selector defines no default. It accepts
exactly finite, strictly positive values, matching Player Revive Interaction.
Target distance is the full world-space Euclidean 3D distance between current
Spatial positions. Coordinates are promoted to `double` before subtraction,
then squared distance and squared range are calculated in `double`:

```text
dx = (double)target.x - (double)reviver.x
dy = (double)target.y - (double)reviver.y
dz = (double)target.z - (double)reviver.z
distance_squared = dx*dx + dy*dy + dz*dz
range_squared = (double)revive_range * (double)revive_range
```

Non-finite numerical results are technical failures. The comparison is
inclusive, so an exact-boundary or co-located target qualifies. No square root,
epsilon, normalization, or horizontal-only shortcut is used.

The nearest eligible in-range Player wins. An exact distance tie explicitly
selects the lower PlayerSlot, independent of Entity index. Every call ranks
current state anew: a nearer target replaces the previous result immediately,
multiple revivers may independently select the same target, and there is no
target memory, reservation, lock, hysteresis, randomness, or allocation.

## Frozen Boundaries and Deferred Composition

Selection reads Lifecycle Runtime, Entity, Actor, Health, and Spatial
authorities only. It does not receive Interaction state, held input, hold
duration, revive Health, delta time, Camera, facing, FOV, LOS, Collision,
PlayerBody, Movement, Enemy state, Renderer, Timing, or networking. It performs
no healing, Window reset, Defeat change, interaction progress, execution, or
other mutation. Complexity is `O(P)` with `P <= HTH_MAX_PLAYERS == 4`, `O(1)`
auxiliary memory, and zero heap allocation.

Future orchestration may feed a selected valid slot into Player Revive
Interaction. It must not blindly feed the normal no-target result
`HTH_PLAYER_SLOT_INVALID` into a held Interaction step because the current
v0.3.33 Interaction contract treats that held candidate as invalid input. That
boundary and the snapshot-aware ordering must be designed before production
integration; v0.3.36 changes neither Interaction nor Execution.

The disconnected v0.3.37 Semantic Interact Input foundation may later provide
the general Interact held state to such orchestration, but it does not invoke
selection or Revive. A future router must cancel/reset when Interact is not
down and must handle this query's successful INVALID result instead of passing
it blindly to held Interaction.

As of v0.3.38, disconnected Player Revive Configuration supplies one prototype
range value for future Selection and Interaction callers. Selection retains
its scalar range API, current-value semantics, and defensive validation.

As of v0.3.40, a private snapshot-aware sibling uses the current exact Roster
identities and snapshot-aware Eligibility without performing live Death
queries. Legacy and snapshot paths share one candidate loop and therefore one
range, distance, nearest-target, and lower-PlayerSlot tie authority. Spatial
remains live. A snapshot mismatch for any current identity is a technical
failure, not a skipped candidate. This path remains disconnected from Engine.
