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

The minimal command-line interface accepts optional `--frames N`, where `N` is
a positive integer. Running without arguments continues until window close.

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
v0.1.2. Server, client, command, and network-flush stages documented in
ADR-0003 remain architectural targets.

All bootstrap code is original project code. No source, license text, or file
header was copied from the read-only Quake III Arena or ioquake3 references.
