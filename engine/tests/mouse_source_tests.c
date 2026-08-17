#include "mouse_source.h"

#include <assert.h>
#include <stddef.h>

static void expect_motion(HTHRelativeMouseFilter *filter,
                          bool relative_mode,
                          bool source_known,
                          uint32_t selected_source,
                          uint32_t event_source,
                          double delivered_x,
                          double delivered_y,
                          HTHRelativeMouseMotionDecision expected_decision,
                          double expected_x,
                          double expected_y)
{
    double corrected_x;
    double corrected_y;

    assert(hth_relative_mouse_filter_motion(
               filter, relative_mode, source_known, selected_source,
               event_source, delivered_x, delivered_y, &corrected_x,
               &corrected_y) == expected_decision);
    assert(corrected_x == expected_x);
    assert(corrected_y == expected_y);
}

static void test_real_reentry_sequence_is_one_to_one(void)
{
    HTHRelativeMouseFilter filter = {0};

    expect_motion(&filter, true, true, 7U, 6U, 1050.0, 753.0,
                  HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE,
                  0.0, 0.0);
    assert(filter.compensation_pending);
    expect_motion(&filter, true, true, 7U, 7U, 0.0, -3.0,
                  HTH_RELATIVE_MOUSE_MOTION_DISCARD_REENTRY_COMPENSATION,
                  0.0, 0.0);
    assert(!filter.compensation_pending);
    expect_motion(&filter, true, true, 7U, 7U, 0.0, -6.0,
                  HTH_RELATIVE_MOUSE_MOTION_ACCEPT, 0.0, -6.0);
    expect_motion(&filter, true, true, 7U, 7U, 0.0, -5.0,
                  HTH_RELATIVE_MOUSE_MOTION_ACCEPT, 0.0, -5.0);
    expect_motion(&filter, true, true, 7U, 7U, 0.0, -7.0,
                  HTH_RELATIVE_MOUSE_MOTION_ACCEPT, 0.0, -7.0);
}

static void test_small_movements_do_not_accumulate(void)
{
    static const double samples[][2] = {
        {-1.0, 0.0}, {-1.0, 0.0}, {-3.0, 0.0},
        {-8.0, -1.0}, {-8.0, 0.0}
    };
    HTHRelativeMouseFilter filter = {0};
    size_t index;

    for (index = 0U; index < sizeof(samples) / sizeof(samples[0]); ++index) {
        expect_motion(&filter, true, true, 7U, 7U,
                      samples[index][0], samples[index][1],
                      HTH_RELATIVE_MOUSE_MOTION_ACCEPT,
                      samples[index][0], samples[index][1]);
    }
}

static void test_alternating_movements_are_one_to_one(void)
{
    static const double samples[][2] = {
        {4.0, -2.0}, {-1.0, 2.0}, {1.0, -1.0}, {-3.0, 4.0}
    };
    HTHRelativeMouseFilter filter = {0};
    size_t index;

    for (index = 0U; index < sizeof(samples) / sizeof(samples[0]); ++index) {
        expect_motion(&filter, true, true, 7U, 7U,
                      samples[index][0], samples[index][1],
                      HTH_RELATIVE_MOUSE_MOTION_ACCEPT,
                      samples[index][0], samples[index][1]);
    }
}

static void test_axis_independence(void)
{
    static const double samples[][2] = {
        {1.0, 0.0}, {2.0, 0.0}, {-1.0, 0.0}, {-2.0, 0.0},
        {0.0, 1.0}, {0.0, 2.0}, {0.0, -1.0}, {0.0, -2.0},
        {1.0, 1.0}, {-1.0, -1.0}, {2.0, -2.0}, {-2.0, 2.0}
    };
    HTHRelativeMouseFilter filter = {0};
    size_t index;

    for (index = 0U; index < sizeof(samples) / sizeof(samples[0]); ++index) {
        expect_motion(&filter, true, true, 7U, 7U,
                      samples[index][0], samples[index][1],
                      HTH_RELATIVE_MOUSE_MOTION_ACCEPT,
                      samples[index][0], samples[index][1]);
    }
}

