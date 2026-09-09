#include "gameplay_interact_action.h"

#include "event.h"
#include "input_internal.h"
#include "keyboard_reconciliation.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,       \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

static bool state_is(HTHGameplayActionState state, bool down,
                     bool pressed, bool released)
{
    return state.down == down && state.pressed == pressed &&
           state.released == released;
}

static void send_key(HTHInput *input, HTHPlatformEventType type,
                     HTHKey key, bool repeat)
{
    HTHPlatformEvent event = {0};

    event.type = type;
    event.data.keyboard.key = key;
    event.data.keyboard.repeat = repeat;
    hth_input_handle_event(input, &event);
}

static void send_focus(HTHInput *input, HTHPlatformEventType type)
{
    HTHPlatformEvent event = {0};

    event.type = type;
    hth_input_handle_event(input, &event);
}

static bool query_state(const HTHInput *input, HTHKey binding,
                        bool down, bool pressed, bool released)
{
    HTHGameplayActionState state = {true, true, true};

    return hth_gameplay_interact_action_query(input, binding, &state) &&
           state_is(state, down, pressed, released);
}

static bool test_validation_and_canonical_output(void)
{
    HTHInput *input = hth_input_create();
    HTHGameplayActionState state = {true, true, true};

    CHECK(input != NULL);
    CHECK(!hth_gameplay_interact_action_query(
        NULL, HTH_KEY_A, &state));
    CHECK(state_is(state, false, false, false));

    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_A, false);
    CHECK(!hth_gameplay_interact_action_query(
        input, HTH_KEY_A, NULL));
    CHECK(hth_input_key_down(input, HTH_KEY_A));
    CHECK(hth_input_key_pressed(input, HTH_KEY_A));

    state = (HTHGameplayActionState){true, true, true};
    CHECK(!hth_gameplay_interact_action_query(
        input, HTH_KEY_UNKNOWN, &state));
    CHECK(state_is(state, false, false, false));
    state = (HTHGameplayActionState){true, true, true};
    CHECK(!hth_gameplay_interact_action_query(
        input, HTH_KEY_COUNT, &state));
    CHECK(state_is(state, false, false, false));
    state = (HTHGameplayActionState){true, true, true};
    CHECK(!hth_gameplay_interact_action_query(
        input, (HTHKey)(HTH_KEY_COUNT + 1), &state));
    CHECK(state_is(state, false, false, false));
    state = (HTHGameplayActionState){true, true, true};
    CHECK(!hth_gameplay_interact_action_query(
        input, (HTHKey)-1, &state));
    CHECK(state_is(state, false, false, false));

    hth_input_destroy(input);
    return true;
}

static bool test_frame_lifecycle(void)
{
    HTHInput *input = hth_input_create();

    CHECK(input != NULL);
    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_E, false);
    CHECK(query_state(input, HTH_KEY_E, true, true, false));

    hth_input_end_frame(input);
    CHECK(query_state(input, HTH_KEY_E, true, true, false));
    hth_input_begin_frame(input);
    CHECK(query_state(input, HTH_KEY_E, true, false, false));

    send_key(input, HTH_PLATFORM_EVENT_KEY_UP, HTH_KEY_E, false);
    CHECK(query_state(input, HTH_KEY_E, false, false, true));
    hth_input_end_frame(input);
    hth_input_begin_frame(input);
    CHECK(query_state(input, HTH_KEY_E, false, false, false));

    hth_input_destroy(input);
    return true;
}

static bool test_same_frame_edges_and_duplicates(void)
{
    HTHInput *input = hth_input_create();

    CHECK(input != NULL);
    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_A, false);
    send_key(input, HTH_PLATFORM_EVENT_KEY_UP, HTH_KEY_A, false);
    CHECK(query_state(input, HTH_KEY_A, false, true, true));

    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_A, false);
    send_key(input, HTH_PLATFORM_EVENT_KEY_UP, HTH_KEY_A, false);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_A, false);
    CHECK(query_state(input, HTH_KEY_A, true, true, true));

    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_UP, HTH_KEY_A, false);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_A, false);
    CHECK(query_state(input, HTH_KEY_A, true, true, true));

    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_A, false);
    CHECK(query_state(input, HTH_KEY_A, true, false, false));
    send_key(input, HTH_PLATFORM_EVENT_KEY_UP, HTH_KEY_A, false);
    send_key(input, HTH_PLATFORM_EVENT_KEY_UP, HTH_KEY_A, false);
    CHECK(query_state(input, HTH_KEY_A, false, false, true));
    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_UP, HTH_KEY_A, false);
    CHECK(query_state(input, HTH_KEY_A, false, false, false));

    hth_input_destroy(input);
    return true;
}

