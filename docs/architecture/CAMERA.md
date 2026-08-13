# Camera and FPS Controller

`HTHCamera` remains reusable mathematical view state: position, forward and up
vectors, vertical FOV in radians, and near/far clipping planes. It contains no
input, player, SDL, OpenGL, yaw, pitch, or renderer-backend state.

The engine owns the bootstrap camera. Its default mathematical state remains
position `(0, 0, 3)`, forward `(0, 0, -1)`, up `(0, 1, 0)`, 75-degree vertical
FOV, near 0.1, and far 1000. In v0.1.7 runtime position is supplied by Player
Body plus its eye offset. Projection aspect comes from framebuffer pixels.

## Internal FPS Camera Controller

v0.1.6 added an engine-internal controller under `src/common/`. It reads only
`HTHInput`, owns yaw/pitch and bootstrap look tuning, and modifies camera
orientation. v0.1.7 removes physical translation from this controller.
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

W/S/A/D now feed Player Movement. That subsystem derives its horizontal basis
from camera orientation but moves Player Body, never `HTHCamera` directly.
After collision resolution, Camera position follows body position plus the
1.60-unit eye offset. Rotating Camera does not rotate or translate the body.
Explicit v0.1.8 step-up and step-down therefore move Camera by exactly the
resolved Body height; smoothing, bobbing, and interpolation remain absent.
v0.1.9 jump and landing likewise change `body.position.y`, and Camera follows
that resolved height immediately. No landing kick, jump smoothing, or view
effect is added.

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

The v0.1.6 controller could produce free-camera navigation. v0.1.7 keeps its
orientation and capture responsibilities while Player Body and Player Movement
own translation. v0.1.9 gives Player Movement a Space jump while capture is
active; gameplay entities and a semantic binding system remain absent.
