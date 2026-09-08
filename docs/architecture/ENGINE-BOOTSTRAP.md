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
`--debug-fps-input`. As of v0.2.6, `--level <id>` selects a logical Level ID at
startup; omitting it selects `bootstrap`. Running without arguments continues
until window close.

The current frame order is:

```text
Timing begin → Input begin → Platform events → Input state
→ capture coordination → FPS orientation → effective simulation delta
→ Player Death query → alive/capture movement gate → movement intent
→ locomotion friction/acceleration/jump/gravity
→ swept slide/step movement → ground probe
→ resolved Player Body + MovementResult → physical eye
→ Player Target Bridge sync → Enemy Pursuit Runtime
→ View Dynamics → final Camera position/FOV
→ post-Pursuit Runtime Enemy visual extraction
→ Renderer camera → static + transient render/present → work measurement → pacing
→ Input end → Timing finish
```

Pressed/released edges and accumulated mouse/wheel deltas are cleared at the
next Input begin. The orientation controller and Player Movement consume the
current frame's Input before Renderer sees the followed camera. Input end advances only the
short relative-mode transition guard; Timing finish then marks the whole frame
complete.

Initialization resolves runtime options to a logical Level ID, creates an
owned Level Selection, and deterministically maps it to a canonical Resource
ID. Engine then creates the internal Resource System, loads that resource,
parses it into a temporary Level v2 Description, and builds/finalizes the
engine-owned World. Resource bytes and the description are released before
Collision, Player, Platform, or Renderer initialization continues. The World
supplies the default spawn and content consumed by Collision and Renderer; the
spawn is checked against the derived Collision World before Platform startup.
Headless and graphical modes use the same selected Level.

Shutdown destroys current consumers and World before Resources, then Platform.
Every failure path unwinds the resources already acquired. Missing, malformed,
or World-invalid level content fails initialization without compiled fallback
geometry.

As of v0.2.4, Engine also eagerly loads one external bootstrap material for
every World visual class and decodes each referenced PPM texture. Headless mode
performs this complete content validation while skipping only Renderer/GPU
creation. In graphical mode Engine resolves visible World objects to transient
renderer draw inputs; OpenGL uploads the image bytes synchronously and owns the
resulting textures. Material/image bytes never become World or Level state,
and missing or malformed appearance resources fail without a compiled palette
fallback.

As of v0.2.5, Engine also validates immutable built-in BOX/WEDGE geometry in
headless and graphical modes. World objects carry independent collision and
render shapes over shared bounds. Collision extracts only explicit AABBs;
Engine translates visible render shapes into primitive/material draw inputs.
The bootstrap wedge is deliberately visible-only and pass-through.

As of v0.2.6, logical Level ID, canonical Resource ID, Level Description, and
World are distinct states. The internal startup selection owns copies of the
Level ID and its mapped Resource ID throughout Engine lifetime. There is no
Level registry, metadata, reload, transition, or current-level public API.

As of v0.2.7, each Engine also owns one private Entity Registry. It is created
empty after the selected Level has produced its finalized static World and is
destroyed before that World during shutdown. No Level content, Player state,
or static World object is converted into an Entity, so production maintains
zero live Entities and the frame lifecycle performs no Entity traversal.

As of v0.2.8, Engine creates one empty optional Spatial Store immediately after
the Entity Registry. It owns no Entity lifetime and retains no Registry
pointer. Shutdown destroys Spatial before Entity identity and World. Production
creates no Spatial associations and performs no Spatial work per frame.

As of v0.2.9, Engine then creates one empty Dynamic Body Store. Shutdown
destroys it before Spatial, Entity identity, and World. Production creates no
Bodies and performs no Dynamic Collision work per frame; the foundation has no
visible runtime effect beyond the Engine version.

As of v0.3.0, Engine creates one empty Actor Store after Dynamic Body. Actor is
only a gameplay-participation association over Entity identity; it owns no
Spatial or physics state. Shutdown destroys Actor before Dynamic Body,
Spatial, Entity identity, and World. Production creates zero Actors and the
frame lifecycle performs no Actor traversal, ticking, behavior, rendering, or
physics work, so runtime behavior remains unchanged beyond the Engine version.

