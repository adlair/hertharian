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

Held keys and mouse buttons persist as `down`. Every valid key-down establishes
`down`; it sets `pressed` for that frame only when it is non-repeat and the
previous logical state was up. A repeat key-down may therefore restore `down`
without setting `pressed`. A transition to up sets `released`. At the beginning
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
idempotent. Any later SDL key-down is current down-state evidence, including a
repeat key-down from a key that remained physically held. Input therefore
restores `down` from that event immediately, but a repeat never creates a
`pressed` edge. A fresh non-repeat key-down works normally after recovery. A newly
reported down is protected from one immediately lagging backend observation;
only a subsequent observed-up discrepancy may normalize it as released. This
keeps the first physical press after an interruption active in the same input
update without restoring keys held before focus loss.

SDL's cached keyboard snapshot is used by the general backend path, but is not
an independent authority under an X11 global interruption: it can remain down
together with the missing SDL key-up. The optional X11 Platform path therefore
uses `XQueryKeymap` as its release-recovery authority. It associates each HTH
key with the native X11 keycode carried by the real SDL key-down event and
recovers only missing releases. Native Wayland does not use this path.
An X11-up/SDL-cached-down observation is treated as a disagreement: the
state-based confirmation can still recover a truly lost release, while a
subsequent SDL key-down repeat supersedes the normalized release without an
extra physical press.

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

On XWayland, a non-relative re-entry motion can be followed by a transition
compensation motion from the otherwise valid relative source. Platform treats
that observed cross-source pair as one pointer-state discontinuity: the foreign
motion arms a one-event compensation discard without producing logical motion.
The following selected-source motion is consumed as compensation, and the next
selected-source motion is accepted immediately. Every accepted SDL motion is
translated one-to-one from `xrel/yrel` to HTH `dx/dy`; Platform never integrates
motion across events. Source identity and event order control only the two
documented discards, never delta magnitude or sign. Input and the FPS controller
remain unaware of the backend detail. Capture-mode, selected-source, focus, and
mouse-focus transitions clear pending compensation according to the Platform
contract. Generic SDL and native Wayland motion follows the same one-to-one
numerical contract without XWayland source filtering.

These physical controls are not a semantic binding system. Rebinding, action
maps, menus, attack semantics, and final gameplay bindings remain excluded.

Quit, focus, and resize remain engine-control events rather than gameplay
actions. Application close continues to come from the window/QUIT event.
