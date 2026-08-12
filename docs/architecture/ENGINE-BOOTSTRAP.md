# Engine Bootstrap v0.1.0

The first engine version establishes a small, deterministic lifecycle without
introducing runtime subsystems prematurely.

## Lifecycle

The executable prints its version, initializes a caller-owned `GFEngine`, runs
frames through `gf_engine_run`, and shuts the engine down. `gf_engine_run` owns
the loop and delegates each iteration to `gf_engine_frame`.

The configuration carries a frame limit so development runs and tests always
terminate. Internally, a limit of zero represents a future unlimited mode; the
v0.1.0 command line intentionally requires a positive value because exit-signal
handling has not been implemented yet.

## Initial Structure

```text
engine/
├── CMakeLists.txt
├── include/
│   └── gf_engine.h
├── src/
│   ├── main.c
│   └── common/
│       └── engine.c
└── tests/
```

`gf_engine.h` is the single declaration point for the bootstrap API and its
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
./build/engine/gf-engine --frames 3
```

The minimal command-line interface accepts only `--frames N`, where `N` is a
positive integer.

## Tests

After configuring and building, run:

```bash
ctest --test-dir build/engine --output-on-failure
```

CTest launches the executable for three frames, checks successful termination,
and verifies the expected lifecycle output.

## Deliberately Excluded

v0.1.0 does not include SDL, OpenGL, rendering, audio, networking, a filesystem,
BSP loading, collision, real input, QVM/VM support, game logic, assets, or maps.
The wait, input, event/command, server, client, and network-flush frame stages
documented in ADR-0003 are architectural targets, not implemented placeholders.

All bootstrap code is original project code. No source, license text, or file
header was copied from the read-only Quake III Arena or ioquake3 references.
