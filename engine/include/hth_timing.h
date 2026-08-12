#ifndef HTH_TIMING_H
#define HTH_TIMING_H

#include <stdint.h>

typedef struct HTHTiming HTHTiming;

uint64_t hth_timing_frame_number(const HTHTiming *timing);
double hth_timing_delta_seconds(const HTHTiming *timing);
double hth_timing_elapsed_seconds(const HTHTiming *timing);
double hth_timing_frame_work_seconds(const HTHTiming *timing);
uint32_t hth_timing_target_fps(const HTHTiming *timing);

#endif
