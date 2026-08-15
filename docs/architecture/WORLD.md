# World Representation Foundation

`HTHWorld` is the engine-internal source of truth for static world content. It
is built once, finalized before use, and then read as immutable data. Engine
owns it and destroys it only after its consumers.

## Static Objects

As of v0.2.5 each static object contains shared world-space `HTHAABB` bounds,
an explicit collision shape, an explicit render shape, flags, and a bootstrap
visual class. Collision supports `none` or `aabb`; rendering supports `none`,
`box`, or `wedge`. The common bounds define canonical extents: Collision may
interpret them as an AABB while Renderer scales the selected unit primitive
into the same volume.

Flags and shapes must agree. A collidable object has collision shape `aabb`,
and a non-collidable object has `none`. A visible object has `box` or `wedge`,
and a non-visible object has render shape `none`. World validates these rules,
enum ranges, bounds, flags, visual classes, spawn state, and aggregate bounds
during construction/finalization.

World bounds remain the union of every valid static object's bounds,
independent of flags or shapes. `HTHAABB` remains geometry-only and contains no
render metadata, color, material, or GPU handle.

## Consumer Boundaries

```text
Level v2 → World builder/finalize → finalized HTHWorld
                                      ├── collision state → CollisionWorld
                                      └── render state → Engine draw extraction
                                                            ├── Geometry
                                                            └── Material
```

Collision copies only objects marked collidable with `aabb` collision shape.
It ignores render shape and visual class. Engine extracts visible objects from
render shape, bounds, and visual class; Renderer receives no collision shape.
Visible-only and collision-only objects are both valid.

The bootstrap level contains the ten historical collidable/visible boxes plus
one visible-only wedge. The wedge intentionally has no collision and exists to
demonstrate that visible geometry no longer implies collision geometry.

World contains no persistence, parser, Resource IDs, renderer handles,
OpenGL, entities, dynamic objects, scene graph, BSP, or gameplay object model.
The current collision implementation remains AABB-only and the current render
shapes remain built-in bootstrap primitives.
