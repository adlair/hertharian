# Math Foundation

Hertharian runtime graphics math uses `float`. The public `hth_math.h` API
provides the small set of vector and matrix operations required by the current
camera and renderer path; it has no SDL, OpenGL, GPU, or display dependency.

## Spatial Convention

The initial world is right-handed: +X points right, +Y points up, and the
baseline camera looks along -Z. Future map and BSP ingestion must preserve this
engine convention or perform an explicit boundary conversion; v0.1.5 does not
implement that conversion.

`HTHMat4` stores elements column-major and transforms column vectors with
`p' = M * p`. Composition reads right to left, so the graphics chain is
`p_clip = Projection * View * Model * p_local`. Translation occupies the final
matrix column. This memory layout is uploaded directly to GLSL with
`transpose = GL_FALSE`.

## Angles and Validity

Angles accepted by projection math are radians. Callers with degrees use the
explicit `hth_degrees_to_radians` helper. Perspective rejects non-finite or
out-of-range FOV, aspect, and clipping planes rather than creating NaN or
infinite matrices.

Vector normalization returns `false` for a null output, non-finite length, or
near-zero vector and writes a zero vector when an output exists. This is a
runtime failure policy, not a universal floating-point comparison policy.
Tests use one centralized `1e-5` tolerance for derived values.

The subsystem deliberately excludes general inversion, determinants,
decomposition, quaternions, transforms, and external math libraries.