static bool test_repeat_and_reconciled_release(void)
{
    HTHInput *input = hth_input_create();
    HTHKeyboardReconciliation reconciliation = {0};
    bool observed_down[HTH_KEY_COUNT] = {false};
    HTHKey released = HTH_KEY_UNKNOWN;

    CHECK(input != NULL);
    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_R, false);
    hth_keyboard_reconciliation_report_down(
        &reconciliation, HTH_KEY_R);
    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_R, true);
    CHECK(query_state(input, HTH_KEY_R, true, false, false));

    CHECK(!hth_keyboard_reconciliation_next_release(
        &reconciliation, observed_down, &released));
    CHECK(hth_keyboard_reconciliation_next_release(
        &reconciliation, observed_down, &released));
    CHECK(released == HTH_KEY_R);
    send_key(input, HTH_PLATFORM_EVENT_KEY_UP, released, false);
    CHECK(query_state(input, HTH_KEY_R, false, false, true));
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_R, true);
    CHECK(query_state(input, HTH_KEY_R, true, false, true));

    hth_input_destroy(input);
    return true;
}

static bool test_focus_semantics(void)
{
    HTHInput *input = hth_input_create();

    CHECK(input != NULL);
    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_F, false);
    hth_input_begin_frame(input);
    send_focus(input, HTH_PLATFORM_EVENT_FOCUS_LOST);
    CHECK(query_state(input, HTH_KEY_F, false, false, true));

    hth_input_begin_frame(input);
    send_focus(input, HTH_PLATFORM_EVENT_FOCUS_GAINED);
    CHECK(query_state(input, HTH_KEY_F, false, false, false));

    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_F, false);
    send_focus(input, HTH_PLATFORM_EVENT_FOCUS_LOST);
    CHECK(query_state(input, HTH_KEY_F, false, true, true));

    hth_input_destroy(input);
    return true;
}

static bool test_capture_and_binding_ownership(void)
{
    HTHInput *input = hth_input_create();

    CHECK(input != NULL);
    hth_input_begin_frame(input);
    send_key(input, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_A, false);
    CHECK(query_state(input, HTH_KEY_A, true, true, false));
    CHECK(query_state(input, HTH_KEY_B, false, false, false));

    hth_input_begin_capture_transition_discard(input);
    CHECK(query_state(input, HTH_KEY_A, true, true, false));
    CHECK(query_state(input, HTH_KEY_B, false, false, false));
    hth_input_end_frame(input);
    hth_input_begin_frame(input);
    CHECK(query_state(input, HTH_KEY_A, true, false, false));
    hth_input_begin_capture_transition_discard(input);
    CHECK(query_state(input, HTH_KEY_A, true, false, false));

    hth_input_destroy(input);
    return true;
}

static bool test_independence_determinism_and_nonmutation(void)
{
    HTHInput *left = hth_input_create();
    HTHInput *right = hth_input_create();
    bool down_before[HTH_KEY_COUNT];
    bool pressed_before[HTH_KEY_COUNT];
    bool released_before[HTH_KEY_COUNT];
    HTHGameplayActionState expected;
    HTHGameplayActionState actual;
    int key;
    unsigned int iteration;

    CHECK(left != NULL && right != NULL);
    hth_input_begin_frame(left);
    hth_input_begin_frame(right);
    send_key(left, HTH_PLATFORM_EVENT_KEY_DOWN, HTH_KEY_I, false);
    CHECK(query_state(left, HTH_KEY_I, true, true, false));
    CHECK(query_state(right, HTH_KEY_I, false, false, false));
    CHECK(hth_gameplay_interact_action_query(
        left, HTH_KEY_I, &expected));

    for (key = (int)HTH_KEY_UNKNOWN; key < (int)HTH_KEY_COUNT; ++key) {
        HTHKey current = (HTHKey)key;

        down_before[key] = hth_input_key_down(left, current);
        pressed_before[key] = hth_input_key_pressed(left, current);
        released_before[key] = hth_input_key_released(left, current);
    }
    for (iteration = 0U; iteration < 128U; ++iteration) {
        CHECK(hth_gameplay_interact_action_query(
            left, HTH_KEY_I, &actual));
        CHECK(state_is(actual, expected.down, expected.pressed,
                       expected.released));
    }
    for (key = (int)HTH_KEY_UNKNOWN; key < (int)HTH_KEY_COUNT; ++key) {
        HTHKey current = (HTHKey)key;

        CHECK(hth_input_key_down(left, current) == down_before[key]);
        CHECK(hth_input_key_pressed(left, current) == pressed_before[key]);
        CHECK(hth_input_key_released(left, current) ==
              released_before[key]);
    }

    hth_input_destroy(right);
    hth_input_destroy(left);
    return true;
}

int main(void)
{
    static bool (*const tests[])(void) = {
        test_validation_and_canonical_output,
        test_frame_lifecycle,
        test_same_frame_edges_and_duplicates,
        test_repeat_and_reconciled_release,
        test_focus_semantics,
        test_capture_and_binding_ownership,
        test_independence_determinism_and_nonmutation
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("gameplay Interact action tests passed");
    return EXIT_SUCCESS;
}
