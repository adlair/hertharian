#include "collision_trace.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,      \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

static bool close_float(float left, float right)
{
    float difference = left - right;

    if (difference < 0.0F) {
        difference = -difference;
    }
    return difference <= 1.0e-5F;
}

static bool equal_vec3(HTHVec3 left, HTHVec3 right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

static HTHCollisionWorld world_with_box(HTHAABB box)
{
    HTHCollisionWorld world = {0};

    world.obstacles[0] = box;
    world.obstacle_count = 1U;
    return world;
}

static bool add_box(HTHCollisionWorld *world, HTHAABB box)
{
    if (world == NULL || !hth_aabb_is_valid(&box) ||
        world->obstacle_count >= HTH_COLLISION_WORLD_MAX_OBSTACLES) {
        return false;
    }
    world->obstacles[world->obstacle_count++] = box;
    return true;
}

static bool trace_segment(const HTHCollisionWorld *world, HTHVec3 start,
                          HTHVec3 end, HTHTrace *out_trace)
{
    return hth_collision_world_trace_segment(world, start, end, out_trace);
}

static bool canonical_no_hit(HTHTrace trace, HTHVec3 end)
{
    return !trace.hit && !trace.start_solid && !trace.all_solid &&
           trace.fraction == 1.0F && equal_vec3(trace.end_position, end) &&
           equal_vec3(trace.normal, hth_vec3(0.0F, 0.0F, 0.0F)) &&
           trace.obstacle_index == SIZE_MAX;
}

static bool test_null_invalid_and_nonfinite_contracts(void)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{1.0F, -1.0F, -1.0F}, {2.0F, 1.0F, 1.0F}});
    HTHCollisionWorld invalid_world = {0};
    HTHTrace marker;
    HTHTrace output;

    memset(&marker, 0x5a, sizeof(marker));
    output = marker;
    CHECK(!trace_segment(NULL, hth_vec3(0.0F, 0.0F, 0.0F),
                         hth_vec3(3.0F, 0.0F, 0.0F), &output));
    CHECK(memcmp(&output, &marker, sizeof(output)) == 0);
    CHECK(!trace_segment(&world, hth_vec3(0.0F, 0.0F, 0.0F),
                         hth_vec3(3.0F, 0.0F, 0.0F), NULL));
    CHECK(!trace_segment(&world, hth_vec3(NAN, 0.0F, 0.0F),
                         hth_vec3(3.0F, 0.0F, 0.0F), &output));
    CHECK(!trace_segment(&world, hth_vec3(INFINITY, 0.0F, 0.0F),
                         hth_vec3(3.0F, 0.0F, 0.0F), &output));
    CHECK(!trace_segment(&world, hth_vec3(-INFINITY, 0.0F, 0.0F),
                         hth_vec3(3.0F, 0.0F, 0.0F), &output));
    CHECK(!trace_segment(&world, hth_vec3(0.0F, 0.0F, 0.0F),
                         hth_vec3(NAN, 0.0F, 0.0F), &output));
    CHECK(!trace_segment(&world, hth_vec3(0.0F, 0.0F, 0.0F),
                         hth_vec3(INFINITY, 0.0F, 0.0F), &output));
    CHECK(!trace_segment(&world, hth_vec3(0.0F, 0.0F, 0.0F),
                         hth_vec3(-INFINITY, 0.0F, 0.0F), &output));
    invalid_world.obstacle_count =
        HTH_COLLISION_WORLD_MAX_OBSTACLES + 1U;
    CHECK(!trace_segment(&invalid_world, hth_vec3(0.0F, 0.0F, 0.0F),
                         hth_vec3(3.0F, 0.0F, 0.0F), &output));
    invalid_world = (HTHCollisionWorld){0};
    invalid_world.obstacles[0] =
        (HTHAABB){{1.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}};
    invalid_world.obstacle_count = 1U;
    CHECK(!trace_segment(&invalid_world, hth_vec3(0.0F, 0.0F, 0.0F),
                         hth_vec3(3.0F, 0.0F, 0.0F), &output));
    return true;
}

