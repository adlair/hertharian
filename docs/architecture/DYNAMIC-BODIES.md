# Dynamic Body Foundation

Hertharian v0.2.9 adds optional runtime Dynamic Body state for live Entities.
Entity continues to own identity and lifetime, Spatial owns world-space
position and yaw, and Dynamic Body owns only an axis-aligned shape and linear
velocity. A Body is not a gameplay Actor, Player representation, renderable,
or general rigid body.

## Representation and Ownership

`HTHDynamicBody` contains an `HTHVec3 half_extents` and an `HTHVec3 velocity`.
It contains no position, previous position, cached world bounds, orientation,
mass, force, acceleration, grounded state, or angular state. Spatial position
is the authoritative center of the Body AABB:

```text
minimum = Spatial.position - half_extents
maximum = Spatial.position + half_extents
```

Every half-extent must be finite and strictly positive. Every velocity
component must be finite; zero velocity is valid. Spatial yaw does not rotate
the AABB. Shape changes are deliberately remove-then-attach operations; v0.2.9
exposes only velocity mutation after attach.

## Association Lifetime

The Entity Registry remains the lifetime authority. The private
`HTHDynamicBodyStore` is indexed by Entity index, and each present entry stores
the owning generation. It retains no Registry or Spatial pointer. For a handle
`H`, Body association exists exactly when the Entity is alive, its index is in
capacity, the entry is present, and the generations match. This predicate does
not require Spatial.

Initial attach does require a live Entity with Spatial, so the Body begins with
a valid world location. Removing Spatial later does not remove the Body: its
shape and velocity remain available and iterable, but it cannot simulate.
Reattaching Spatial to the same Entity generation makes that same Body
simulatable again. Entity destruction hides stale Body storage immediately,
and a reused index does not inherit it; explicit attach replaces the stale
entry.

## Store Operations and Iteration

The Store begins with 64 absent entries and grows by safe doubling. Allocation
occurs only at Store creation or growth, never once per Body. Attach rejects a
duplicate current association. Get copies out and zeroes valid output storage
on failure. Velocity updates validate before writeback. Remove clears Body
state without changing Entity or Spatial lifetime.

The value iterator yields live, generation-matching Bodies in ascending Entity
index, including Bodies whose Spatial state is temporarily absent. It never
returns backing pointers. Entity or Body mutation during iteration is
unsupported. A Store must be paired with the Registry whose handles populated
it; Registry identity itself is not encoded.

The current foundation has no gravity, grounded state, friction, restitution,
drag, broadphase, Dynamic-vs-Dynamic collision, triggers, layers, persistence,
thread safety, rendering, gameplay, Level syntax, ECS framework, or per-frame
automatic simulation.
