#ifndef HTH_INPUT_INTERNAL_H
#define HTH_INPUT_INTERNAL_H

#include "event.h"
#include "hth_input.h"

HTHInput *hth_input_create(void);
void hth_input_destroy(HTHInput *input);
void hth_input_begin_frame(HTHInput *input);
void hth_input_handle_event(HTHInput *input, const HTHPlatformEvent *event);

#endif
