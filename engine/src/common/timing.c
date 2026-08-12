#include "timing_internal.h"

#include <stdlib.h>

struct HTHTiming {
    uint64_t frame_number;
    uint64_t start_counter;
    uint64_t previous_counter;
    uint64_t frame_start_counter;
    uint64_t frame_end_counter;
    uint64_t counter_frequency;
    double delta_seconds;
    double elapsed_seconds;
    double frame_work_seconds;
    uint32_t target_fps;
};

HTHTiming *hth_timing_create(uint32_t target_fps, uint64_t counter,
                             uint64_t frequency)
{
    HTHTiming *timing;

    if (frequency == 0) {
        return NULL;
    }

    timing = calloc(1, sizeof(*timing));
    if (timing == NULL) {
        return NULL;
    }

    timing->target_fps = target_fps;
    timing->counter_frequency = frequency;
    timing->start_counter = counter;
    timing->previous_counter = counter;
    timing->frame_start_counter = counter;
    return timing;
}

void hth_timing_destroy(HTHTiming *timing)
{
    free(timing);
}

void hth_timing_begin_frame(HTHTiming *timing, uint64_t counter)
{
    if (timing == NULL) {
        return;
    }

    timing->frame_start_counter = counter;
    if (timing->frame_number == 0 || counter < timing->previous_counter) {
        timing->delta_seconds = 0.0;
    } else {
        timing->delta_seconds =
            (double)(counter - timing->previous_counter) /
            (double)timing->counter_frequency;
    }

    timing->elapsed_seconds = counter >= timing->start_counter
        ? (double)(counter - timing->start_counter) /
          (double)timing->counter_frequency
        : 0.0;
    timing->previous_counter = counter;
}

void hth_timing_measure_work(HTHTiming *timing, uint64_t counter)
{
    if (timing == NULL) {
        return;
    }

    timing->frame_work_seconds = counter >= timing->frame_start_counter
        ? (double)(counter - timing->frame_start_counter) /
          (double)timing->counter_frequency
        : 0.0;
}

uint64_t hth_timing_remaining_ns(const HTHTiming *timing)
{
    long double remaining_ns;
    long double remaining_seconds;
    long double target_seconds;

    if (timing == NULL || timing->target_fps == 0) {
        return 0;
    }

    target_seconds = 1.0L / (long double)timing->target_fps;
    remaining_seconds = target_seconds -
                        (long double)timing->frame_work_seconds;
    if (remaining_seconds <= 0.0L) {
        return 0;
    }

    remaining_ns = remaining_seconds * 1000000000.0L;
    if (remaining_ns >= (long double)UINT64_MAX) {
        return UINT64_MAX;
    }
    return (uint64_t)remaining_ns;
}

void hth_timing_finish_frame(HTHTiming *timing, uint64_t counter)
{
    if (timing == NULL) {
        return;
    }

    timing->frame_end_counter = counter;
    timing->frame_number++;
}

uint64_t hth_timing_frame_number(const HTHTiming *timing)
{
    return timing != NULL ? timing->frame_number : 0;
}

double hth_timing_delta_seconds(const HTHTiming *timing)
{
    return timing != NULL ? timing->delta_seconds : 0.0;
}

double hth_timing_elapsed_seconds(const HTHTiming *timing)
{
    return timing != NULL ? timing->elapsed_seconds : 0.0;
}

double hth_timing_frame_work_seconds(const HTHTiming *timing)
{
    return timing != NULL ? timing->frame_work_seconds : 0.0;
}

uint32_t hth_timing_target_fps(const HTHTiming *timing)
{
    return timing != NULL ? timing->target_fps : 0;
}
