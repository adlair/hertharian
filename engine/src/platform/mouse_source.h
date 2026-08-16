#ifndef HTH_MOUSE_SOURCE_H
#define HTH_MOUSE_SOURCE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum HTHRelativeMouseMotionDecision {
    HTH_RELATIVE_MOUSE_MOTION_ACCEPT = 0,
    HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE,
    HTH_RELATIVE_MOUSE_MOTION_DISCARD_REENTRY_COMPENSATION
} HTHRelativeMouseMotionDecision;

typedef struct HTHRelativeMouseFilter {
    double reconstructed_delta_x;
    double reconstructed_delta_y;
    bool compensation_pending;
} HTHRelativeMouseFilter;

bool hth_mouse_name_is_explicit_relative_source(const char *name);
void hth_relative_mouse_filter_reset(HTHRelativeMouseFilter *filter);
void hth_relative_mouse_filter_cancel_transition(
    HTHRelativeMouseFilter *filter);
HTHRelativeMouseMotionDecision hth_relative_mouse_filter_motion(
    HTHRelativeMouseFilter *filter,
    bool relative_mode,
    bool relative_source_known,
    uint32_t relative_source_id,
    uint32_t event_source_id,
    double delivered_delta_x,
    double delivered_delta_y,
    double *corrected_delta_x,
    double *corrected_delta_y);

#endif
