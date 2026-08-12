# Camera Foundation

`HTHCamera` is engine-owned mathematical state: position, forward and up
vectors, vertical FOV in radians, and near/far clipping planes. It contains no
input, player, SDL, OpenGL, or renderer-backend state.

The v0.1.5 bootstrap camera is static:

```text
position = (0, 0, 3)
forward  = (0, 0, -1)
up       = (0, 1, 0)
FOV Y    = 75 degrees
near     = 0.1
far      = 1000
```

The view calculation normalizes forward, derives right from `forward × up`,
and derives a corrected orthogonal up vector. A zero forward vector or a
forward vector parallel to up fails cleanly instead of producing NaNs. No
automatic recovery policy is introduced yet.

Projection derives aspect from the current framebuffer pixel width and height,
never from logical window size. Resize therefore updates both viewport and
projection. A zero-sized minimized framebuffer is a valid non-drawable state:
the renderer skips rendering and presentation until a later valid resize.

The camera remains fixed in this milestone. Movement, mouse-look, input
binding, player state, scene transforms, and gameplay are deliberately absent.
