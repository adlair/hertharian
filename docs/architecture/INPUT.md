# Input v0.1.2

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

Relative mouse mode, cursor capture, semantic actions, and binding UI are
deliberately excluded until a later FPS/gameplay milestone.

Quit, focus, and resize remain engine-control events rather than gameplay
actions. Resize updates the engine's current logical window width and height so
Renderer Bootstrap can consume correct dimensions later; no viewport work is
performed in v0.1.2.
