#ifndef HTH_TIMING_INTERNAL_H
#define HTH_TIMING_INTERNAL_H

#include "hth_timing.h"

#include <stdbool.h>

HTHTiming *hth_timing_create(uint32_t target_fps, uint64_t counter,
                             uint64_t frequency);
void hth_timing_destroy(HTHTiming *timing);
void hth_timing_begin_frame(HTHTiming *timing, uint64_t counter);
void hth_timing_measure_work(HTHTiming *timing, uint64_t counter);
uint64_t hth_timing_remaining_ns(const HTHTiming *timing);
void hth_timing_finish_frame(HTHTiming *timing, uint64_t counter);

#endif
