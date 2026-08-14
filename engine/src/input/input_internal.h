#ifndef HTH_INPUT_INTERNAL_H
#define HTH_INPUT_INTERNAL_H

#include "event.h"
#include "hth_input.h"

HTHInput *hth_input_create(void);
void hth_input_destroy(HTHInput *input);
void hth_input_set_debug_fps_input(HTHInput *input, bool enabled);
void hth_input_begin_frame(HTHInput *input);
void hth_input_end_frame(HTHInput *input);
void hth_input_handle_event(HTHInput *input, const HTHPlatformEvent *event);
void hth_input_clear_mouse_delta(HTHInput *input);
void hth_input_begin_capture_transition_discard(HTHInput *input);
bool hth_input_mouse_motion_discard_active(const HTHInput *input);

#endif
