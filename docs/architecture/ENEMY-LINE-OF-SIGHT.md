# Enemy Line-of-Sight Foundation

Hertharian v0.3.7 provides one pure, synchronous, stateless gameplay query
that asks whether the center-to-center segment from an Enemy to an arbitrary
candidate Entity is unobstructed by static CollisionWorld geometry. It owns no
Store, Engine state, cache, allocation, initialization, shutdown, logging, or
per-frame work.

The observer must be a live Actor with current Enemy and Spatial associations.
The candidate needs only a live Entity generation and Spatial association; it
may be a non-Actor. Self LOS is valid. Health, Dynamic Body, Player, Target,
and radius Perception state are irrelevant to the query.

## Geometry and Result Contract

The exact endpoints are `Spatial(enemy).position` and
`Spatial(candidate).position`, with no eye height, body-surface adjustment,
offset, radius, FOV, or epsilon. If those positions are exactly equal in all
components after semantic validation, gameplay LOS returns true without
calling Collision: there is no intervening spatial interval. This deliberately
does not change Collision's zero-length point-occupancy contract.

Distinct positions invoke exactly one released
`hth_collision_world_trace_segment()` query. Failure to execute that query is
not visible and returns false. A successful `trace.hit` blocks LOS; a clear
trace permits LOS. The closed segment means static contact exactly at the
candidate endpoint blocks, while an obstacle beyond that endpoint is
irrelevant. LOS does not reinterpret fraction, normal, obstacle index,
`start_solid`, or `all_solid`, and uses no fraction threshold.

Only static CollisionWorld AABBs obstruct LOS. Actors, Enemies, Players,
Dynamic Bodies, materials, transparency, triggers, layers, masks, portals,
PVS, and renderer depth do not participate. The query performs O(N) work over
N static obstacles, uses O(1) auxiliary memory, inherits the current
16-obstacle limit, and allocates nothing.

## Independent Gameplay Foundations

Enemy LOS neither accepts nor mutates Enemy Target relationships and never
acquires, clears, or replaces a target. It neither accepts nor calls radius
Perception. LOS, Perception, Target relation, future target selection, and
future AI are orthogonal foundations; callers may compose them later without
changing their individual contracts.

Production has zero LOS callers and performs zero LOS work per frame in
v0.3.7. Dynamic occlusion, FOV, hearing, memory, last-seen state, visibility
caching, batch queries, target selection, AI, locomotion, combat, Level
declarations, persistence, networking, scripting, and ECS remain deferred.
