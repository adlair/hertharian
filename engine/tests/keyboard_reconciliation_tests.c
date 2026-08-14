#include "keyboard_reconciliation.h"

#include <assert.h>
#include <stddef.h>

static void test_reported_and_physical_down_produces_nothing(void)
{
    HTHKeyboardReconciliation state = {0};
    bool physical_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    physical_down[HTH_KEY_W] = true;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, physical_down, &released));
}

static void test_missing_release_is_emitted_once(void)
{
    HTHKeyboardReconciliation state = {0};
    bool physical_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    assert(hth_keyboard_reconciliation_next_release(
        &state, physical_down, &released));
    assert(released == HTH_KEY_W);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, physical_down, &released));
}

static void test_normal_release_prevents_reconciliation(void)
{
    HTHKeyboardReconciliation state = {0};
    bool physical_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_A);
    hth_keyboard_reconciliation_report_up(&state, HTH_KEY_A);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, physical_down, &released));
}

static void test_reported_up_observed_down_does_not_synthesize_press(void)
{
    HTHKeyboardReconciliation state = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    observed_down[HTH_KEY_D] = true;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
}

static void test_multiple_missing_releases(void)
{
    HTHKeyboardReconciliation state = {0};
    bool physical_down[HTH_KEY_COUNT] = {false};
    bool released[HTH_KEY_COUNT] = {false};
    HTHKey key;
    unsigned int release_count = 0U;

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_A);
    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_D);
    physical_down[HTH_KEY_D] = true;
    while (hth_keyboard_reconciliation_next_release(
               &state, physical_down, &key)) {
        released[key] = true;
        release_count++;
    }
    assert(release_count == 2U);
    assert(released[HTH_KEY_W]);
    assert(released[HTH_KEY_A]);
    assert(!released[HTH_KEY_D]);
}

static void test_focus_reset_and_gain_do_not_synthesize_down(void)
{
    HTHKeyboardReconciliation state = {0};
    bool physical_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_S);
    hth_keyboard_reconciliation_reset(&state);
    physical_down[HTH_KEY_S] = true;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, physical_down, &released));

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_S);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, physical_down, &released));
}

static void test_normalized_release_allows_new_press_and_late_release(void)
{
    HTHKeyboardReconciliation state = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    assert(hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    assert(released == HTH_KEY_W);

    /* A much later real key-up remains idempotent. */
    hth_keyboard_reconciliation_report_up(&state, HTH_KEY_W);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));

    /* A fresh real key-down is accepted immediately after recovery. */
    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    observed_down[HTH_KEY_W] = true;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    observed_down[HTH_KEY_W] = false;
    assert(hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    assert(released == HTH_KEY_W);
}

static void assert_each_key_releases_independently(const HTHKey *keys,
                                                   size_t key_count)
{
    HTHKeyboardReconciliation state = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    bool released[HTH_KEY_COUNT] = {false};
    HTHKey key;
    size_t index;
    size_t release_count = 0U;

    for (index = 0; index < key_count; ++index) {
        hth_keyboard_reconciliation_report_down(&state, keys[index]);
    }
    while (hth_keyboard_reconciliation_next_release(
               &state, observed_down, &key)) {
        released[key] = true;
        release_count++;
    }
    assert(release_count == key_count);
    for (index = 0; index < key_count; ++index) {
        assert(released[keys[index]]);
    }
}

static void test_movement_keys_and_combinations_are_generic(void)
{
    static const HTHKey movement_keys[] = {
        HTH_KEY_W, HTH_KEY_S, HTH_KEY_A, HTH_KEY_D
    };
    static const HTHKey forward_right[] = {HTH_KEY_W, HTH_KEY_D};
    static const HTHKey forward_left[] = {HTH_KEY_W, HTH_KEY_A};
    static const HTHKey forward_jump[] = {HTH_KEY_W, HTH_KEY_SPACE};

    assert_each_key_releases_independently(
        movement_keys, sizeof(movement_keys) / sizeof(movement_keys[0]));
    assert_each_key_releases_independently(
        forward_right, sizeof(forward_right) / sizeof(forward_right[0]));
    assert_each_key_releases_independently(
        forward_left, sizeof(forward_left) / sizeof(forward_left[0]));
    assert_each_key_releases_independently(
        forward_jump, sizeof(forward_jump) / sizeof(forward_jump[0]));
}

int main(void)
{
    test_reported_and_physical_down_produces_nothing();
    test_missing_release_is_emitted_once();
    test_normal_release_prevents_reconciliation();
    test_reported_up_observed_down_does_not_synthesize_press();
    test_multiple_missing_releases();
    test_focus_reset_and_gain_do_not_synthesize_down();
    test_normalized_release_allows_new_press_and_late_release();
    test_movement_keys_and_combinations_are_generic();
    return 0;
}
