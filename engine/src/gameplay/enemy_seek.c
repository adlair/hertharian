#include "enemy_seek.h"

#include <math.h>

bool hth_enemy_seek_compute(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    const HTHSpatialStore *spatial,
    HTHEntityHandle enemy,
    HTHEntityHandle target,
    HTHVec3 *out_direction)
{
    const HTHVec3 zero = {0.0F, 0.0F, 0.0F};
    HTHSpatialTransform enemy_transform;
    HTHSpatialTransform target_transform;
    double dx;
    double dy;
    double dz;
    double length_squared;
    double length;

    if (out_direction != NULL) {
        *out_direction = zero;
    }
    if (entities == NULL || actors == NULL || enemies == NULL ||
        spatial == NULL || out_direction == NULL ||
        !hth_enemy_store_has(enemies, entities, actors, enemy) ||
        !hth_spatial_store_get(spatial, entities, enemy,
                               &enemy_transform) ||
        !hth_spatial_store_get(spatial, entities, target,
                               &target_transform)) {
        return false;
    }

    dx = (double)target_transform.position.x -
         (double)enemy_transform.position.x;
    dy = (double)target_transform.position.y -
         (double)enemy_transform.position.y;
    dz = (double)target_transform.position.z -
         (double)enemy_transform.position.z;
    if (dx == 0.0 && dy == 0.0 && dz == 0.0) {
        return true;
    }

    length_squared = dx * dx + dy * dy + dz * dz;
    length = sqrt(length_squared);
    out_direction->x = (float)(dx / length);
    out_direction->y = (float)(dy / length);
    out_direction->z = (float)(dz / length);
    return true;
}
