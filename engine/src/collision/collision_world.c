#include "collision_world.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static const HTHAABB bootstrap_obstacles[] = {
    {{-20.0F, -1.0F, -20.0F}, {20.0F, 0.0F, 20.0F}},
    {{-5.0F, 0.0F, -12.0F}, {-4.0F, 3.0F, 1.0F}},
    {{4.0F, 0.0F, -12.0F}, {5.0F, 3.0F, 1.0F}},
    {{-1.0F, 0.0F, -6.0F}, {1.0F, 1.5F, -4.5F}},
    {{-3.2F, 0.0F, -3.5F}, {-2.0F, 1.2F, -2.3F}},
    {{2.0F, 0.0F, -9.0F}, {3.2F, 2.0F, -7.8F}},
};

static bool finite_vec3(HTHVec3 vector)
{
    return isfinite(vector.x) && isfinite(vector.y) && isfinite(vector.z);
}

bool hth_collision_world_init_bootstrap(HTHCollisionWorld *world)
{
    size_t index;

    if (world == NULL ||
        sizeof(bootstrap_obstacles) / sizeof(bootstrap_obstacles[0]) >
            HTH_COLLISION_WORLD_MAX_OBSTACLES) {
        return false;
    }
    memset(world, 0, sizeof(*world));
    world->obstacle_count =
        sizeof(bootstrap_obstacles) / sizeof(bootstrap_obstacles[0]);
    for (index = 0; index < world->obstacle_count; ++index) {
        world->obstacles[index] = bootstrap_obstacles[index];
    }
    return hth_collision_world_is_valid(world);
}

bool hth_collision_world_is_valid(const HTHCollisionWorld *world)
{
    size_t index;

    if (world == NULL || world->obstacle_count == 0 ||
        world->obstacle_count > HTH_COLLISION_WORLD_MAX_OBSTACLES) {
        return false;
    }
    for (index = 0; index < world->obstacle_count; ++index) {
        if (!hth_aabb_is_valid(&world->obstacles[index])) {
            return false;
        }
    }
    return true;
}

bool hth_collision_world_body_penetrates(const HTHCollisionWorld *world,
                                         const HTHPlayerBody *body)
{
    HTHAABB body_bounds;
    size_t index;

    if (!hth_collision_world_is_valid(world) ||
        !hth_player_body_bounds(body, &body_bounds)) {
        return true;
    }
    for (index = 0; index < world->obstacle_count; ++index) {
        if (hth_aabb_intersects(&body_bounds, &world->obstacles[index])) {
            return true;
        }
    }
    return false;
}

static bool resolve_x(const HTHCollisionWorld *world, HTHPlayerBody *body,
                      float displacement)
{
    HTHAABB bounds;
    size_t index;

    body->position.x += displacement;
    if (!hth_player_body_bounds(body, &bounds)) {
        return false;
    }
    for (index = 0; index < world->obstacle_count; ++index) {
        const HTHAABB *obstacle = &world->obstacles[index];
        if (!hth_aabb_intersects(&bounds, obstacle)) {
            continue;
        }
        body->position.x = displacement > 0.0F
            ? obstacle->min.x - body->half_width
            : obstacle->max.x + body->half_width;
        body->velocity.x = 0.0F;
        if (!hth_player_body_bounds(body, &bounds)) {
            return false;
        }
    }
    return true;
}

static bool resolve_y(const HTHCollisionWorld *world, HTHPlayerBody *body,
                      float displacement)
{
    HTHAABB bounds;
    HTHAABB previous_bounds;
    size_t index;

    if (!hth_player_body_bounds(body, &previous_bounds)) {
        return false;
    }
    body->grounded = false;
    body->position.y += displacement;
    if (!hth_player_body_bounds(body, &bounds)) {
        return false;
    }
    for (index = 0; index < world->obstacle_count; ++index) {
        const HTHAABB *obstacle = &world->obstacles[index];
        if (!hth_aabb_intersects(&bounds, obstacle)) {
            continue;
        }
        if (displacement < 0.0F && body->velocity.y <= 0.0F &&
            previous_bounds.min.y >= obstacle->max.y &&
            bounds.min.y < obstacle->max.y) {
            body->position.y = obstacle->max.y;
            body->grounded = true;
        } else if (displacement > 0.0F && body->velocity.y > 0.0F &&
                   previous_bounds.max.y <= obstacle->min.y &&
                   bounds.max.y > obstacle->min.y) {
            body->position.y = obstacle->min.y - body->height;
        } else {
            /* A lateral overlap is not a vertical landing or ceiling hit. */
            continue;
        }
        body->velocity.y = 0.0F;
        if (!hth_player_body_bounds(body, &bounds)) {
            return false;
        }
    }
    return true;
}

static bool resolve_z(const HTHCollisionWorld *world, HTHPlayerBody *body,
                      float displacement)
{
    HTHAABB bounds;
    size_t index;

    body->position.z += displacement;
    if (!hth_player_body_bounds(body, &bounds)) {
        return false;
    }
    for (index = 0; index < world->obstacle_count; ++index) {
        const HTHAABB *obstacle = &world->obstacles[index];
        if (!hth_aabb_intersects(&bounds, obstacle)) {
            continue;
        }
        body->position.z = displacement > 0.0F
            ? obstacle->min.z - body->half_width
            : obstacle->max.z + body->half_width;
        body->velocity.z = 0.0F;
        if (!hth_player_body_bounds(body, &bounds)) {
            return false;
        }
    }
    return true;
}

bool hth_collision_world_move_body(const HTHCollisionWorld *world,
                                   HTHPlayerBody *body,
                                   HTHVec3 displacement)
{
    if (!hth_collision_world_is_valid(world) ||
        !hth_player_body_is_valid(body) || !finite_vec3(displacement)) {
        return false;
    }

    return resolve_x(world, body, displacement.x) &&
           resolve_y(world, body, displacement.y) &&
           resolve_z(world, body, displacement.z);
}
