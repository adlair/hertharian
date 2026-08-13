# Collision Foundation

v0.1.7 uses a small engine-owned static collision world made from fixed AABBs.
The bootstrap set contains a floor, left/right walls, a central obstacle, and
two boxes. The renderer receives the same bounds and draws simple boxes, so
manual collision references are visible without introducing Scene, Mesh,
Entity, Material, map, or world-format systems.

The player AABB uses its feet-centered position, half-width, and height as
documented in `PLAYER-BODY.md`. AABBs that share exactly one face are touching,
not penetrating. Initial spawn is validated for penetration; there is no
general depenetration solver for invalid spawns.

Movement is resolved discretely in X, then Y, then Z. Each axis moves
independently and brute-force checks the small static list. A blocked horizontal
axis is corrected to the obstacle face and its velocity component becomes
zero; the other horizontal axis can continue, providing basic wall sliding.
Descending vertical collision places the feet at the obstacle top, clears
vertical velocity, and marks the body grounded only when the previous bottom
was on or above that top and the vertical step crosses it from above. Upward
collision similarly requires the previous top to be on or below the ceiling
face before placing the body below it. A lateral overlap therefore cannot be
reinterpreted as vertical support.

v0.1.7 has no step-up. Horizontal collision never promotes Player Body onto an
obstacle; top-surface landing requires a vertical approach from above. Step
movement remains a later milestone.

X/Y/Z is a simple, documented bootstrap order, not a claim of order-independent
physics. The solver has no swept tests and can tunnel if a discrete displacement
crosses an entire thin obstacle. The 4-unit movement speed, 0.1-second local dt
cap, and substantial bootstrap obstacles bound this limitation for v0.1.7.
There is no capsule, slope handling, stair/step movement, BSP, broadphase,
raycast subsystem, rigid body simulation, or external physics middleware.
