# World Geometry Separation

v0.2.5 breaks the bootstrap equivalence between collision and rendered
geometry. A static object owns one canonical AABB of world-space extents but
selects collision and render representations independently.

```text
HTHWorldStaticObject
├── bounds + collision shape + collidable flag → CollisionWorld
└── bounds + render shape + visible flag → Geometry/Renderer
                         visual class → Material/Texture
```

Collision shape is `none` or `aabb`; render shape is `none`, `box`, or
`wedge`. The collidable and visible flags must agree exactly with their
respective shapes. Collision ignores render shape and visual class. Render
extraction ignores collision shape. Aggregate World bounds union every object,
including visible-only and collision-only objects.

The bootstrap diagnostic wedge is visible-only and intentionally pass-through.
It proves the separation without adding slope or triangle collision. A
collision-only object is supported and tested but is not placed in the
bootstrap level.

Current limits include AABB-only collision, built-in box/wedge rendering, no
rotation, no custom UVs, no external meshes or BSP, the existing 16-obstacle
Collision capacity, and the existing non-relocatable development Resource
Root.
