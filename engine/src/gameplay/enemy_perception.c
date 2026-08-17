#include "enemy_perception.h"

#include <math.h>

bool hth_enemy_perception_can_perceive(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    HTHEntityHandle enemy,
    HTHEntityHandle candidate,
    float radius)
{
    HTHSpatialTransform enemy_transform;
    HTHSpatialTransform candidate_transform;
    double dx;
    double dy;
    double dz;
    double distance_squared;
    double radius_double;

    if (!isfinite(radius) || radius < 0.0F ||
        !hth_enemy_store_has(enemies, entities, actors, enemy) ||
        !hth_spatial_store_get(spatial, entities, enemy,
                               &enemy_transform) ||
        !hth_spatial_store_get(spatial, entities, candidate,
                               &candidate_transform)) {
        return false;
    }
    dx = (double)candidate_transform.position.x -
         (double)enemy_transform.position.x;
    dy = (double)candidate_transform.position.y -
         (double)enemy_transform.position.y;
    dz = (double)candidate_transform.position.z -
         (double)enemy_transform.position.z;
    distance_squared = dx * dx + dy * dy + dz * dz;
    radius_double = (double)radius;
    return distance_squared <= radius_double * radius_double;
}
