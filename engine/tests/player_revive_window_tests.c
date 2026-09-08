#include "player_revive_window.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,      \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

static bool state_equals(const HTHPlayerReviveWindow *window,
                         bool active, double remaining_seconds)
{
    bool observed_active = !active;
    double observed_remaining = -1.0;

    return hth_player_revive_window_query(
               window, &observed_active, &observed_remaining) &&
           observed_active == active &&
           observed_remaining == remaining_seconds;
}

static bool raw_state_equals(const HTHPlayerReviveWindow *left,
                             const HTHPlayerReviveWindow *right)
{
    bool remaining_equal = left->remaining_seconds == right->remaining_seconds;

    if (isnan(left->remaining_seconds) && isnan(right->remaining_seconds)) {
        remaining_equal = true;
    }
    return remaining_equal && left->active == right->active;
}

static bool test_zero_reset_and_query(void)
{
    HTHPlayerReviveWindow window = {0};
    bool active = true;
    double remaining = 123.0;
    size_t index;

    CHECK(state_equals(&window, false, 0.0));
    for (index = 0U; index < 128U; ++index) {
        CHECK(state_equals(&window, false, 0.0));
    }
    hth_player_revive_window_reset(NULL);
    hth_player_revive_window_reset(&window);
    CHECK(state_equals(&window, false, 0.0));

    CHECK(!hth_player_revive_window_query(NULL, &active, &remaining));
    CHECK(!active && remaining == 0.0);
    remaining = 123.0;
    CHECK(!hth_player_revive_window_query(&window, NULL, &remaining));
    CHECK(remaining == 0.0);
    active = true;
    CHECK(!hth_player_revive_window_query(&window, &active, NULL));
    CHECK(!active);
    return true;
}

static bool test_begin_validation(void)
{
    HTHPlayerReviveWindow window = {0};
    HTHPlayerReviveWindow snapshot;
    const double invalid_durations[] = {
        0.0, -1.0, NAN, INFINITY, -INFINITY
    };
    size_t index;

    CHECK(!hth_player_revive_window_begin(NULL, 5.0));
    for (index = 0U;
         index < sizeof(invalid_durations) / sizeof(invalid_durations[0]);
         ++index) {
        snapshot = window;
        CHECK(!hth_player_revive_window_begin(
            &window, invalid_durations[index]));
        CHECK(raw_state_equals(&window, &snapshot));
    }
    CHECK(hth_player_revive_window_begin(&window, 5.0));
    CHECK(state_equals(&window, true, 5.0));
    snapshot = window;
    CHECK(!hth_player_revive_window_begin(&window, 10.0));
    CHECK(raw_state_equals(&window, &snapshot));
    return true;
}

static bool test_advance_validation_and_inactive(void)
{
    HTHPlayerReviveWindow inactive = {0};
    HTHPlayerReviveWindow window = {0};
    HTHPlayerReviveWindow snapshot;
    const double invalid_deltas[] = {-1.0, NAN, INFINITY, -INFINITY};
    bool expired = true;
    size_t index;

    CHECK(!hth_player_revive_window_advance(NULL, 1.0, &expired));
    CHECK(!expired);
    CHECK(hth_player_revive_window_advance(&inactive, 1.0, &expired));
    CHECK(!expired && state_equals(&inactive, false, 0.0));
    CHECK(hth_player_revive_window_advance(&inactive, 100.0, &expired));
    CHECK(!expired && state_equals(&inactive, false, 0.0));

    CHECK(hth_player_revive_window_begin(&window, 5.0));
    snapshot = window;
    CHECK(!hth_player_revive_window_advance(&window, 1.0, NULL));
    CHECK(raw_state_equals(&window, &snapshot));
    for (index = 0U;
         index < sizeof(invalid_deltas) / sizeof(invalid_deltas[0]);
         ++index) {
        expired = true;
        snapshot = window;
        CHECK(!hth_player_revive_window_advance(
            &window, invalid_deltas[index], &expired));
        CHECK(!expired && raw_state_equals(&window, &snapshot));
    }
    CHECK(hth_player_revive_window_advance(&window, 0.0, &expired));
    CHECK(!expired && state_equals(&window, true, 5.0));
    return true;
}

static bool test_partial_exact_overshoot_and_once(void)
{
    HTHPlayerReviveWindow window = {0};
    bool expired = true;
    size_t index;

    CHECK(hth_player_revive_window_begin(&window, 5.0));
    CHECK(hth_player_revive_window_advance(&window, 2.0, &expired));
    CHECK(!expired && state_equals(&window, true, 3.0));
    hth_player_revive_window_reset(&window);

    CHECK(hth_player_revive_window_begin(&window, 1.0));
    for (index = 0U; index < 3U; ++index) {
        CHECK(hth_player_revive_window_advance(&window, 0.25, &expired));
        CHECK(!expired);
    }
    CHECK(state_equals(&window, true, 0.25));
    CHECK(hth_player_revive_window_advance(&window, 0.25, &expired));
    CHECK(expired && state_equals(&window, false, 0.0));
    CHECK(hth_player_revive_window_advance(&window, 0.0, &expired));
    CHECK(!expired);
    CHECK(hth_player_revive_window_advance(&window, 1.0, &expired));
    CHECK(!expired);
    CHECK(hth_player_revive_window_advance(&window, 100.0, &expired));
    CHECK(!expired && state_equals(&window, false, 0.0));

    CHECK(hth_player_revive_window_begin(&window, 1.0));
    CHECK(hth_player_revive_window_advance(&window, 100.0, &expired));
    CHECK(expired && state_equals(&window, false, 0.0));
    return true;
}

