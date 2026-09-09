#include "player_revive_config.h"

#include <float.h>
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

static bool config_equals(HTHPlayerReviveConfig left,
                          HTHPlayerReviveConfig right)
{
    return left.revive_window_duration_seconds ==
               right.revive_window_duration_seconds &&
           left.revive_range == right.revive_range &&
           left.hold_duration_seconds == right.hold_duration_seconds &&
           left.revive_health == right.revive_health;
}

static bool test_null_zero_and_defaults(void)
{
    const HTHPlayerReviveConfig zero = {0};
    const HTHPlayerReviveConfig config = hth_player_revive_config_default();

    CHECK(!hth_player_revive_config_is_valid(NULL));
    CHECK(!hth_player_revive_config_is_valid(&zero));
    CHECK(config.revive_window_duration_seconds == 10.0);
    CHECK(config.revive_range == 2.0F);
    CHECK(config.hold_duration_seconds == 2.0);
    CHECK(config.revive_health == 25.0F);
    CHECK(hth_player_revive_config_is_valid(&config));
    return true;
}

static bool test_float_field_validation(void)
{
    const float invalid_values[] = {
        0.0F, -1.0F, NAN, INFINITY, -INFINITY
    };
    const float valid_extremes[] = {FLT_MIN, FLT_MAX};
    HTHPlayerReviveConfig config;
    size_t index;

    for (index = 0U;
         index < sizeof(invalid_values) / sizeof(invalid_values[0]);
         ++index) {
        config = hth_player_revive_config_default();
        config.revive_range = invalid_values[index];
        CHECK(!hth_player_revive_config_is_valid(&config));
        config = hth_player_revive_config_default();
        config.revive_health = invalid_values[index];
        CHECK(!hth_player_revive_config_is_valid(&config));
    }
    for (index = 0U;
         index < sizeof(valid_extremes) / sizeof(valid_extremes[0]);
         ++index) {
        config = hth_player_revive_config_default();
        config.revive_range = valid_extremes[index];
        CHECK(hth_player_revive_config_is_valid(&config));
        config = hth_player_revive_config_default();
        config.revive_health = valid_extremes[index];
        CHECK(hth_player_revive_config_is_valid(&config));
    }
    return true;
}

static bool test_double_field_validation(void)
{
    const double invalid_values[] = {
        0.0, -1.0, NAN, INFINITY, -INFINITY
    };
    const double valid_extremes[] = {DBL_MIN, DBL_MAX};
    HTHPlayerReviveConfig config;
    size_t index;

    for (index = 0U;
         index < sizeof(invalid_values) / sizeof(invalid_values[0]);
         ++index) {
        config = hth_player_revive_config_default();
        config.revive_window_duration_seconds = invalid_values[index];
        CHECK(!hth_player_revive_config_is_valid(&config));
        config = hth_player_revive_config_default();
        config.hold_duration_seconds = invalid_values[index];
        CHECK(!hth_player_revive_config_is_valid(&config));
    }
    for (index = 0U;
         index < sizeof(valid_extremes) / sizeof(valid_extremes[0]);
         ++index) {
        config = hth_player_revive_config_default();
        config.revive_window_duration_seconds = valid_extremes[index];
        CHECK(hth_player_revive_config_is_valid(&config));
        config = hth_player_revive_config_default();
        config.hold_duration_seconds = valid_extremes[index];
        CHECK(hth_player_revive_config_is_valid(&config));
    }
    return true;
}

static bool test_determinism_no_mutation_and_copy(void)
{
    const HTHPlayerReviveConfig original =
        hth_player_revive_config_default();
    HTHPlayerReviveConfig copy = original;
    size_t index;

    for (index = 0U; index < 128U; ++index) {
        CHECK(hth_player_revive_config_is_valid(&original));
        CHECK(config_equals(original, hth_player_revive_config_default()));
    }
    copy.revive_health = 1.0F;
    CHECK(copy.revive_health != original.revive_health);
    CHECK(original.revive_window_duration_seconds == 10.0);
    CHECK(original.revive_range == 2.0F);
    CHECK(original.hold_duration_seconds == 2.0);
    CHECK(original.revive_health == 25.0F);
    return true;
}

int main(void)
{
    if (!test_null_zero_and_defaults() ||
        !test_float_field_validation() ||
        !test_double_field_validation() ||
        !test_determinism_no_mutation_and_copy()) {
        return EXIT_FAILURE;
    }
    puts("player revive config tests passed");
    return EXIT_SUCCESS;
}
