# Ground/Air Locomotion

v0.1.9 introduces an engine-internal locomotion policy between movement intent
and the v0.1.8 trace/slide/step solver:

```text
Input → Movement Intent → Locomotion velocity policy
      → Trace / Slide / Step Collision → resolved Player Body
```

Locomotion generates velocity; Collision constrains that velocity and the
resulting displacement. Collision does not decide friction, acceleration,
jump capability, or character tuning, and locomotion never iterates geometry.

## Default Movement Configuration

Each engine physical-state instance owns an `HTHMovementConfig`. The sole
v0.1.9 default profile is a tuning baseline:

```text
max_ground_speed     6.0
ground_acceleration 12.0
ground_friction      7.0
stop_speed           1.5
air_acceleration     2.5
max_air_wish_speed   3.0
gravity             -9.81
jump_height          1.0
max_fall_speed      40.0
```

All values are centralized and validated when engine physical state is
initialized. Speeds and coefficients must be nonnegative, gravity must be
negative, and jump height and maximum fall speed must be positive. Invalid
configuration fails initialization without aborting. `HTHPlayerBody` contains
none of these policy values and has no mass.

The configuration is intentionally suitable for future per-character
profiles, but v0.1.9 adds no character IDs, classes, selection, or additional
profiles. A character's narrative weight may inform future tuning; it does not
imply rigid-body mass or mechanically determine jump.

## Intent and World-Space Momentum

W/S contribute horizontal camera forward and A/D contribute horizontal right.
Pitch is discarded; yaw changes only the basis used to create future wish
direction. The raw intent length is retained as wish magnitude and clamped to
one before the direction is normalized. Thus diagonal digital input gains no
extra speed while the contract can later accept analog magnitudes.

Existing velocity remains in world space. Turning the view never rotates it.
New directional acceleration gradually adds velocity toward the new wish
direction, preserving perpendicular or external momentum rather than applying
a global horizontal speed clamp.

## Ground Policy

Only a body that began the update grounded receives ground friction. Friction
uses horizontal speed and runs before acceleration:

```text
control   = max(horizontal_speed, stop_speed)
drop      = control * ground_friction * dt
new_speed = max(0, horizontal_speed - drop)
```

The horizontal direction is preserved while scaling. `stop_speed` makes low
speed settle firmly; it is not an input threshold. Directional acceleration
then computes current speed along wish direction, adds at most
`ground_acceleration * wish_speed * dt`, and never exceeds wish speed along
that direction. `max_ground_speed` limits input's desired speed, not total
physical velocity. Reverse input therefore brakes through zero before building
backward speed.

## Air Policy and Gravity

Airborne bodies receive no friction. Their wish speed is the ground-derived
wish speed limited to `max_air_wish_speed`; basic directional acceleration
uses the same add-speed formula with `air_acceleration`. With no input,
horizontal momentum is unchanged. Perpendicular input adds a component
gradually. There is no special air-control formula, strafe-jump policy,
bunny-hopping system, or hidden damping.

Airborne vertical velocity uses semi-implicit Euler:

```text
velocity.y += gravity * dt
velocity.y = max(velocity.y, -max_fall_speed)
```

The fall-speed limit prevents absurd vertical speeds during long falls; it is
not atmospheric drag. Movement locally clamps a frame delta to 0.1 seconds.
There is still no fixed simulation tick.

## Jump

Space requests jump only on its `pressed` transition, while FPS movement
capture is active and the body began grounded. Holding Space cannot jump again
on landing; a new release/press transition is required. Airborne requests do
nothing, so v0.1.9 has no double jump.

Jump capability is expressed as the design parameter `jump_height`, and the
launch speed is derived rather than tuned independently:

```text
jump_speed = sqrt(2 * abs(gravity) * jump_height)
```

This lets future characters reach different optional routes, secrets, tactical
positions, or traversal advantages through profile data. Core mandatory
progression should generally remain traversable by all intended protagonists
unless level design deliberately chooses otherwise.

An accepted jump clears grounded state before movement. Gravity is integrated
once in the same update, ascending bodies skip the ground probe, and jump
frames cannot use step-up or step-down. A ceiling clips upward velocity through
the ordinary collision solver; subsequent gravity produces descent without a
bounce. Landing restores grounded state and clears descending velocity.

## Capture and Focus

Capture-off or focus-lost input produces zero wish magnitude and no jump
request, but does not freeze physics. Ground friction continues, airborne
horizontal momentum remains, gravity continues, and Collision resolves the
body. Escape therefore releases capture without zeroing velocity or
teleporting the player.

## Non-Goals

This milestone adds no multiple character profiles, mass, rigid-body forces,
knockback, wall/double jump, crouch, sprint, dash, advanced Quake air control,
camera effects, step smoothing, slopes, capsules, moving platforms, BSP,
network prediction, gameplay classes, or fixed-timestep simulation.
