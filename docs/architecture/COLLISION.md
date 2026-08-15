# Collision Foundation

v0.1.8 retained the small static AABB collision world but replaced the
v0.1.7 X/Y/Z resolver with swept AABB traces as the sole Player Movement path.
Movement submits an origin, destination, and local volume to Collision; it does
not iterate or resolve `CollisionWorld` obstacle storage. Trace details and the
result contract are documented in `COLLISION-TRACES.md`.

As of v0.1.9, Locomotion owns ground friction, directional ground/air
acceleration, gravity, and jump. Collision owns none of that policy: it only
constrains generated velocity and movement geometrically, and its clipped
velocity remains the Player Body's physical truth.

## Slide Movement

Player Movement traces `velocity * remaining_time`, moves to the earliest
impact, collects its normal, and removes the velocity component entering that
plane. Up to four impacts are processed per frame. Nearly identical normals
are not added twice. Two non-parallel planes restrict velocity to their
cross-product crease; a third incompatible plane stops movement. A `1e-5`
Movement-owned surface offset keeps the body outside a contacted face without
altering the trace fraction.

The previous discrete axis-separated solver was removed. Sweeping prevents a
queried static AABB from being skipped by a large displacement, so discrete
tunneling is no longer a limitation of the active path.

## Ground and Steps

A 0.04-unit downward AABB probe recognizes only the exact +Y normal as ground;
v0.1.8 has no slope walkability policy. Supported downward velocity becomes
zero. A player that began grounded can probe as far as the 0.30-unit step
height plus ground-probe distance after horizontal movement, providing explicit
step-down. Larger drops do not snap and become airborne.

Step-up is attempted only when a grounded movement was blocked horizontally:

1. trace upward by the single 0.30-unit step height;
2. use the same trace/slide routine for elevated horizontal movement;
3. trace down by step height plus ground-probe distance;
4. require a +Y support normal and greater horizontal progress than the normal
   slide result.

Any blocked clearance, missing support, airborne start, high ledge, or
non-improving result rejects the candidate. Horizontal collision itself never
changes body height. Obstacles at or below 0.30 units can be stepped; taller
ledges block. Camera smoothing is deliberately absent, so view height follows
the physical step directly.

## Bootstrap World and Limits

As of v0.2.5, Collision builds its private fixed-capacity array by copying only
finalized World objects that are marked collidable and explicitly select AABB
collision. It consumes only shared bounds, collision shape, and the collidable
flag; it ignores render shape, visual class, materials, and geometry
primitives. Trace obstacle indices identify that Collision-owned copy and are
not stable World-object identifiers.

The bootstrap content contains a floor, long walls and corridor, an
inside corner, a box, a 0.20-unit low step, an exact 0.30-unit platform, and a
0.60-unit high ledge. The physical bounds are unchanged from v0.1.8. The
additional diagnostic wedge is visible-only and never enters this array.

Collision remains axis-aligned and static. There is no general depenetration,
rotation, capsule, slope, stair flight, dynamic body, moving platform,
broadphase, BSP, or external physics middleware. A start-solid trace stops
Player Movement safely but does not choose an arbitrary escape direction.
