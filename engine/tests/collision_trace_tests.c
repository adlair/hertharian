#include "collision_trace.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>

static const HTHVec3 player_mins = {-0.5F, -0.5F, -0.5F};
static const HTHVec3 player_maxs = {0.5F, 0.5F, 0.5F};

static bool close_float(float left, float right)
{
    return fabsf(left - right) <= 1.0e-5F;
}

static bool close_vec3(HTHVec3 left, HTHVec3 right)
{
    return close_float(left.x, right.x) &&
           close_float(left.y, right.y) &&
           close_float(left.z, right.z);
}

static HTHCollisionWorld world_with_box(HTHAABB box)
{
    HTHCollisionWorld world = {0};
    world.obstacles[0] = box;
    world.obstacle_count = 1;
    assert(hth_collision_world_is_valid(&world));
    return world;
}

static HTHTrace trace_box(const HTHCollisionWorld *world, HTHVec3 start,
                          HTHVec3 end)
{
    HTHTrace trace;
    assert(hth_collision_world_trace_aabb(
        world, start, end, player_mins, player_maxs, &trace));
    return trace;
}

static void test_no_collision(void)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{4.0F, -1.0F, -1.0F}, {5.0F, 1.0F, 1.0F}});
    HTHTrace trace = trace_box(
        &world, hth_vec3(0.0F, 3.0F, 0.0F),
        hth_vec3(10.0F, 3.0F, 0.0F));

    assert(!trace.hit);
    assert(!trace.start_solid);
    assert(!trace.all_solid);
    assert(close_float(trace.fraction, 1.0F));
    assert(close_vec3(trace.end_position, hth_vec3(10.0F, 3.0F, 0.0F)));
    assert(trace.obstacle_index == SIZE_MAX);
}

static void assert_face_trace(HTHVec3 start, HTHVec3 end,
                              HTHVec3 expected_normal)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}});
    HTHTrace trace = trace_box(&world, start, end);
    HTHVec3 expected_end = hth_vec3_add(
        start, hth_vec3_scale(hth_vec3_subtract(end, start), 0.25F));

    assert(trace.hit);
    assert(!trace.start_solid);
    assert(close_float(trace.fraction, 0.25F));
    assert(close_vec3(trace.normal, expected_normal));
    assert(close_vec3(trace.end_position, expected_end));
}

static void test_six_faces(void)
{
    assert_face_trace(hth_vec3(-3.0F, 0.0F, 0.0F),
                      hth_vec3(3.0F, 0.0F, 0.0F),
                      hth_vec3(-1.0F, 0.0F, 0.0F));
    assert_face_trace(hth_vec3(3.0F, 0.0F, 0.0F),
                      hth_vec3(-3.0F, 0.0F, 0.0F),
                      hth_vec3(1.0F, 0.0F, 0.0F));
    assert_face_trace(hth_vec3(0.0F, -3.0F, 0.0F),
                      hth_vec3(0.0F, 3.0F, 0.0F),
                      hth_vec3(0.0F, -1.0F, 0.0F));
    assert_face_trace(hth_vec3(0.0F, 3.0F, 0.0F),
                      hth_vec3(0.0F, -3.0F, 0.0F),
                      hth_vec3(0.0F, 1.0F, 0.0F));
    assert_face_trace(hth_vec3(0.0F, 0.0F, -3.0F),
                      hth_vec3(0.0F, 0.0F, 3.0F),
                      hth_vec3(0.0F, 0.0F, -1.0F));
    assert_face_trace(hth_vec3(0.0F, 0.0F, 3.0F),
                      hth_vec3(0.0F, 0.0F, -3.0F),
                      hth_vec3(0.0F, 0.0F, 1.0F));
}

static void test_known_fraction_and_extents(void)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{4.5F, -1.0F, -1.0F}, {5.5F, 1.0F, 1.0F}});
    HTHTrace trace = trace_box(
        &world, hth_vec3(0.0F, 0.0F, 0.0F),
        hth_vec3(10.0F, 0.0F, 0.0F));

    assert(trace.hit);
    assert(close_float(trace.fraction, 0.4F));
    assert(close_float(trace.end_position.x, 4.0F));
    assert(close_vec3(trace.normal, hth_vec3(-1.0F, 0.0F, 0.0F)));
}

