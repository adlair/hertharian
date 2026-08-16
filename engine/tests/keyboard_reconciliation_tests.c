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
    assert(!hth_keyboard_reconciliation_next_release(
        &state, physical_down, &released));
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
    assert(!hth_keyboard_reconciliation_next_release(
        &state, physical_down, &key));
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
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
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

static void test_repeat_down_reopens_after_normalized_release(void)
{
    HTHKeyboardReconciliation state = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    assert(hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    assert(released == HTH_KEY_W);

    /* Platform reports repeat and non-repeat KEY_DOWN through this same path. */
    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    observed_down[HTH_KEY_W] = true;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    hth_keyboard_reconciliation_report_up(&state, HTH_KEY_W);
    observed_down[HTH_KEY_W] = false;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
}

static void test_five_held_interruption_recovery_cycles(void)
{
    HTHKeyboardReconciliation state = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;
    unsigned int cycle;

    for (cycle = 0U; cycle < 5U; ++cycle) {
        hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
        assert(!hth_keyboard_reconciliation_next_release(
            &state, observed_down, &released));
        assert(hth_keyboard_reconciliation_next_release(
            &state, observed_down, &released));
        assert(released == HTH_KEY_W);

        /* A repeat KEY_DOWN is current held-state evidence. */
        hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
        observed_down[HTH_KEY_W] = true;
        assert(!hth_keyboard_reconciliation_next_release(
            &state, observed_down, &released));
        hth_keyboard_reconciliation_report_up(&state, HTH_KEY_W);
        observed_down[HTH_KEY_W] = false;
    }
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
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
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &key));
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

static void assert_fresh_press_survives_stale_observation(HTHKey key)
{
    HTHKeyboardReconciliation state = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    hth_keyboard_reconciliation_report_down(&state, key);
    hth_keyboard_reconciliation_reset(&state); /* Focus interruption. */
    hth_keyboard_reconciliation_report_down(&state, key);

    /* The first post-interruption snapshot can lag the real KeyDown. */
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    observed_down[key] = true;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));

    observed_down[key] = false;
    assert(hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    assert(released == key);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
}

static void test_first_press_after_interruption_is_not_reconciled_away(void)
{
    static const HTHKey movement_keys[] = {
        HTH_KEY_W, HTH_KEY_A, HTH_KEY_S, HTH_KEY_D
    };
    size_t index;

    for (index = 0U; index < sizeof(movement_keys) /
                               sizeof(movement_keys[0]); ++index) {
        assert_fresh_press_survives_stale_observation(
            movement_keys[index]);
    }
}

static void test_repeated_interruptions_and_key_switch(void)
{
    HTHKeyboardReconciliation state = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    hth_keyboard_reconciliation_reset(&state);
    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    observed_down[HTH_KEY_W] = true;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    hth_keyboard_reconciliation_report_up(&state, HTH_KEY_W);

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    hth_keyboard_reconciliation_reset(&state);
    observed_down[HTH_KEY_W] = false;
    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_A);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    observed_down[HTH_KEY_A] = true;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
}

static void test_multi_key_recovery_arms_each_key_independently(void)
{
    HTHKeyboardReconciliation state = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_D);
    hth_keyboard_reconciliation_reset(&state);
    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_W);
    hth_keyboard_reconciliation_report_down(&state, HTH_KEY_D);
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
    observed_down[HTH_KEY_W] = true;
    observed_down[HTH_KEY_D] = true;
    assert(!hth_keyboard_reconciliation_next_release(
        &state, observed_down, &released));
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
    test_repeat_down_reopens_after_normalized_release();
    test_five_held_interruption_recovery_cycles();
    test_movement_keys_and_combinations_are_generic();
    test_first_press_after_interruption_is_not_reconciled_away();
    test_repeated_interruptions_and_key_switch();
    test_multi_key_recovery_arms_each_key_independently();
    return 0;
}
