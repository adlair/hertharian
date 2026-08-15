# HTH Level Format Version 2

`.hthlevel` is the minimal text representation used to persist the current
static `HTHWorld`. Version 2 separates collision and render shape selection.
It is independent of engine version 0.2.6 and material format version 1.
Version 1 is historical to v0.2.3/v0.2.4 and is no longer accepted; there is
no compatibility parser or automatic upgrade.

## Encoding and Grammar

Tokens are ASCII. Spaces, tabs, LF, CRLF, bare CR, blank lines, and `#`
comments are accepted. A final newline is optional. BOM and embedded NUL are
errors. Order is strict:

```text
file          := header spawn static_object* EOF
header        := "hthlevel" "2"
spawn         := "spawn" float float float float
static_object := "static_object"
                 "bounds" float float float float float float
                 "collision" collision_shape
                 "render" render_shape
                 "flags" flag_form
                 "visual" visual_class
                 "end"
collision_shape := "none" | "aabb"
render_shape    := "none" | "box" | "wedge"
```

Unknown, duplicate, reordered, omitted, or trailing tokens fail. There is no
numeric enum syntax and no fallback from an unknown shape to AABB or box.

## Bounds, Shapes, and Flags

`bounds min_x min_y min_z max_x max_y max_z` supplies the common extents used
by both representations. World remains authoritative for finite, nondegenerate
bounds and all object invariants.

Canonical flag forms remain `none`, `visible`, `collidable`, and `collidable
visible`. They must agree with shapes:

```text
flags collidable visible → collision aabb, render box|wedge
flags visible            → collision none, render box|wedge
flags collidable         → collision aabb, render none
flags none               → collision none, render none
```

The diagnostic visual tokens remain `none`, `floor`, `wall`, `box`,
`low_step`, `limit_step`, `high_ledge`, `platform`, `corner`, and
`corridor_corner`. They select the existing external bootstrap material bridge
and are independent of render primitive choice.

## Example

```text
hthlevel 2
spawn 0 0.05 3 0

static_object
bounds -4.5 0 -11 -2.5 1.5 -9
collision none
render wedge
flags visible
visual box
end
```

This object is intentionally visible and pass-through. Changing `render
wedge` to `render box` changes its primitive on the next process start without
rebuilding.

## Parsing and Current Scope

Floats retain the strict finite locale-independent decimal contract. The
parser consumes a byte pointer plus size, retains no token pointers, rejects
embedded NUL, and publishes output transactionally with one-based diagnostics.
The Level-to-World builder performs a second authoritative validation.

Version 2 has no object rotation, custom UVs, material IDs, external meshes,
entities, dynamic objects, BSP, slopes, triangle collision, lights, scripts,
or serialization. Logical Level identity is external runtime state and is not
encoded in `hthlevel 2`.
