# Spatial Entity State Foundation

Hertharian v0.2.8 adds optional world-space state for live runtime Entities.
Spatial state is separate from Entity identity: an Entity may be alive without
a Spatial association, and attaching or removing Spatial state never creates
or destroys the Entity.

## Transform and Coordinates

`HTHSpatialTransform` contains only an `HTHVec3` world-space position and a
world-space yaw. Coordinates follow the existing convention: X is horizontal,
Y is vertical, and Z is depth. Yaw is a `float` in radians around the world Y
axis. Any finite yaw is stored exactly as supplied; the Store does not
normalize it. Attach and set reject NaN or infinity in any position component
or yaw.

There is no pitch, roll, scale, matrix, quaternion, hierarchy, local transform,
velocity, interpolation, dirty flag, or legality check against World or
Collision. Spatial state records a pose; it does not define motion, physics,
rendering, or gameplay.

## Store and Lifetime Validation

Each Engine owns one private `HTHSpatialStore` beside its Entity Registry. The
Store owns a dynamically sized array keyed directly by Entity index. An entry
contains a presence bit, the generation of the Entity lifetime that attached
it, and a transform copy. The initial capacity is 64 entries and grows by safe
doubling to cover an incoming live Entity index. Allocation occurs only when
the Store is created or grows, never once per Spatial association.

The Entity Registry remains the sole lifetime authority. Every operation
receives the Registry explicitly; the Store neither retains nor owns its
pointer and never creates, destroys, or changes an Entity. A Store must always
be used with the Registry whose identities populated it. Registry identity is
not encoded, so passing a different Registry with numerically coincident
handles is outside the contract.

For handle `H = (index, generation)`, Spatial state exists exactly when:

```text
Entity Registry reports H alive
and index is within Store capacity
and entry[index] is present
and entry[index].generation == H.generation
```

Destroying an Entity therefore hides its Spatial state immediately even though
the backing entry may remain physically present. If the index is reused, the
new generation cannot inherit that stale transform. A later explicit attach
for the new Entity replaces the stale entry. There is no observer, callback,
periodic pruning, garbage collection, or raw occupied-entry count.

## Operations

- Attach requires a live Entity, a finite transform, and no matching current
  association. Duplicate attach fails without overwriting; set is the only
  update operation. A stale entry from an older generation may be replaced.
- Has is a pure application of the lifetime/generation predicate.
- Get copies the transform out and never returns a backing-store pointer. On
  failure, valid output storage receives the deterministic zero transform.
- Set requires an existing matching association and a finite transform. It
  never implicitly attaches, and invalid input leaves the old value intact.
- Remove clears the matching entry deterministically but leaves the Entity
  alive. Dead and stale handles cannot use remove as storage cleanup.

Attach, has, get, set, and remove are O(1) amortized; occasional growth is
O(capacity). Store memory is O(capacity).

## Iteration and Engine Boundary

The value iterator scans entries in ascending Entity index and copies out both
the current handle and transform. It consults the Registry and skips absent,
dead, and stale-generation entries without mutating them. On failure or
exhaustion, valid outputs receive the invalid Entity handle and zero transform.
Entity create/destroy and Spatial attach/set/remove are unsupported while an
iteration is active; no mutation-safe traversal is promised.

Engine creates the empty Store after the empty Entity Registry and destroys it
before that Registry and the static World. Production creates zero Entities
and zero Spatial associations, performs no per-frame Spatial work, and emits no
new Spatial logging.

The current Player remains separate. Spatial state is internal, runtime-only,
single-threaded, and nonpersistent. It is not an ECS component framework and
has no public API, Level syntax, serialization, physics, collision, rendering,
gameplay, events, callbacks, or filesystem/platform dependency.
