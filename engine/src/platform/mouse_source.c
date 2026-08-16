#include "mouse_source.h"

#include <stddef.h>
#include <string.h>

bool hth_mouse_name_is_explicit_relative_source(const char *name)
{
    static const char prefix[] = "xwayland-relative-pointer";
    size_t prefix_length = sizeof(prefix) - 1;

    if (name == NULL || strncmp(name, prefix, prefix_length) != 0) {
        return false;
    }
    return name[prefix_length] == '\0' || name[prefix_length] == ':';
}

void hth_relative_mouse_filter_reset(HTHRelativeMouseFilter *filter)
{
    if (filter != NULL) {
        filter->reconstructed_delta_x = 0.0;
        filter->reconstructed_delta_y = 0.0;
        hth_relative_mouse_filter_cancel_transition(filter);
    }
}

void hth_relative_mouse_filter_cancel_transition(
    HTHRelativeMouseFilter *filter)
{
    if (filter != NULL) {
        filter->compensation_pending = false;
    }
}

HTHRelativeMouseMotionDecision hth_relative_mouse_filter_motion(
    HTHRelativeMouseFilter *filter,
    bool relative_mode,
    bool relative_source_known,
    uint32_t relative_source_id,
    uint32_t event_source_id,
    double delivered_delta_x,
    double delivered_delta_y,
    double *corrected_delta_x,
    double *corrected_delta_y)
{
    if (corrected_delta_x != NULL) {
        *corrected_delta_x = delivered_delta_x;
    }
    if (corrected_delta_y != NULL) {
        *corrected_delta_y = delivered_delta_y;
    }
    if (filter == NULL || !relative_mode || !relative_source_known) {
        hth_relative_mouse_filter_reset(filter);
        return HTH_RELATIVE_MOUSE_MOTION_ACCEPT;
    }

    /* SDL's X11 path differences the explicit XWayland relative-pointer
     * samples as if they were absolute master-device valuators. Integrating
     * the delivered differences restores each original relative sample. */
    filter->reconstructed_delta_x += delivered_delta_x;
    filter->reconstructed_delta_y += delivered_delta_y;
    if (corrected_delta_x != NULL) {
        *corrected_delta_x = filter->reconstructed_delta_x;
    }
    if (corrected_delta_y != NULL) {
        *corrected_delta_y = filter->reconstructed_delta_y;
    }

    if (event_source_id != relative_source_id) {
        filter->compensation_pending = true;
        return HTH_RELATIVE_MOUSE_MOTION_DISCARD_FOREIGN_SOURCE;
    }

    if (filter->compensation_pending) {
        filter->compensation_pending = false;
        return HTH_RELATIVE_MOUSE_MOTION_DISCARD_REENTRY_COMPENSATION;
    }

    return HTH_RELATIVE_MOUSE_MOTION_ACCEPT;
}
