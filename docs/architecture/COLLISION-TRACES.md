# Collision Traces

v0.1.8 introduces an internal swept-volume query between Player Movement and
Collision. A trace asks how an axis-aligned moving volume travels from `start`
to `end`; it does not apply gravity, change velocity, select a step, or modify
player state.

The volume is expressed as local `mins` and `maxs` around the traced origin.
For the feet-origin Player Body these are:

```text
mins = (-half_width, 0,      -half_width)
maxs = ( half_width, height,  half_width)
```

`HTHTrace` reports:

- `hit`: the sweep encountered solid geometry;
- `fraction`: progress in `[0, 1]`, where 1 means the requested end was clear;
- `end_position`: `start + fraction * (end - start)`;
- `normal`: the geometric normal of the earliest impact face;
- `obstacle_index`: the current backend's hit identifier;
- `start_solid`: the initial AABB strictly penetrates solid geometry;
- `all_solid`: it starts solid and the requested end remains in the union of
  solid volumes.

Sharing a face is touching, not penetration. Consequently, resting on a floor
is not `start_solid`, and motion exactly tangent to a touched face remains
free. A trace into that face may hit at fraction zero. Zero movement outside
solid returns a clear fraction-one result; zero movement inside solid reports
`start_solid` and `all_solid` without producing NaNs.

## Current Algorithm and Backend

The static obstacle is expanded by the moving volume extents, reducing swept
AABB versus AABB to a point segment versus an expanded box. Per-axis entry and
exit fractions handle positive, negative, and zero displacement without
division by zero. The maximum entry and minimum exit identify a valid interval;
the entry axis directly supplies one of the six face normals. Collision World
checks every static AABB and returns the lowest valid fraction.

Trace fractions remain geometric. Player Movement owns its separate `1e-5`
surface offset policy; Collision does not falsify a trace fraction to create a
gap.

The contract is internal and the current backend is a brute-force static AABB
array copied from the collidable subset of the finalized World. Its
`obstacle_index` is local to that copied Collision array; it is not a World
object index or a persistent content identifier. A future BSP collision backend
can provide equivalent swept-volume
semantics without making Player Movement understand BSP or obstacle storage.
Initial penetration is reported but no general depenetration is attempted.
