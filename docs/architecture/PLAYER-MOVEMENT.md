# Player Movement

The W/S/A/D intent contract from v0.1.7 is unchanged. W/S use camera forward
projected onto XZ; A/D use `horizontal_forward × up`; pitch is ignored and
combined intent is normalized. Movement remains enabled only during FPS pointer
capture, and focus loss clears held Input state.

Horizontal velocity is still assigned directly at 4 world units per second,
including while airborne, and becomes zero without intent. There is no
acceleration, friction, air-acceleration, or jump. Gravity remains -9.81 world
units per second squared with semi-implicit Euler, and only movement clamps a
frame delta above 0.1 seconds.

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
step, and larger drops fall under gravity.

This remains local variable-delta bootstrap movement, not a fixed simulation
tick or a claim of network determinism. It has no Quake movement dynamics,
walkable slopes, crouch, jump, or gameplay binding layer.
