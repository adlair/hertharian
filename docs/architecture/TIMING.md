# Timing v0.1.2

Timing uses the platform layer's high-resolution monotonic counter and
frequency. `HTHTiming` is engine-owned and contains no SDL types.

At frame start, the current counter is compared with the previous frame start.
That measured interval becomes `delta_seconds`; it is never replaced with a
hardcoded target interval. `elapsed_seconds` measures time since timing
initialization. `frame_work_seconds` is captured when engine work finishes and
therefore excludes pacing sleep. Work completion and full frame completion are
separate timing phases: the frame counter advances only after pacing has
finished.

## Frame Pacing

The default `target_fps` is 60. A positive target enables pacing; zero means
uncapped and performs no division or sleep. After frame work, the engine
calculates the remaining time before the target deadline and uses the platform
nanosecond sleep backed by SDL3's `SDL_DelayNS()` when useful. Remaining time
is retained in nanoseconds instead of being systematically truncated to whole
milliseconds. Scheduler overshoot remains possible and is reflected in the
next measured delta; no busy-spin is used.

This is presentation-independent engine pacing. It replaces the fixed
bootstrap sleep, but it is not a fixed simulation tick. v0.1.2 deliberately
does not add an accumulator, interpolation, prediction, a separate physics
tick, or fixed-timestep simulation.