static void test_earliest_hit(void)
{
    HTHCollisionWorld world = {0};
    HTHTrace trace;

    world.obstacles[0] = (HTHAABB){
        {5.0F, -1.0F, -1.0F}, {6.0F, 1.0F, 1.0F}
    };
    world.obstacles[1] = (HTHAABB){
        {2.0F, -1.0F, -1.0F}, {3.0F, 1.0F, 1.0F}
    };
    world.obstacle_count = 2;
    assert(hth_collision_world_trace_aabb(
        &world, hth_vec3(0.0F, 0.0F, 0.0F),
        hth_vec3(10.0F, 0.0F, 0.0F), player_mins, player_maxs, &trace));
    assert(trace.hit);
    assert(trace.obstacle_index == 1);
    assert(close_float(trace.fraction, 0.15F));
}

static void test_parallel_touching(void)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{1.0F, -1.0F, -1.0F}, {2.0F, 1.0F, 1.0F}});
    HTHTrace trace = trace_box(
        &world, hth_vec3(0.5F, 0.0F, -3.0F),
        hth_vec3(0.5F, 0.0F, 3.0F));

    assert(!trace.hit);
    assert(close_float(trace.fraction, 1.0F));
}

static void test_zero_motion_and_start_solid(void)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}});
    HTHTrace clear = trace_box(
        &world, hth_vec3(3.0F, 0.0F, 0.0F),
        hth_vec3(3.0F, 0.0F, 0.0F));
    HTHTrace solid = trace_box(
        &world, hth_vec3(0.0F, 0.0F, 0.0F),
        hth_vec3(0.0F, 0.0F, 0.0F));
    HTHTrace leaves = trace_box(
        &world, hth_vec3(0.0F, 0.0F, 0.0F),
        hth_vec3(3.0F, 0.0F, 0.0F));

    assert(!clear.hit && !clear.start_solid && !clear.all_solid);
    assert(solid.hit && solid.start_solid && solid.all_solid);
    assert(close_float(solid.fraction, 0.0F));
    assert(leaves.hit && leaves.start_solid && !leaves.all_solid);
}

static void test_floor_touching_and_probe(void)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{-10.0F, -1.0F, -10.0F}, {10.0F, 0.0F, 10.0F}});
    HTHVec3 feet_mins = {-0.3F, 0.0F, -0.3F};
    HTHVec3 feet_maxs = {0.3F, 1.8F, 0.3F};
    HTHTrace tangent;
    HTHTrace probe;

    assert(hth_collision_world_trace_aabb(
        &world, hth_vec3(0.0F, 0.0F, 0.0F),
        hth_vec3(1.0F, 0.0F, 0.0F), feet_mins, feet_maxs,
        &tangent));
    assert(!tangent.hit && !tangent.start_solid);
    assert(hth_collision_world_trace_aabb(
        &world, hth_vec3(0.0F, 0.0F, 0.0F),
        hth_vec3(0.0F, -0.04F, 0.0F), feet_mins, feet_maxs,
        &probe));
    assert(probe.hit && !probe.start_solid);
    assert(close_float(probe.fraction, 0.0F));
    assert(close_vec3(probe.normal, hth_vec3(0.0F, 1.0F, 0.0F)));
}

static void test_invalid_arguments_fail_cleanly(void)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}});
    HTHTrace trace;
    HTHVec3 invalid_maxs = {-0.5F, 0.5F, 0.5F};

    assert(!hth_collision_world_trace_aabb(
        &world, hth_vec3(0.0F, 0.0F, 0.0F),
        hth_vec3(1.0F, 0.0F, 0.0F), player_mins, invalid_maxs,
        &trace));
    assert(!hth_collision_world_trace_aabb(
        &world, hth_vec3(0.0F, 0.0F, 0.0F),
        hth_vec3(1.0F, 0.0F, 0.0F), player_mins, player_maxs,
        NULL));
}

int main(void)
{
    test_no_collision();
    test_six_faces();
    test_known_fraction_and_extents();
    test_earliest_hit();
    test_parallel_touching();
    test_zero_motion_and_start_solid();
    test_floor_touching_and_probe();
    test_invalid_arguments_fail_cleanly();
    return 0;
}
