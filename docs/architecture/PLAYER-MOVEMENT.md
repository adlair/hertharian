# Player Movement

The W/S/A/D spatial convention from v0.1.7 is unchanged. W/S use camera forward
projected onto XZ; A/D use `horizontal_forward × up`; pitch is ignored and
combined intent is normalized. v0.1.9 also retains wish magnitude and consumes
Space's pressed transition for jump. Movement intent remains enabled only
during FPS pointer capture, and focus loss clears held Input state.

The direct horizontal-velocity assignment from v0.1.8 is replaced by the
ground/air acceleration model documented in `LOCOMOTION.md`. Player Movement
consumes a per-instance MovementConfig, generates velocity through friction,
directional acceleration, jump and gravity, then passes that velocity to the
unchanged collision-resolution responsibilities below. Existing world-space
momentum is not rotated with Camera yaw or globally clamped to ground speed.

## Trace-Based Resolution

Player Movement converts Body dimensions to feet-origin trace extents and uses
only the Collision Trace API. The old direct X/Y/Z resolver is removed.
Velocity drives a swept trace for remaining frame time. At impact, pure plane
projection removes the incoming component:

```text
clipped = velocity - normal * dot(velocity, normal)
```

The solver handles up to four impacts with temporary, stack-owned normals.
One plane preserves tangential motion, two planes can constrain motion to their
crease, and three incompatible planes stop it. Collision state is not retained
in Player Body.

A 0.04-unit ground probe establishes final support and clears downward
velocity. Grounded movement may explicitly step up or down by at most 0.30
units. Step-up requires upward clearance, trace-based horizontal progress,
valid +Y support, and more horizontal progress than ordinary slide. Step-down
snaps only a previously grounded body within that range. Airborne bodies never
step, and larger drops fall under gravity. An accepted jump is airborne
immediately, skips ground snap while ascending, and cannot attempt step-up or
step-down that frame.

This remains local variable-delta bootstrap movement, not a fixed simulation
tick or a claim of network determinism. It has no advanced Quake air control,
walkable slopes, crouch, sprint, or gameplay binding layer.
