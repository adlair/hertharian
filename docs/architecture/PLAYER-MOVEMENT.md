# Player Movement

The v0.1.7 movement foundation converts physical W/S/A/D state into an
engine-internal horizontal movement intent. Input does not mutate the player
body directly, and the body does not read Input. During this bootstrap,
movement intent is enabled only while FPS pointer capture is active; focus loss
clears held keys through the existing Input policy, so stale movement stops.

W/S use the camera forward direction projected onto XZ. A/D use the normalized
right vector `horizontal_forward × up`. Pitch is discarded, so looking up or
down cannot move the body vertically. Combined input is normalized to prevent
faster diagonal travel.

Movement currently sets horizontal velocity directly to intent times 4 world
units per second, including while airborne. This is intentionally a minimal
interactive foundation, not an acceleration, friction, or air-control model.
With no horizontal intent, X/Z velocity is zero. There is no jump.

Gravity is -9.81 world units per second squared and uses semi-implicit Euler:

```text
velocity.y += gravity * dt
position += velocity * dt
```

Only movement integration clamps a frame delta above 0.1 seconds, limiting a
single variable-delta collision step after a pause. Normal timing remains
unchanged. This local interactive movement is not a fixed simulation tick and
does not claim network determinism.
