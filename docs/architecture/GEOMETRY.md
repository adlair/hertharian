# Built-in Geometry Library

The internal Geometry library owns immutable CPU descriptions of the built-in
BOX and WEDGE render primitives. It knows no World, Collision, Material,
Resource, Renderer, filesystem, or OpenGL types.

Each view exposes vertices containing only local-space position and UV plus
`uint32_t` triangle indices. Both primitives occupy canonical bounds
`[-0.5, 0.5]` on X, Y, and Z, so Renderer can derive a model matrix by scaling
to object size and translating to object center. BOX uses 24 vertices and 36
indices. WEDGE uses 18 vertices and 24 indices; its inclined face rises toward
positive Z from Y=-0.5 to Y=0.5.

UVs are primitive-local, finite, and limited to `[0,1]`. Faces use simple
independent mappings; seams and consistent artistic orientation are outside
this bootstrap. Geometry validation covers finite values, canonical extents,
index ranges, and nondegenerate triangles.

The OpenGL backend uploads one VAO/VBO/EBO set per primitive during Renderer
initialization. All World draws reuse these two GPU meshes and select one by an
abstract CPU primitive enum. No upload or allocation occurs per frame. Future
external mesh formats may replace or extend this library without moving GPU
handles or file I/O into Geometry.

The internal `HTHGeometryPrimitive` indexing contract is explicit: `BOX == 0`,
and valid primitives form a dense enum in the half-open range
`[0, HTH_GEOMETRY_PRIMITIVE_COUNT)`. `HTH_GEOMETRY_PRIMITIVE_COUNT` is a
sentinel and upper bound, not a primitive. After validating that range, Renderer
may use the primitive ordinal to index its private GPU geometry table; every
lookup must perform this validation first. If the enum stops being dense in the
future, this ordinal-based indexing contract must change.