static void test_foreign_payload_is_never_emitted(void)
{
    static const double foreign[][2] = {
        {939.0, 632.0}, {-929.0, -659.0},
        {1.0, 1.0}, {100000.0, -100000.0}
    };
    size_t index;

    for (index = 0U; index < sizeof(foreign) / sizeof(foreign[0]); ++index) {
        HTHRelativeMouseFilter filter = {0};

        expect_motion(&filter, true, true, 7U, 6U,
                      foreign[index][0], foreign[index][1],
                      HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE,
                      0.0, 0.0);
        expect_motion(&filter, true, true, 7U, 7U, 3.0, 5.0,
                      HTH_RELATIVE_MOUSE_MOTION_DISCARD_REENTRY_COMPENSATION,
                      0.0, 0.0);
        expect_motion(&filter, true, true, 7U, 7U, 4.0, -2.0,
                      HTH_RELATIVE_MOUSE_MOTION_ACCEPT, 4.0, -2.0);
    }
}

static void test_repeated_reentry_cycles(void)
{
    HTHRelativeMouseFilter filter = {0};
    unsigned int cycle;

    for (cycle = 0U; cycle < 5U; ++cycle) {
        expect_motion(&filter, true, true, 7U, 7U, 1.0, -1.0,
                      HTH_RELATIVE_MOUSE_MOTION_ACCEPT, 1.0, -1.0);
        expect_motion(&filter, true, true, 7U, 6U,
                      1000.0 + (double)cycle,
                      600.0 - (double)cycle,
                      HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE,
                      0.0, 0.0);
        expect_motion(&filter, true, true, 7U, 7U, 3.0, 2.0,
                      HTH_RELATIVE_MOUSE_MOTION_DISCARD_REENTRY_COMPENSATION,
                      0.0, 0.0);
        expect_motion(&filter, true, true, 7U, 7U,
                      4.0 + (double)cycle, -2.0 - (double)cycle,
                      HTH_RELATIVE_MOUSE_MOTION_ACCEPT,
                      4.0 + (double)cycle, -2.0 - (double)cycle);
    }
}

static void test_capture_focus_and_source_transitions(void)
{
    HTHRelativeMouseFilter filter = {0};

    expect_motion(&filter, true, true, 7U, 7U, 5.0, 6.0,
                  HTH_RELATIVE_MOUSE_MOTION_ACCEPT, 5.0, 6.0);
    expect_motion(&filter, true, true, 7U, 6U, 900.0, 700.0,
                  HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE,
                  0.0, 0.0);
    hth_relative_mouse_filter_reset(&filter); /* Capture exit. */
    hth_relative_mouse_filter_reset(&filter); /* Capture re-entry. */
    assert(!filter.compensation_pending);
    expect_motion(&filter, true, true, 7U, 7U, 2.0, -1.0,
                  HTH_RELATIVE_MOUSE_MOTION_ACCEPT, 2.0, -1.0);

    expect_motion(&filter, true, true, 7U, 6U, -5000.0, 4000.0,
                  HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE,
                  0.0, 0.0);
    hth_relative_mouse_filter_cancel_transition(&filter); /* Focus loss. */
    hth_relative_mouse_filter_cancel_transition(&filter); /* Focus gain. */
    hth_relative_mouse_filter_cancel_transition(&filter); /* Mouse loss. */
    hth_relative_mouse_filter_cancel_transition(&filter); /* Mouse gain. */
    assert(!filter.compensation_pending);
    expect_motion(&filter, true, true, 7U, 7U, 2.0, -1.0,
                  HTH_RELATIVE_MOUSE_MOTION_ACCEPT, 2.0, -1.0);

    hth_relative_mouse_filter_reset(&filter); /* Source 7 -> source 8. */
    expect_motion(&filter, true, true, 8U, 8U, -4.0, 2.0,
                  HTH_RELATIVE_MOUSE_MOTION_ACCEPT, -4.0, 2.0);
}

static void test_generic_and_inactive_paths_are_one_to_one(void)
{
    HTHRelativeMouseFilter filter = {0};

    filter.compensation_pending = true;
    expect_motion(&filter, true, false, 0U, 3U, 8.0, -6.0,
                  HTH_RELATIVE_MOUSE_MOTION_ACCEPT, 8.0, -6.0);
    assert(!filter.compensation_pending);
    filter.compensation_pending = true;
    expect_motion(&filter, false, true, 7U, 6U, -2.0, 5.0,
                  HTH_RELATIVE_MOUSE_MOTION_ACCEPT, -2.0, 5.0);
    assert(!filter.compensation_pending);
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
    test_real_reentry_sequence_is_one_to_one();
    test_small_movements_do_not_accumulate();
    test_alternating_movements_are_one_to_one();
    test_axis_independence();
    test_foreign_payload_is_never_emitted();
    test_repeated_reentry_cycles();
    test_capture_focus_and_source_transitions();
    test_generic_and_inactive_paths_are_one_to_one();
    return 0;
}