As of v0.3.1, Engine creates one empty Health Store after Actor and destroys it
before Actor. Health is optional Actor-dependent gameplay state with
generation-safe damage and healing operations. Production attaches no Health
and performs no Health work per frame; no Player, Level, World, physics, or
rendering behavior changes beyond the Engine version.

As of v0.3.4, Engine also creates one private empty Enemy Store after Actor and
before Health. Shutdown destroys Health, Enemy, then Actor, followed by Dynamic
Body, Spatial, Entity identity, and World. Enemy is a presence-only Actor
specialization; production attaches zero Enemies and performs no Enemy work
per frame, so runtime behavior remains unchanged beyond the Engine version.

As of v0.3.5, Engine creates one private empty Enemy Target Store after Health.
Shutdown destroys Enemy Target before Health, Enemy, and Actor. Production
establishes zero target relations and performs zero target or perception work
per frame. Enemy Perception remains a stateless query with no Engine-owned
state.

As of v0.3.15, Engine also owns one private bootstrap pursuit integration. At
the end of initialization it creates one stable Player Entity+Spatial proxy and
one canonical Runtime Enemy. Each applicable frame synchronizes the proxy after
Player movement and executes Pursuit before View/Camera/Renderer. Shutdown
despawns the Enemy, destroys the proxy, then continues existing Store teardown.
The Enemy collides only with static World geometry; see
`ENEMY-PURSUIT-ENGINE-INTEGRATION.md` and ADR-0039.

As of v0.3.16, graphical frames extract that Enemy's post-Pursuit Spatial and
DynamicBody into one stack-local BOX draw immediately before Renderer. The
white opaque draw is presentation-only, uses no persistent visual state, and is
skipped entirely in headless mode; see `RUNTIME-BODY-VISUALIZATION.md` and
ADR-0040.

As of v0.3.20, the same stable Player target Entity also carries Actor and
Health. The bootstrap supplies temporary `100/100` Player Health while
PlayerBody remains the physical authority and Spatial remains its synchronized
body-center mirror. No new frame phase, DamageIntent, Health mutation, death
behavior, or Player rendering is introduced; see `PLAYER-DAMAGE-TARGET.md` and
ADR-0044.

As of v0.3.23, the Engine owns the private Enemy Attack Cadence Store and the
existing Enemy Pursuit frame phase also executes ready attacks after Player
movement and Player Target Bridge sync. Bootstrap supplies temporary 10-point
damage and a one-second interval. Player Health may reach zero without death
runtime effects; rendering remains independent of Health.

As of v0.3.24, Player Death defines a private derived query over the Bridge
target's current Health.

As of v0.3.25, Engine consumes that query once before movement intent. A dead
Player receives zero voluntary intent but still executes the complete physical
movement step, so gravity, friction, momentum, collision, grounding, and
landing continue. Input, mouse-look, Camera, View Dynamics, Bridge sync,
targeting, attacks, rendering, and shutdown remain active. Damage applied by
Enemy Runtime in frame N affects voluntary movement from frame N+1; healing
above zero restores normal control without persistent death state. See
`PLAYER-DEATH-RUNTIME-INTEGRATION.md` and ADR-0049.

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

As of v0.3.26, Engine passes its single Player Death snapshot to Bootstrap.
Bootstrap maps the dead Player's stable Bridge handle to the generic
`excluded_target` used by Pursuit and Selection; no Health/death dependency
crosses into those Enemy foundations.

As of v0.3.27, the private Player Defeat Foundation exists as a disconnected
caller-owned primitive. Engine and Bootstrap own no Defeat state, make zero
Defeat calls, and perform zero related per-frame work.

As of v0.3.28, the private Player Revive Window Foundation is likewise
disconnected. Engine and Bootstrap own no window, never begin or advance one,
and perform zero related per-frame work.

As of v0.3.29, the private Player Role / Roster Foundation is also
disconnected. Engine and Bootstrap own no roster, register no Player, and
perform zero roster work per frame.

All bootstrap code is original project code. No source, license text, or file
header was copied from the read-only Quake III Arena or ioquake3 references.