static bool test_empty_world_and_basic_hit(void)
{
    HTHCollisionWorld empty_world = {0};
    HTHCollisionWorld blocked_world = world_with_box(
        (HTHAABB){{2.0F, -1.0F, -1.0F}, {4.0F, 1.0F, 1.0F}});
    HTHVec3 start = hth_vec3(0.0F, 0.0F, 0.0F);
    HTHVec3 end = hth_vec3(10.0F, 0.0F, 0.0F);
    HTHTrace trace;

    CHECK(trace_segment(&empty_world, start, end, &trace));
    CHECK(canonical_no_hit(trace, end));
    CHECK(trace_segment(&blocked_world, start, end, &trace));
    CHECK(trace.hit && !trace.start_solid && !trace.all_solid);
    CHECK(close_float(trace.fraction, 0.2F));
    CHECK(equal_vec3(trace.end_position, hth_vec3(2.0F, 0.0F, 0.0F)));
    CHECK(equal_vec3(trace.normal, hth_vec3(-1.0F, 0.0F, 0.0F)));
    CHECK(trace.obstacle_index == 0U);
    return true;
}

static bool test_finite_domain_and_endpoint(void)
{
    HTHTrace trace;
    HTHCollisionWorld endpoint = world_with_box(
        (HTHAABB){{2.0F, -1.0F, -1.0F}, {3.0F, 1.0F, 1.0F}});
    HTHCollisionWorld behind = world_with_box(
        (HTHAABB){{3.0F, -1.0F, -1.0F}, {4.0F, 1.0F, 1.0F}});
    HTHCollisionWorld before = world_with_box(
        (HTHAABB){{-3.0F, -1.0F, -1.0F}, {-2.0F, 1.0F, 1.0F}});
    HTHCollisionWorld off_axis = world_with_box(
        (HTHAABB){{1.0F, 2.0F, -1.0F}, {2.0F, 3.0F, 1.0F}});
    HTHVec3 start = hth_vec3(0.0F, 0.0F, 0.0F);
    HTHVec3 end = hth_vec3(2.0F, 0.0F, 0.0F);

    CHECK(trace_segment(&endpoint, start, end, &trace));
    CHECK(trace.hit && trace.fraction == 1.0F &&
          trace.obstacle_index == 0U);
    CHECK(equal_vec3(trace.end_position, end));
    CHECK(equal_vec3(trace.normal, hth_vec3(-1.0F, 0.0F, 0.0F)));
    CHECK(trace_segment(&behind, start, end, &trace));
    CHECK(canonical_no_hit(trace, end));
    CHECK(trace_segment(&before, start, end, &trace));
    CHECK(canonical_no_hit(trace, end));
    end = hth_vec3(4.0F, 0.0F, 0.0F);
    CHECK(trace_segment(&off_axis, start, end, &trace));
    CHECK(canonical_no_hit(trace, end));
    return true;
}

static bool test_zero_length_and_start_solid(void)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}});
    HTHTrace trace;
    HTHVec3 outside = hth_vec3(3.0F, 0.0F, 0.0F);
    HTHVec3 inside = hth_vec3(0.0F, 0.0F, 0.0F);
    HTHVec3 boundary = hth_vec3(1.0F, 0.0F, 0.0F);
    HTHVec3 exit = hth_vec3(3.0F, 0.0F, 0.0F);
    HTHVec3 remains_inside = hth_vec3(0.5F, 0.0F, 0.0F);

    CHECK(trace_segment(&world, outside, outside, &trace));
    CHECK(canonical_no_hit(trace, outside));
    CHECK(trace_segment(&world, inside, inside, &trace));
    CHECK(trace.hit && trace.start_solid && trace.all_solid &&
          trace.fraction == 0.0F && trace.obstacle_index == 0U);
    CHECK(equal_vec3(trace.end_position, inside));
    CHECK(trace_segment(&world, boundary, boundary, &trace));
    CHECK(canonical_no_hit(trace, boundary));
    CHECK(trace_segment(&world, inside, exit, &trace));
    CHECK(trace.hit && trace.start_solid && !trace.all_solid &&
          trace.fraction == 0.0F);
    CHECK(trace_segment(&world, inside, remains_inside, &trace));
    CHECK(trace.hit && trace.start_solid && trace.all_solid &&
          trace.fraction == 0.0F);
    return true;
}

