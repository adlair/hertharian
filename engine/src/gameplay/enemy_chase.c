#include "enemy_chase.h"

#include <float.h>
#include <math.h>

static bool vector_is_finite(HTHVec3 vector)
{
    return isfinite(vector.x) && isfinite(vector.y) && isfinite(vector.z);
}

static bool scaled_component(float component, float speed,
                             float *out_component)
{
    double product = (double)component * (double)speed;

    if (!isfinite(product) || product > (double)FLT_MAX ||
        product < -(double)FLT_MAX) {
        return false;
    }
    *out_component = (float)product;
    return isfinite(*out_component);
}

bool hth_enemy_chase_apply(
    const HTHEntityRegistry *entities,
    const HTHActorStore *actors,
    const HTHEnemyStore *enemies,
    HTHSpatialStore *spatial,
    HTHDynamicBodyStore *bodies,
    const HTHCollisionWorld *collision_world,
    HTHEntityHandle enemy,
    HTHVec3 desired_direction,
    float chase_speed,
    float delta_seconds,
    HTHDynamicCollisionResult *out_result)
{
    const HTHDynamicCollisionResult empty_result = {false, false, false};
    HTHDynamicBody previous_body;
    HTHVec3 desired_velocity;

    if (out_result != NULL) {
        *out_result = empty_result;
    }
    if (entities == NULL || actors == NULL || enemies == NULL ||
        spatial == NULL || bodies == NULL || out_result == NULL ||
        !hth_collision_world_is_valid(collision_world) ||
        !vector_is_finite(desired_direction) || !isfinite(chase_speed) ||
        chase_speed < 0.0F || !isfinite(delta_seconds) ||
        delta_seconds < 0.0F ||
        !hth_enemy_store_has(enemies, entities, actors, enemy) ||
        !hth_spatial_store_has(spatial, entities, enemy) ||
        !hth_dynamic_body_get(bodies, entities, enemy, &previous_body) ||
        !scaled_component(desired_direction.x, chase_speed,
                          &desired_velocity.x) ||
        !scaled_component(desired_direction.y, chase_speed,
                          &desired_velocity.y) ||
        !scaled_component(desired_direction.z, chase_speed,
                          &desired_velocity.z)) {
        return false;
    }

    if (!hth_dynamic_body_set_velocity(
            bodies, entities, enemy, desired_velocity)) {
        return false;
    }
    if (!hth_dynamic_collision_move(
            bodies, entities, spatial, collision_world, enemy,
            delta_seconds, out_result)) {
        (void)hth_dynamic_body_set_velocity(
            bodies, entities, enemy, previous_body.velocity);
        *out_result = empty_result;
        return false;
    }
    return true;
}
