# Hertharian Engine Bootstrap v0.1.0

The first engine version establishes a small, deterministic lifecycle without
introducing runtime subsystems prematurely.

## Lifecycle

The executable prints its version, initializes a caller-owned `HTHEngine`, runs
frames through `hth_engine_run`, and shuts the engine down. `hth_engine_run`
owns the loop and delegates each iteration to `hth_engine_frame`.

The configuration carries a frame limit so development runs and tests can
terminate deterministically. As of v0.1.1, a limit of zero runs until the
platform reports a window-close event.

## Initial Structure

```text
engine/
├── CMakeLists.txt
├── include/
│   └── hth_engine.h
├── src/
│   ├── main.c
│   └── common/
│       └── engine.c
└── tests/
```

`hth_engine.h` is the single declaration point for the bootstrap API and its
small explicit state object. There is no implementation-private `engine.h` yet,
because v0.1.0 has no private declarations to put in it.

## Build

From the repository root:

```bash
cmake -S engine -B build/engine
cmake --build build/engine -j$(nproc)
```

## Run

Run exactly three frames with:

```bash
./build/engine/hertharian-engine --frames 3
```

The command-line interface accepts optional `--frames N`, where `N` is a
positive integer, plus `--headless` and the focused development diagnostic
`--debug-fps-input`. Running without arguments continues until window close.

The current frame order is:

```text
Timing begin → Input begin → Platform events → Input state
→ capture coordination → FPS orientation → movement intent
→ locomotion friction/acceleration/jump/gravity
→ swept slide/step movement → ground probe
→ resolved Player Body + MovementResult → physical eye
→ View Dynamics → final Camera position/FOV
→ Renderer camera → render/present → work measurement → pacing
→ Input end → Timing finish
```

Pressed/released edges and accumulated mouse/wheel deltas are cleared at the
next Input begin. The orientation controller and Player Movement consume the
current frame's Input before Renderer sees the followed camera. Input end advances only the
short relative-mode transition guard; Timing finish then marks the whole frame
complete.

As of v0.2.1, initialization first builds and finalizes the engine-owned World,
reads its default spawn, then derives Collision and Renderer initialization
data. The spawn is validated against the derived Collision World before
Platform or Renderer startup. Shutdown destroys Renderer and physical
Collision state before World so no consumer can outlive its source content.

## Tests

After configuring and building, run:

```bash
ctest --test-dir build/engine --output-on-failure
```

CTest launches the executable for three frames, checks successful termination,
and verifies the expected lifecycle output.

## Deliberately Excluded

v0.1.0 did not include SDL, OpenGL, rendering, audio, networking, a filesystem,
BSP loading, collision, real input, QVM/VM support, game logic, assets, or maps.
The SDL3 platform foundation introduced in v0.1.1 is documented separately in
`PLATFORM.md` and ADR-0004.
Timing, platform event translation, and physical input were introduced in
v0.1.2. Renderer Bootstrap introduced graphical clear and presentation in
v0.1.3. Server, client, command, and network-flush stages documented in ADR-0003
remain architectural targets.

All bootstrap code is original project code. No source, license text, or file
header was copied from the read-only Quake III Arena or ioquake3 references.
