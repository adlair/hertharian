# Input

Input consumes engine-owned physical events produced by the private platform
backend:

```text
SDL3 event → HTH platform event → HTH input state
```

SDL scancodes, keycodes, event structures, and mouse constants do not cross
the platform boundary. The initial physical key set covers letters, digits,
common editing/navigation keys, modifiers, arrows, and F1 through F12. It does
not define gameplay actions or bindings.

## Frame State

Held keys and mouse buttons persist as `down`. A transition to down sets
`pressed` for that frame; a transition to up sets `released`. At the beginning
of the next frame, pressed/released edges are cleared while down state remains.

Mouse state includes absolute position, accumulated relative delta, five
buttons, and accumulated horizontal/vertical wheel movement. Delta and wheel
movement reset every frame; position and held buttons persist.

When keyboard focus is lost, all held keys and mouse buttons are released and
mouse delta is cleared. This prevents input released outside the window from
remaining virtually stuck.

Platform reconciles only keys it previously reported as down after draining
the native event queue. If a normal key-up is unavailable but the backend's
observed state says that a reported key is up, Platform emits exactly one
normalized HTH key-up through the regular event path. It never synthesizes a
key-down or `pressed`, and a later real key-up is harmless because release is
idempotent. A fresh non-repeat key-down works normally after recovery.

SDL's cached keyboard snapshot is used by the general backend path, but is not
an independent authority under an X11 global interruption: it can remain down
together with the missing SDL key-up. The optional X11 Platform path therefore
uses `XQueryKeymap` as its release-recovery authority. It associates each HTH
key with the native X11 keycode carried by the real SDL key-down event and
recovers only missing releases. Native Wayland does not use this path.

## FPS Camera Consumption

v0.1.6 uses the existing accumulated `mouse_delta_x` and `mouse_delta_y` for
FPS look. Relative mode changes only how Platform supplies motion; the
controller still reads HTH Input and never SDL. Mouse delta is displacement
that occurred during the frame, not a rate, and therefore is intentionally not
multiplied by frame delta time.

As of v0.1.7, W/S/A/D physical state feeds an internal Player Movement intent
while capture is active. Player Movement derives its horizontal basis from
Camera orientation and applies variable-delta motion to Player Body; the FPS
controller no longer translates Camera directly. Input remains unaware of
Player Body and Collision.

v0.1.9 additionally consumes Space's `pressed` edge as the bootstrap jump
request while capture is active. It intentionally does not use held `down`, so
holding Space through landing cannot auto-jump. This is still direct bootstrap
physical-key consumption, not an action-map or binding system.

The engine coordinates a temporary bootstrap capture policy: left click
enables relative mode and Escape disables it. Capture transitions clear Input's
current mouse delta and discard relative-motion events for the remainder of the
transition frame plus the next complete frame. This deterministic transition
window prevents backend-generated relative motion from reaching the controller
without inspecting delta magnitude or video-driver identity. Focus transitions
also clear frame mouse delta. Escape is not quit, no keyboard grab is enabled,
and a released pointer can be captured again.

Platform may validate an explicit SDL relative-motion source before producing
an HTH mouse event. That source selection is dynamic and backend-private; Input
still receives the same device-independent `HTHPlatformEvent` and knows no SDL
device IDs or names. This filtering is distinct from Input's short capture-
transition guard: filtering chooses the valid relative stream, while the guard
suppresses asynchronous residue around any successful mode change.

These physical controls are not a semantic binding system. Rebinding, action
maps, menus, attack semantics, and final gameplay bindings remain excluded.

Quit, focus, and resize remain engine-control events rather than gameplay
actions. Application close continues to come from the window/QUIT event.
