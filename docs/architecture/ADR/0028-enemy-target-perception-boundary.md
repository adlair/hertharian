# ADR-0028: Separate Enemy Targets, Spatial Perception, and Future AI

- Status: Accepted
- Milestone: v0.3.5

## Context

Enemy currently marks a presence-only Actor specialization. The next runtime
foundation needs both an explicit Entity reference owned by an Enemy and a
way to evaluate spatial proximity, without prematurely introducing target
selection policy, AI, visibility, combat, or Enemy definitions.

## Decision

Add a dedicated internal Enemy Target Store that owns one mutable
`HTHEntityHandle` relation per Enemy generation. Any live Entity may be the
target, including a non-Actor, non-Spatial Entity or the Enemy itself. Target
storage does not imply perception.

Separately add a pure, stateless Enemy Perception query. It evaluates any live
Spatial candidate against a current Spatial Enemy using a caller-supplied 3D
Euclidean radius. It neither receives the Target Store nor mutates target
state. The Target Store does not depend on Perception. Future target selection
and AI remain separate responsibilities above both foundations.

## Rejected Alternatives

- A target field in Enemy Store or an Enemy payload would couple independent
  role and relationship lifetimes.
- Requiring target Actor, Player, Health, or Spatial state would conflate an
  Entity reference with gameplay composition or current perceptibility.
- Automatic acquisition, nearest-target scans, or priority would introduce
  target-selection policy.
- FOV, view cones, yaw policy, LOS traces, hearing, caches, or perception
  memory would exceed a radius-only stateless foundation.
- Bundling targeting or perception with AI, locomotion, attacks, or combat
  would merge responsibilities whose contracts are not yet defined.
- A generic relation graph, generic Entity link, ECS component framework, or
  query system would replace two narrow requirements with premature
  infrastructure.

## Consequences

Logical target validity and current perception may differ. Enemy or target
generation reuse is safe; Enemy removal can hide and later reveal a retained
same-generation relation. Perception remains deterministic, allocation-free,
and independent of current target. Production creates one empty Target Store
but establishes no relations and performs no perception or target work in
v0.3.5.
