#ifndef HTH_KEYBOARD_RECONCILIATION_H
#define HTH_KEYBOARD_RECONCILIATION_H

#include "hth_input.h"

#include <stdbool.h>

typedef struct {
    bool reported_down[HTH_KEY_COUNT];
} HTHKeyboardReconciliation;

void hth_keyboard_reconciliation_report_down(
    HTHKeyboardReconciliation *state, HTHKey key);
void hth_keyboard_reconciliation_report_up(
    HTHKeyboardReconciliation *state, HTHKey key);
void hth_keyboard_reconciliation_reset(HTHKeyboardReconciliation *state);
bool hth_keyboard_reconciliation_next_release(
    HTHKeyboardReconciliation *state,
    const bool physical_down[HTH_KEY_COUNT], HTHKey *out_key);

#endif
