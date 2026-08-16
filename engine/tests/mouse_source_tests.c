#include "mouse_source.h"

#include <assert.h>
#include <stddef.h>

static void test_reentry_compensation_pair(void)
{
    HTHRelativeMouseFilter filter = {0};
    double x;
    double y;

    hth_relative_mouse_filter_reset(&filter);
    assert(hth_relative_mouse_filter_motion(
        &filter, true, true, 7U, 6U, 981.0, 709.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE);
    assert(hth_relative_mouse_filter_motion(
        &filter, true, true, 7U, 7U, -980.0, -710.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_DISCARD_REENTRY_COMPENSATION);
    assert(x == 1.0 && y == -1.0);
    assert(hth_relative_mouse_filter_motion(
        &filter, true, true, 7U, 7U, 4.0, 0.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
    assert(x == 5.0 && y == -1.0);
}

static void test_repeated_reentry_cycles(void)
{
    HTHRelativeMouseFilter filter = {0};
    double x;
    double y;
    double previous_sample = 4.0;
    unsigned int cycle;

    assert(hth_relative_mouse_filter_motion(
        &filter, true, true, 7U, 7U, previous_sample, 0.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
    for (cycle = 0; cycle < 5U; ++cycle) {
        hth_relative_mouse_filter_cancel_transition(&filter);
        assert(hth_relative_mouse_filter_motion(
            &filter, true, true, 7U, 6U, 1000.0 - previous_sample,
            600.0 - y, &x, &y) ==
            HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE);
        assert(hth_relative_mouse_filter_motion(
            &filter, true, true, 7U, 7U, -999.0, -601.0, &x, &y) ==
            HTH_RELATIVE_MOUSE_MOTION_DISCARD_REENTRY_COMPENSATION);
        assert(hth_relative_mouse_filter_motion(
            &filter, true, true, 7U, 7U, 2.0, 3.0, &x, &y) ==
            HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
        assert(x == 3.0 && y == 2.0);
        previous_sample = x;
    }
}

static void test_sustained_motion_and_immediate_reversal(void)
{
    static const double delivered_x[] = {4.0, -2.0, -1.0, 2.0, -5.0};
    static const double expected_x[] = {4.0, 2.0, 1.0, 3.0, -2.0};
    HTHRelativeMouseFilter filter = {0};
    double x;
    double y;
    size_t index;

    for (index = 0; index < sizeof(delivered_x) / sizeof(delivered_x[0]);
         ++index) {
        assert(hth_relative_mouse_filter_motion(
            &filter, true, true, 7U, 7U, delivered_x[index], 0.0,
            &x, &y) == HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
        assert(x == expected_x[index] && y == 0.0);
    }

    hth_relative_mouse_filter_reset(&filter);
    assert(hth_relative_mouse_filter_motion(
        &filter, true, true, 7U, 7U, -4.0, -3.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
    assert(x == -4.0 && y == -3.0); /* Right and up diagonal. */
    assert(hth_relative_mouse_filter_motion(
        &filter, true, true, 7U, 7U, 2.0, 1.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
    assert(x == -2.0 && y == -2.0);
    assert(hth_relative_mouse_filter_motion(
        &filter, true, true, 7U, 7U, 3.0, 5.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
    assert(x == 1.0 && y == 3.0); /* Immediate left/down reversal. */
}

static void test_generic_and_inactive_paths_are_unchanged(void)
{
    HTHRelativeMouseFilter filter = {0};
    double x;
    double y;

    assert(hth_relative_mouse_filter_motion(
        &filter, true, false, 0U, 3U, 8.0, -6.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
    assert(x == 8.0 && y == -6.0);
    assert(hth_relative_mouse_filter_motion(
        &filter, false, true, 7U, 6U, -2.0, 5.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
    assert(x == -2.0 && y == 5.0);
    assert(hth_relative_mouse_filter_motion(
        &filter, true, true, 7U, 7U, 1.0, 2.0, &x, &y) ==
        HTH_RELATIVE_MOUSE_MOTION_ACCEPT);
    assert(x == 1.0 && y == 2.0);
}

int main(void)
{
    assert(hth_mouse_name_is_explicit_relative_source(
        "xwayland-relative-pointer:16"));
    assert(hth_mouse_name_is_explicit_relative_source(
        "xwayland-relative-pointer:2048"));
    assert(hth_mouse_name_is_explicit_relative_source(
        "xwayland-relative-pointer"));
    assert(!hth_mouse_name_is_explicit_relative_source(
        "xwayland-pointer:16"));
    assert(!hth_mouse_name_is_explicit_relative_source(
        "xwayland-pointer-gestures:16"));
    assert(!hth_mouse_name_is_explicit_relative_source(
        "xwayland-relative-pointer-extra:16"));
    assert(!hth_mouse_name_is_explicit_relative_source("Generic Mouse"));
    assert(!hth_mouse_name_is_explicit_relative_source(NULL));
    test_reentry_compensation_pair();
    test_repeated_reentry_cycles();
    test_sustained_motion_and_immediate_reversal();
    test_generic_and_inactive_paths_are_unchanged();
    return 0;
}
