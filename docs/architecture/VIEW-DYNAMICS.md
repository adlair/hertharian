# View Dynamics

v0.2.0 separates the physical eye anchor from transient presentation of the
player view:

```text
PlayerBody → Physical Eye Anchor → View Dynamics → Final Camera → Renderer
```

View Dynamics never receives or modifies PlayerBody, CollisionWorld,
Locomotion, Input, Renderer, SDL, or OpenGL. It consumes a small observation
record and returns vertical, view-local lateral, and FOV offsets. The state and
configuration are internal and owned per engine instance.

## Physical Anchor and Composition

The physical source of truth remains:

```text
physical_eye = body.position + (0, eye_height, 0)
```

Each frame the Engine recomposes presentation from that anchor and a stable
base FOV:

```text
vertical_offset = step_offset + landing_offset + bob_vertical
camera.position = physical_eye
                + world_up * vertical_offset
                + camera_right * bob_lateral
camera.fov      = base_fov + fov_offset
```

Offsets are never added to the previous frame's Camera position or FOV, so
they cannot accumulate into drift. Camera orientation remains the immediate
FPS-controller result; View Dynamics does not produce pitch, yaw, roll, mouse
smoothing, or aim lag. Renderer consumes only the final `HTHCamera`.

## State, Input, and Output

State retains initialization, previous physical-eye Y and grounded status,
independent step and landing offsets, bob phase and current bob offsets, and
the current FOV offset. The input contains only physical eye position,
horizontal physical speed, the active movement profile's speed reference,
grounded status, and the frame-local MovementResult landing observation. The
output contains vertical and lateral positional offsets plus an FOV offset in
radians.

The baseline configuration is provisional tuning:

```text
step_smoothing_half_life  0.08 s
max_step_smoothing_delta  0.35
min_landing_speed         1.5
landing_scale             0.035
max_landing_offset        0.14
landing_half_life         0.16 s
bob_vertical_amplitude    0.035
bob_lateral_amplitude     0.014
bob_frequency             1.7 cycles/s at full speed
max_fov_boost_degrees     4.0
fov_half_life             0.12 s
```

Half-lives must be positive; limits, amplitudes, frequency, scale, and minimum
speed must be nonnegative. All values and observations reject NaN/infinity.
The view update clamps only its local delta to 0.1 seconds as long-gap safety.

## Frame-Rate-Independent Recovery

All recovery uses half-life decay:

```text
decay = 2^(-dt / half_life)
x_to_zero *= decay
x_to_target = target + (x_to_target - target) * decay
```

No fixed per-frame lerp factor or fixed simulation tick is introduced.

## Step Smoothing

The first observation seeds previous eye height and grounded state without
interpreting spawn position as a step. On later frames, a grounded-to-grounded
physical-eye change no larger than `max_step_smoothing_delta` injects the
opposite delta into the step offset. A +0.20 step therefore injects -0.20; a
-0.20 step injects +0.20. The offset is then decayed in the same update.

Grounded-to-airborne jumps, airborne motion, airborne-to-grounded landings,
and larger discontinuities inject no step offset. PlayerBody still completes
the physical step immediately; only its visual presentation settles.

## Landing Response and MovementResult

Player Movement returns a frame-local internal result containing `landed` and
`landing_speed`. `landed` is exactly an airborne-to-grounded transition, and
speed is the magnitude of downward velocity after locomotion/gravity but before
Collision clears it. It is neither persistent body state nor a gameplay event.

At or above the minimum speed, View Dynamics injects:

```text
magnitude = clamp((landing_speed - min_landing_speed) * landing_scale,
                  0, max_landing_offset)
landing_offset -= magnitude
```

This gives a small downward compression followed by half-life recovery. Small
contacts create no response; large falls remain capped. There is no bounce,
shake, fall damage, animation, sound, or mass relationship.

## Conservative Walking Bob

Bob is driven by final horizontal physical speed, not input. Speed is divided
by the active profile's reference speed and clamped to `[0,1]`; a zero reference
produces zero. While grounded and moving:

```text
phase += 2π * frequency * speed_norm * dt
vertical = sin(phase) * vertical_amplitude * speed_norm
lateral  = sin(phase / 2) * lateral_amplitude * speed_norm
```

Phase wraps at `4π`, preserving continuity for both functions. The output's
lateral scalar is projected by Engine onto current camera-right; View Dynamics
does not know Camera orientation. Stationary or airborne bob output is zero,
while phase is retained for a stable resume. Amplitudes never exceed their
configured conservative maxima.

## Speed FOV Response

The same capped physical-speed normalization targets a boost from zero to four
degrees. The target is converted once per update to the Camera's radian units,
then approached with `fov_half_life`. Stopping smoothly returns the offset to
zero; overspeed cannot exceed the configured boost. Vertical speed, jump, and
landing do not affect FOV.

## Future Configuration

Separate configuration can later support reduced/disabled bob, view movement,
or FOV response for accessibility, and distinct view profiles for characters.
Conceptually a heavy character might use slower bob and a weightier landing,
while an agile character might use a lighter response. v0.2.0 implements no UI,
settings system, character identities, multiple profiles, or mass semantics.
