# World Representation Foundation

v0.2.1 introduces `HTHWorld` as the engine-internal source of truth for static
bootstrap-world content. A World is built once, finalized before use, and then
read as immutable data for the remainder of its lifetime. It is owned by the
Engine and destroyed only after its consumers.

## Static Objects

Each static object contains:

- an `HTHAABB` with its physical world-space bounds;
- independent `collidable` and `visible` flags;
- a small bootstrap diagnostic visual class.

The flags are deliberately independent. A visible-only object does not enter
Collision, while a collidable-only object does not enter Renderer. `HTHAABB`
remains pure geometry and carries no color, rendering handle, or gameplay
classification. The visual class is descriptive metadata, not RGBA or a
Material system.

World storage grows dynamically only while building. Finalization validates
the contents, computes aggregate bounds, and closes all mutation APIs. Read
access is index-based and const. Empty finalized worlds are valid, although
the current Collision bootstrap requires at least one collidable object.

## Bootstrap Content

The current builder reproduces the pre-v0.2.1 physical scene exactly: floor,
walls/corridor, inside corner, generic box, 0.20 low step, exact 0.30 step,
0.60 high ledge, platform/drop reference, and corridor-corner reference. It
also defines the default player spawn at `(0, 0.05, 3)` with yaw `0` radians.

The Engine reads this spawn instead of embedding it in lifecycle code. It then
builds Collision from the collidable subset and verifies the Player Body does
not start solid.

## Consumer Boundaries

```text
Bootstrap builder → finalized HTHWorld
                         ├── collidable objects → CollisionWorld copy
                         └── visible objects → Renderer frontend draw data
```

Collision copies only AABBs and owns its trace-local obstacle indices. They do
not promise stable World-object identity. Renderer frontend maps diagnostic
visual classes to the existing bootstrap colors and sends only derived bounds
and colors to the OpenGL backend. The backend includes no World or Collision
types.

This milestone does not introduce map loading, entities, dynamic objects,
scene graphs, materials, textures, assets, BSP data, serialization, or a
gameplay object model. Future World/Scene/Material work can replace the
bootstrap visual metadata without changing the current collision geometry.
The World abstraction is the ownership/content boundary. The AABB primitive is
the current bootstrap representation, not a promise that future loaded worlds
will be lists of AABBs; a future loader or BSP-backed representation can target
the same boundary.
