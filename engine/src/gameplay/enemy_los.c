#include "enemy_los.h"

#include "collision_trace.h"

static bool equal_position(HTHVec3 left, HTHVec3 right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

bool hth_enemy_los_has_line_of_sight(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    const HTHCollisionWorld *collision_world,
    HTHEntityHandle enemy,
    HTHEntityHandle candidate)
{
    HTHSpatialTransform enemy_transform;
    HTHSpatialTransform candidate_transform;
    HTHTrace trace;

    if (collision_world == NULL ||
        !hth_enemy_store_has(enemies, entities, actors, enemy) ||
        !hth_spatial_store_get(spatial, entities, enemy,
                               &enemy_transform) ||
        !hth_spatial_store_get(spatial, entities, candidate,
                               &candidate_transform)) {
        return false;
    }
    if (equal_position(enemy_transform.position,
                       candidate_transform.position)) {
        return true;
    }
    if (!hth_collision_world_trace_segment(
            collision_world, enemy_transform.position,
            candidate_transform.position, &trace)) {
        return false;
    }
    return !trace.hit;
}