static bool test_reset_reentry_and_copy(void)
{
    HTHPlayerReviveWindow original = {0};
    HTHPlayerReviveWindow copy;
    bool expired;

    CHECK(hth_player_revive_window_begin(&original, 10.0));
    CHECK(hth_player_revive_window_advance(&original, 3.0, &expired));
    CHECK(!expired && state_equals(&original, true, 7.0));
    copy = original;
    hth_player_revive_window_reset(&copy);
    CHECK(state_equals(&copy, false, 0.0));
    CHECK(state_equals(&original, true, 7.0));

    hth_player_revive_window_reset(&original);
    CHECK(hth_player_revive_window_begin(&original, 7.0));
    CHECK(state_equals(&original, true, 7.0));
    CHECK(hth_player_revive_window_advance(&original, 7.0, &expired));
    CHECK(expired && state_equals(&original, false, 0.0));
    CHECK(hth_player_revive_window_begin(&original, 2.0));
    CHECK(state_equals(&original, true, 2.0));
    return true;
}

static bool test_floating_point_residue(void)
{
    HTHPlayerReviveWindow window = {0};
    const double partition = 1.0 / 30.0;
    const double tiny = nextafter(0.0, 1.0);
    bool active;
    bool expired = false;
    double remaining;
    size_t index;

    CHECK(hth_player_revive_window_begin(&window, 1.0));
    for (index = 0U; index < 30U; ++index) {
        CHECK(hth_player_revive_window_advance(
            &window, partition, &expired));
    }
    CHECK(hth_player_revive_window_query(&window, &active, &remaining));
    printf("floating residue after 30 partitions: %.17g\n", remaining);
    if (active) {
        CHECK(remaining > 0.0 && !expired);
        CHECK(hth_player_revive_window_advance(
            &window, remaining, &expired));
        CHECK(expired && state_equals(&window, false, 0.0));
    } else {
        CHECK(expired && remaining == 0.0);
    }

    CHECK(hth_player_revive_window_begin(&window, tiny));
    CHECK(hth_player_revive_window_advance(&window, 0.0, &expired));
    CHECK(!expired && state_equals(&window, true, tiny));
    CHECK(hth_player_revive_window_advance(&window, tiny, &expired));
    CHECK(expired && state_equals(&window, false, 0.0));
    return true;
}

static bool malformed_case(HTHPlayerReviveWindow malformed)
{
    HTHPlayerReviveWindow window = malformed;
    HTHPlayerReviveWindow snapshot = malformed;
    bool active = true;
    bool expired = true;
    double remaining = 123.0;

    CHECK(!hth_player_revive_window_query(&window, &active, &remaining));
    CHECK(!active && remaining == 0.0);
    CHECK(raw_state_equals(&window, &snapshot));
    CHECK(!hth_player_revive_window_begin(&window, 1.0));
    CHECK(raw_state_equals(&window, &snapshot));
    CHECK(!hth_player_revive_window_advance(&window, 0.5, &expired));
    CHECK(!expired && raw_state_equals(&window, &snapshot));
    hth_player_revive_window_reset(&window);
    CHECK(state_equals(&window, false, 0.0));
    return true;
}

static bool test_malformed_state_recovery(void)
{
    CHECK(malformed_case((HTHPlayerReviveWindow){1.0, false}));
    CHECK(malformed_case((HTHPlayerReviveWindow){0.0, true}));
    CHECK(malformed_case((HTHPlayerReviveWindow){-1.0, true}));
    CHECK(malformed_case((HTHPlayerReviveWindow){NAN, true}));
    CHECK(malformed_case((HTHPlayerReviveWindow){INFINITY, true}));
    return true;
}

static bool test_multiple_instances(void)
{
    HTHPlayerReviveWindow windows[4] = {{0}};
    const double durations[4] = {1.0, 2.0, 3.0, 4.0};
    bool expired;
    size_t index;

    for (index = 0U; index < 4U; ++index) {
        CHECK(hth_player_revive_window_begin(&windows[index],
                                              durations[index]));
    }
    CHECK(hth_player_revive_window_advance(&windows[1], 0.5, &expired));
    CHECK(!expired && state_equals(&windows[1], true, 1.5));
    CHECK(hth_player_revive_window_advance(&windows[3], 4.0, &expired));
    CHECK(expired && state_equals(&windows[3], false, 0.0));
    CHECK(state_equals(&windows[0], true, 1.0));
    CHECK(state_equals(&windows[2], true, 3.0));
    return true;
}

int main(void)
{
    typedef bool (*TestFunction)(void);
    const TestFunction tests[] = {
        test_zero_reset_and_query,
        test_begin_validation,
        test_advance_validation_and_inactive,
        test_partial_exact_overshoot_and_once,
        test_reset_reentry_and_copy,
        test_floating_point_residue,
        test_malformed_state_recovery,
        test_multiple_instances
    };
    size_t index;

    for (index = 0U; index < sizeof(tests) / sizeof(tests[0]); ++index) {
        if (!tests[index]()) {
            return EXIT_FAILURE;
        }
    }
    puts("player revive window tests passed");
    return EXIT_SUCCESS;
}