static bool check_axis_trace(HTHVec3 start, HTHVec3 end,
                             HTHVec3 expected_normal)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{-1.0F, -1.0F, -1.0F}, {1.0F, 1.0F, 1.0F}});
    HTHTrace trace;

    return trace_segment(&world, start, end, &trace) && trace.hit &&
           !trace.start_solid && close_float(trace.fraction, 1.0F / 3.0F) &&
           equal_vec3(trace.normal, expected_normal);
}

static bool test_axes_and_negative_direction(void)
{
    CHECK(check_axis_trace(hth_vec3(-3.0F, 0.0F, 0.0F),
                           hth_vec3(3.0F, 0.0F, 0.0F),
                           hth_vec3(-1.0F, 0.0F, 0.0F)));
    CHECK(check_axis_trace(hth_vec3(0.0F, -3.0F, 0.0F),
                           hth_vec3(0.0F, 3.0F, 0.0F),
                           hth_vec3(0.0F, -1.0F, 0.0F)));
    CHECK(check_axis_trace(hth_vec3(0.0F, 0.0F, -3.0F),
                           hth_vec3(0.0F, 0.0F, 3.0F),
                           hth_vec3(0.0F, 0.0F, -1.0F)));
    CHECK(check_axis_trace(hth_vec3(3.0F, 0.0F, 0.0F),
                           hth_vec3(-3.0F, 0.0F, 0.0F),
                           hth_vec3(1.0F, 0.0F, 0.0F)));
    return true;
}

static bool test_diagonal_edge_corner_and_parallel(void)
{
    HTHCollisionWorld diagonal = world_with_box(
        (HTHAABB){{1.0F, 1.0F, -1.0F}, {2.0F, 2.0F, 1.0F}});
    HTHCollisionWorld corner = world_with_box(
        (HTHAABB){{1.0F, 1.0F, 1.0F}, {2.0F, 2.0F, 2.0F}});
    HTHCollisionWorld parallel = world_with_box(
        (HTHAABB){{1.0F, -1.0F, -1.0F}, {2.0F, 1.0F, 1.0F}});
    HTHTrace trace;

    CHECK(trace_segment(&diagonal, hth_vec3(0.0F, 0.0F, 0.0F),
                        hth_vec3(3.0F, 3.0F, 0.0F), &trace));
    CHECK(trace.hit && close_float(trace.fraction, 1.0F / 3.0F));
    CHECK(equal_vec3(trace.normal, hth_vec3(-1.0F, 0.0F, 0.0F)));
    CHECK(trace_segment(&corner, hth_vec3(0.0F, 0.0F, 0.0F),
                        hth_vec3(3.0F, 3.0F, 3.0F), &trace));
    CHECK(trace.hit && close_float(trace.fraction, 1.0F / 3.0F));
    CHECK(equal_vec3(trace.normal, hth_vec3(-1.0F, 0.0F, 0.0F)));
    CHECK(trace_segment(&parallel, hth_vec3(0.0F, 2.0F, 0.0F),
                        hth_vec3(3.0F, 2.0F, 0.0F), &trace));
    CHECK(canonical_no_hit(trace, hth_vec3(3.0F, 2.0F, 0.0F)));
    CHECK(trace_segment(&parallel, hth_vec3(0.0F, 0.0F, 0.0F),
                        hth_vec3(3.0F, 0.0F, 0.0F), &trace));
    CHECK(trace.hit && close_float(trace.fraction, 1.0F / 3.0F));
    CHECK(trace_segment(&parallel, hth_vec3(0.0F, 1.0F, 0.0F),
                        hth_vec3(3.0F, 1.0F, 0.0F), &trace));
    CHECK(canonical_no_hit(trace, hth_vec3(3.0F, 1.0F, 0.0F)));
    return true;
}

