# Player Body

v0.1.7 introduces `HTHPlayerBody` as the engine-internal source of truth for
the local player's physical state. It is deliberately separate from
`HTHCamera`: the body owns translation and collision state, while the camera
owns view orientation and projection state.

The body position is the center of its feet/base. Its bootstrap AABB extends
`half_width` on X and Z, from `position.y` to `position.y + height` on Y:

```text
min = (x - half_width, y,          z - half_width)
max = (x + half_width, y + height, z + half_width)
```

The baseline body has half-width 0.30, height 1.80, and eye height 1.60 world
units. Eye height must be greater than zero and lower than body height. Linear
velocity is a three-component world-space vector. `grounded` means vertical
resolution found supporting static geometry while descending; simple gravity
re-establishes this contact each moving frame without treating face contact as
penetration.

After physical resolution, the engine places the camera at:

```text
body.position + (0, body.eye_height, 0)
```

Changing camera yaw or pitch never changes body position. The player body is
not public API, a gameplay `Entity`, a renderable object, or a networked player
model. Jumping, crouching, animation, weapons, and entity composition are
deliberately absent.
