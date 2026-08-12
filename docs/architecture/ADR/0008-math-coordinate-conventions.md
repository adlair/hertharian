# ADR-0008: Math and Coordinate Conventions

- Status: Accepted
- Milestone: v0.1.5

## Context

Camera and MVP transforms require one convention shared by reusable engine
math, renderer frontend code, GLSL, and future world data. An implicit or mixed
convention makes multiplication order and translation errors difficult to see.

## Decision

Hertharian Engine uses one explicit and documented mathematical convention for
vectors, matrices, coordinate handedness, and transform composition throughout
engine and renderer code.

- World space is right-handed.
- +X is right and +Y is up.
- Baseline camera forward is -Z.
- Matrices use column-major memory layout.
- Vectors are columns transformed as `p' = M * p`.
- Transform composition is `MVP = Projection * View * Model`.
- GLSL receives native matrix storage with `transpose = GL_FALSE`.

## Consequences

Matrix multiplication order is observable and covered by tests. Camera view and
OpenGL-compatible perspective share the same handedness. Future map/BSP tooling
must honor this convention or convert explicitly at its boundary; no format
conversion is part of this decision.