static bool test_multiple_obstacles_and_insertion_order(void)
{
    const HTHAABB near_box =
        {{2.0F, -1.0F, -1.0F}, {3.0F, 1.0F, 1.0F}};
    const HTHAABB far_box =
        {{6.0F, -1.0F, -1.0F}, {7.0F, 1.0F, 1.0F}};
    HTHCollisionWorld first = {0};
    HTHCollisionWorld reversed = {0};
    HTHCollisionWorld tied = {0};
    HTHTrace trace;
    HTHVec3 start = hth_vec3(0.0F, 0.0F, 0.0F);
    HTHVec3 end = hth_vec3(10.0F, 0.0F, 0.0F);

    CHECK(add_box(&first, far_box));
    CHECK(add_box(&first, near_box));
    CHECK(trace_segment(&first, start, end, &trace));
    CHECK(trace.hit && trace.obstacle_index == 1U &&
          close_float(trace.fraction, 0.2F));
    CHECK(add_box(&reversed, near_box));
    CHECK(add_box(&reversed, far_box));
    CHECK(trace_segment(&reversed, start, end, &trace));
    CHECK(trace.hit && trace.obstacle_index == 0U &&
          close_float(trace.fraction, 0.2F));
    CHECK(add_box(&tied, near_box));
    CHECK(add_box(&tied, near_box));
    CHECK(trace_segment(&tied, start, end, &trace));
    CHECK(trace.hit && trace.obstacle_index == 0U);
    return true;
}

static bool test_extreme_finite_coordinates(void)
{
    HTHCollisionWorld world = world_with_box(
        (HTHAABB){{-1.0e37F, -1.0F, -1.0F},
                  {1.0e37F, 1.0F, 1.0F}});
    HTHVec3 start = hth_vec3(-3.0e38F, 0.0F, 0.0F);
    HTHVec3 end = hth_vec3(3.0e38F, 0.0F, 0.0F);
    HTHTrace trace;

    CHECK(trace_segment(&world, start, end, &trace));
    CHECK(trace.hit && !trace.start_solid && isfinite(trace.fraction));
    CHECK(close_float(trace.fraction, 0.48333332F));
    CHECK(equal_vec3(trace.normal, hth_vec3(-1.0F, 0.0F, 0.0F)));
    return true;
}

static bool traces_equal(HTHTrace left, HTHTrace right)
{
    return left.hit == right.hit &&
           left.start_solid == right.start_solid &&
           left.all_solid == right.all_solid &&
           left.fraction == right.fraction &&
           equal_vec3(left.end_position, right.end_position) &&
           equal_vec3(left.normal, right.normal) &&
           left.obstacle_index == right.obstacle_index;
}

static bool test_purity_independent_worlds_and_aabb_contract(void)
{
    HTHCollisionWorld clear = {0};
    HTHCollisionWorld blocked = world_with_box(
        (HTHAABB){{2.0F, -1.0F, -1.0F}, {3.0F, 1.0F, 1.0F}});
    HTHCollisionWorld before = blocked;
    HTHTrace first;
    HTHTrace repeated;
    HTHTrace clear_trace;
    HTHVec3 zero = hth_vec3(0.0F, 0.0F, 0.0F);

    CHECK(trace_segment(&blocked, zero, hth_vec3(5.0F, 0.0F, 0.0F),
                        &first));
    CHECK(trace_segment(&blocked, zero, hth_vec3(5.0F, 0.0F, 0.0F),
                        &repeated));
    CHECK(traces_equal(first, repeated));
    CHECK(memcmp(&blocked, &before, sizeof(blocked)) == 0);
    CHECK(trace_segment(&clear, zero, hth_vec3(5.0F, 0.0F, 0.0F),
                        &clear_trace));
    CHECK(canonical_no_hit(clear_trace, hth_vec3(5.0F, 0.0F, 0.0F)));
    CHECK(first.hit);
    CHECK(!hth_collision_world_trace_aabb(
        &blocked, zero, hth_vec3(5.0F, 0.0F, 0.0F), zero, zero,
        &repeated));
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_null_invalid_and_nonfinite_contracts,
        test_empty_world_and_basic_hit,
        test_finite_domain_and_endpoint,
        test_zero_length_and_start_solid,
        test_axes_and_negative_direction,
        test_diagonal_edge_corner_and_parallel,
        test_multiple_obstacles_and_insertion_order,
        test_extreme_finite_coordinates,
        test_purity_independent_worlds_and_aabb_contract
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("collision segment tests passed");
    return EXIT_SUCCESS;
}
