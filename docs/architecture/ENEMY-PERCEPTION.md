# Enemy Perception Foundation

Hertharian v0.3.5 provides one pure, stateless spatial query that asks whether
a current Enemy can perceive an arbitrary candidate Entity inside a
caller-supplied radius. Perception has no Store, allocation, cache, Engine
ownership, persistent fields, global mutable state, or per-frame work.

The observer must be a live Actor with current Enemy and Spatial associations.
The candidate needs only a live Entity generation and Spatial association; it
need not be an Actor, Enemy, Player, Dynamic Body, or Health owner. The current
Enemy Target relationship is neither accepted nor consulted by this query.

## Radius and Geometry

Radius is valid only when finite and nonnegative. Zero is valid and perceives
only candidates at exactly zero distance. The query measures full Euclidean 3D
distance, including vertical separation, and includes the exact spherical
boundary:

```text
dx*dx + dy*dy + dz*dz <= radius*radius
```

Each finite float coordinate is converted to `double` before subtraction, and
all differences and squared arithmetic use `double`. This prevents overflow in
the intermediate domain across finite float coordinates without requiring a
square root.

Yaw does not affect the result. There is no forward vector, view cone, field of
view, line-of-sight trace, collision query, hearing, memory, timeout, or target
priority.

## Purity and Deferred Scope

The query modifies no Entity, Actor, Enemy, Spatial, Target, Body, or Health
state. Repeated calls over unchanged Stores return the same result. It neither
sets nor clears a target and never performs automatic acquisition. Production
makes zero perception calls in v0.3.5.

FOV, LOS, hearing, perception memory, target selection, AI, locomotion, combat,
Enemy definitions or stats, persistence, networking, scripting, and generic
query/ECS infrastructure remain deferred.

As of v0.3.7, radius Perception and static-world Enemy LOS remain independent
queries; neither result implies or mutates the other.
