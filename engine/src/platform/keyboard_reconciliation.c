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
        state->release_armed[key] = false;
    }
}

void hth_keyboard_reconciliation_report_up(
    HTHKeyboardReconciliation *state, HTHKey key)
{
    if (state != NULL && valid_key(key)) {
        state->reported_down[key] = false;
        state->release_armed[key] = false;
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
        if (!state->reported_down[index]) {
            continue;
        }
        if (physical_down[index]) {
            state->release_armed[index] = true;
            continue;
        }
        if (!state->release_armed[index]) {
            /*
             * A native down event and its backend snapshot can briefly
             * disagree. Require one completed observation before a later
             * physical-up discrepancy may normalize the release.
             */
            state->release_armed[index] = true;
            continue;
        }
        state->reported_down[index] = false;
        state->release_armed[index] = false;
        *out_key = (HTHKey)index;
        return true;
    }
    return false;
}
