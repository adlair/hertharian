#include "keyboard_reconciliation.h"

#include <stddef.h>
#include <string.h>

static bool valid_key(HTHKey key)
{
    return key > HTH_KEY_UNKNOWN && key < HTH_KEY_COUNT;
}

void hth_keyboard_reconciliation_report_down(
    HTHKeyboardReconciliation *state, HTHKey key)
{
    if (state != NULL && valid_key(key)) {
        state->reported_down[key] = true;
    }
}

void hth_keyboard_reconciliation_report_up(
    HTHKeyboardReconciliation *state, HTHKey key)
{
    if (state != NULL && valid_key(key)) {
        state->reported_down[key] = false;
    }
}

void hth_keyboard_reconciliation_reset(HTHKeyboardReconciliation *state)
{
    if (state != NULL) {
        memset(state, 0, sizeof(*state));
    }
}

bool hth_keyboard_reconciliation_next_release(
    HTHKeyboardReconciliation *state,
    const bool physical_down[HTH_KEY_COUNT], HTHKey *out_key)
{
    size_t index;

    if (state == NULL || physical_down == NULL || out_key == NULL) {
        return false;
    }
    for (index = (size_t)HTH_KEY_UNKNOWN + 1U;
         index < (size_t)HTH_KEY_COUNT; ++index) {
        if (state->reported_down[index] && !physical_down[index]) {
            state->reported_down[index] = false;
            *out_key = (HTHKey)index;
            return true;
        }
    }
    return false;
}
