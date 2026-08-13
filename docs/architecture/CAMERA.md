# Camera and FPS Controller

`HTHCamera` remains reusable mathematical view state: position, forward and up
vectors, vertical FOV in radians, and near/far clipping planes. It contains no
input, player, SDL, OpenGL, yaw, pitch, or renderer-backend state.

The engine owns the bootstrap camera. Its initial state remains position
`(0, 0, 3)`, forward `(0, 0, -1)`, up `(0, 1, 0)`, 75-degree vertical FOV,
near 0.1, and far 1000. Projection aspect comes from framebuffer pixels.

## Internal FPS Camera Controller

v0.1.6 adds an engine-internal controller under `src/common/`. It reads only
`HTHInput`, owns yaw/pitch and bootstrap tuning, and modifies an `HTHCamera`.
The controller is separate so cameras can still be constructed or driven by
non-FPS systems later.

Yaw is rotation around global +Y. Zero yaw faces -Z; positive yaw turns toward
+X, so positive horizontal mouse delta looks right. Pitch is positive upward
and is clamped to ±89 degrees, avoiding a forward vector parallel to up. The
forward vector is:

```text
(sin(yaw) cos(pitch), sin(pitch), -cos(yaw) cos(pitch))
```

Sensitivity is a bootstrap constant of 0.10 degrees per mouse unit. Mouse
delta is displacement already accumulated during the frame, so it is not
multiplied by delta time.

W/S add or subtract a forward vector projected onto XZ. Right is normalized
`horizontal_forward × up`; at the baseline it is +X. A/D subtract or add that
right vector. Combined intent is normalized, movement is 4 world units per
second, and timing delta makes movement frame-rate independent. Only the
controller's movement delta is capped at 0.1 seconds to prevent teleporting
after a debugger or scheduler pause. Pitch never changes movement height.

## Bootstrap Capture Policy

Left click requests relative mouse capture; Escape releases it; left click can
capture again. Mouse movement changes look only while capture is active.
Capture transitions clear accumulated mouse delta, then Input discards relative
motion for the rest of that frame and the next complete frame. This small,
backend-independent transition window prevents synthetic capture motion from
reaching the controller. Focus loss already clears held Input state, and both
focus loss and gain discard frame mouse delta to prevent a stale look jump.
Shutdown disables relative mode before destroying the window. No keyboard grab,
mouse warp, manual cursor hiding, or extra mouse grab is used.

Platform can select an explicit SDL relative-pointer source dynamically when a
backend exposes one, with generic SDL event behavior as fallback. This solves a
source-selection concern before HTH event translation. It is separate from the
short Input transition guard, and neither device IDs nor backend names enter the
FPS Controller or Camera.

The click-to-capture / Escape-to-release policy is temporary interaction
behavior for FPS camera validation, not the final Hertharian input/menu binding
design.

This is free camera navigation, not player movement. Gravity, collision,
grounding, jump, vertical controls, physics, player entities, and semantic
bindings remain deliberately absent.
