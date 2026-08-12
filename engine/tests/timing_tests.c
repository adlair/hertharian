#include "hth_timing.h"
#include "timing_internal.h"

#include <assert.h>
#include <stddef.h>

static void test_timing_progress(void)
{
    HTHTiming *timing = hth_timing_create(60, 1000, 1000);

    assert(timing != NULL);
    hth_timing_begin_frame(timing, 1000);
    assert(hth_timing_delta_seconds(timing) >= 0.0);
    hth_timing_measure_work(timing, 1004);
    assert(hth_timing_remaining_ns(timing) > 12000000);
    assert(hth_timing_remaining_ns(timing) < 13000000);
    assert(hth_timing_frame_number(timing) == 0);
    assert(hth_timing_frame_work_seconds(timing) > 0.003);
    assert(hth_timing_frame_work_seconds(timing) < 0.005);
    hth_timing_finish_frame(timing, 1017);
    assert(hth_timing_frame_number(timing) == 1);

    hth_timing_begin_frame(timing, 1017);
    assert(hth_timing_delta_seconds(timing) > 0.016);
    assert(hth_timing_delta_seconds(timing) < 0.018);
    assert(hth_timing_elapsed_seconds(timing) > 0.016);
    assert(hth_timing_elapsed_seconds(timing) < 0.018);
    hth_timing_measure_work(timing, 1020);
    assert(hth_timing_frame_number(timing) == 1);
    hth_timing_finish_frame(timing, 1034);
    assert(hth_timing_frame_number(timing) == 2);
    hth_timing_destroy(timing);
}

static void test_uncapped_and_invalid(void)
{
    HTHTiming *timing = hth_timing_create(0, 100, 1000);

    assert(timing != NULL);
    assert(hth_timing_target_fps(timing) == 0);
    hth_timing_begin_frame(timing, 100);
    hth_timing_measure_work(timing, 101);
    assert(hth_timing_remaining_ns(timing) == 0);
    hth_timing_destroy(timing);
    assert(hth_timing_create(60, 0, 0) == NULL);
}

int main(void)
{
    test_timing_progress();
    test_uncapped_and_invalid();
    return 0;
}
