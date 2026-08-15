#include "hth_resource_config.h"
#include "aabb.h"
#include "collision_trace.h"
#include "collision_world.h"
#include "level.h"
#include "level_selection.h"
#include "player_body.h"
#include "resource.h"
#include "runtime_options.h"
#include "world.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n", __FILE__,       \
                    __LINE__, #condition);                                   \
            return false;                                                    \
        }                                                                    \
    } while (0)

static bool float_equal(float left, float right)
{
    return fabsf(left - right) <= 0.00001F;
}

static bool test_level_id_grammar(void)
{
    static const char *const valid[] = {
        "bootstrap", "selection_test", "crypt-01", "forest_intro",
        "level2", "a", "0", "a_b-c9"
    };
    static const char *const invalid[] = {
        "", "Bootstrap", "BOOTSTRAP", "bootstrap.hthlevel",
        "levels/bootstrap", "../bootstrap", "./bootstrap", "foo/bar",
        "foo\\bar", "foo bar", "foo.bar", "line\nbreak", "tab\tvalue"
    };
    static const char non_ascii[] = {'a', (char)0x80, '\0'};
    size_t index;

    CHECK(!hth_level_id_is_valid(NULL));
    for (index = 0U; index < sizeof(valid) / sizeof(valid[0]); ++index) {
        CHECK(hth_level_id_is_valid(valid[index]));
    }
    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        CHECK(!hth_level_id_is_valid(invalid[index]));
    }
    CHECK(!hth_level_id_is_valid(non_ascii));
    return true;
}

static bool test_mapping_and_ownership(void)
{
    char mutable_id[] = "bootstrap";
    HTHLevelSelection first = {0};
    HTHLevelSelection second = {0};
    HTHLevelSelection third = {0};

    CHECK(hth_level_selection_init(&first, mutable_id));
    CHECK(strcmp(first.level_id, "bootstrap") == 0);
    CHECK(strcmp(first.resource_id, "levels/bootstrap.hthlevel") == 0);
    CHECK(hth_resource_id_is_valid(first.resource_id));
    mutable_id[0] = 'x';
    CHECK(strcmp(first.level_id, "bootstrap") == 0);
    CHECK(strcmp(first.resource_id, "levels/bootstrap.hthlevel") == 0);

    CHECK(hth_level_selection_init(&second, "selection_test"));
    CHECK(strcmp(second.resource_id,
                 "levels/selection_test.hthlevel") == 0);
    CHECK(hth_level_selection_init(&third, "crypt-01"));
    CHECK(strcmp(third.resource_id, "levels/crypt-01.hthlevel") == 0);
    hth_level_selection_destroy(&second);
    CHECK(strcmp(first.level_id, "bootstrap") == 0);
    CHECK(strcmp(third.level_id, "crypt-01") == 0);

    hth_level_selection_destroy(&first);
    hth_level_selection_destroy(&first);
    hth_level_selection_destroy(&second);
    hth_level_selection_destroy(&third);
    return true;
}

static bool test_long_id_and_invalid_arguments(void)
{
    const size_t length = 65536U;
    char *long_id = malloc(length + 1U);
    HTHLevelSelection selection = {0};
    HTHLevelSelection occupied = {0};
    bool long_id_checks;

    if (long_id == NULL) {
        return false;
    }
    memset(long_id, 'a', length);
    long_id[length] = '\0';
    long_id_checks = hth_level_selection_init(&selection, long_id) &&
        strlen(selection.level_id) == length &&
        strlen(selection.resource_id) ==
            strlen("levels/") + length + strlen(".hthlevel") &&
        hth_resource_id_is_valid(selection.resource_id);
    hth_level_selection_destroy(&selection);
    free(long_id);
    CHECK(long_id_checks);

    CHECK(!hth_level_selection_init(NULL, "bootstrap"));
    CHECK(!hth_level_selection_init(&selection, NULL));
    CHECK(!hth_level_selection_init(&selection, "../bootstrap"));
    occupied.level_id = (char *)"occupied";
    CHECK(!hth_level_selection_init(&occupied, "bootstrap"));
    occupied.level_id = NULL;
    hth_level_selection_destroy(&selection);
    return true;
}

static bool parse_options(int argc, char *argv[], HTHRuntimeOptions *options,
                          HTHRuntimeOptionsErrorCode expected_error)
{
    HTHRuntimeOptionsError error;
    bool success = hth_runtime_options_parse(argc, argv, options, &error);

    return success == (expected_error == HTH_RUNTIME_OPTIONS_ERROR_NONE) &&
           error.code == expected_error;
}

static bool test_runtime_options(void)
{
    char *default_argv[] = {"engine"};
    char *explicit_argv[] = {"engine", "--level", "bootstrap"};
    char *alternate_a_argv[] = {
        "engine", "--headless", "--frames", "3", "--level",
        "selection_test"
    };
    char *alternate_b_argv[] = {
        "engine", "--level", "selection_test", "--headless", "--frames",
        "3"
    };
    char *duplicate_argv[] = {
        "engine", "--level", "bootstrap", "--level", "selection_test"
    };
    char *missing_argv[] = {"engine", "--level"};
    char *invalid_argv[] = {"engine", "--level", "Bootstrap"};
    char *empty_argv[] = {"engine", "--level", ""};
    char *pathlike_argv[] = {"engine", "--level", "../bootstrap"};
    char *unknown_argv[] = {"engine", "--unknown"};
    HTHRuntimeOptions default_options;
    HTHRuntimeOptions explicit_options;
    HTHRuntimeOptions alternate_a;
    HTHRuntimeOptions alternate_b;
    HTHRuntimeOptions unused;
    HTHLevelSelection default_selection = {0};
    HTHLevelSelection explicit_selection = {0};
    bool default_equivalence;

    CHECK(strcmp(hth_runtime_options_default_level_id(), "bootstrap") == 0);
    CHECK(parse_options(1, default_argv, &default_options,
                        HTH_RUNTIME_OPTIONS_ERROR_NONE));
    CHECK(strcmp(default_options.level_id, "bootstrap") == 0);
    CHECK(parse_options(3, explicit_argv, &explicit_options,
                        HTH_RUNTIME_OPTIONS_ERROR_NONE));
    CHECK(strcmp(default_options.level_id, explicit_options.level_id) == 0);
    default_equivalence = hth_level_selection_init(
        &default_selection, default_options.level_id) &&
        hth_level_selection_init(&explicit_selection,
                                 explicit_options.level_id) &&
        strcmp(default_selection.resource_id,
               explicit_selection.resource_id) == 0;
    hth_level_selection_destroy(&explicit_selection);
    hth_level_selection_destroy(&default_selection);
    CHECK(default_equivalence);
    CHECK(parse_options(6, alternate_a_argv, &alternate_a,
                        HTH_RUNTIME_OPTIONS_ERROR_NONE));
    CHECK(parse_options(6, alternate_b_argv, &alternate_b,
                        HTH_RUNTIME_OPTIONS_ERROR_NONE));
    CHECK(strcmp(alternate_a.level_id, alternate_b.level_id) == 0);
    CHECK(alternate_a.engine.headless && alternate_b.engine.headless);
    CHECK(alternate_a.engine.frame_limit == 3U &&
          alternate_b.engine.frame_limit == 3U);
    CHECK(parse_options(5, duplicate_argv, &unused,
                        HTH_RUNTIME_OPTIONS_ERROR_DUPLICATE_LEVEL));
    CHECK(parse_options(2, missing_argv, &unused,
                        HTH_RUNTIME_OPTIONS_ERROR_MISSING_LEVEL_VALUE));
    CHECK(parse_options(3, invalid_argv, &unused,
                        HTH_RUNTIME_OPTIONS_ERROR_INVALID_LEVEL_ID));
    CHECK(parse_options(3, empty_argv, &unused,
                        HTH_RUNTIME_OPTIONS_ERROR_INVALID_LEVEL_ID));
    CHECK(parse_options(3, pathlike_argv, &unused,
                        HTH_RUNTIME_OPTIONS_ERROR_INVALID_LEVEL_ID));
    CHECK(parse_options(2, unknown_argv, &unused,
                        HTH_RUNTIME_OPTIONS_ERROR_INVALID_ARGUMENTS));
    return true;
}

static bool load_selected_world(const char *level_id, HTHWorld *out_world)
{
    HTHResourceConfig config = {HTH_DEVELOPMENT_RESOURCE_ROOT};
    HTHResourceSystem *resources = NULL;
    HTHLevelSelection selection = {0};
    HTHResourceData data = {0};
    HTHLevelDescription description = {0};
    HTHLevelError error = {0};
    bool success = false;

    if (!hth_level_selection_init(&selection, level_id)) {
        return false;
    }
    resources = hth_resource_system_create(&config);
    if (resources != NULL &&
        hth_resource_load(resources, selection.resource_id, &data) &&
        hth_level_parse(data.data, data.size, &description, &error) &&
        hth_level_build_world(&description, out_world, &error)) {
        success = true;
    }
    hth_level_description_destroy(&description);
    hth_resource_data_release(&data);
    hth_resource_system_destroy(resources);
    hth_level_selection_destroy(&selection);
    return success;
}

static bool spawn_is_clear(const HTHWorld *world, HTHWorldSpawn spawn)
{
    HTHCollisionWorld collision_world;
    HTHPlayerBody body;
    HTHTrace trace;
    HTHVec3 mins;
    HTHVec3 maxs;

    CHECK(hth_player_body_init(&body, spawn.position));
    CHECK(hth_collision_world_build_from_world(&collision_world, world));
    mins = hth_vec3(-body.half_width, 0.0F, -body.half_width);
    maxs = hth_vec3(body.half_width, body.height, body.half_width);
    CHECK(hth_collision_world_trace_aabb(
        &collision_world, body.position, body.position, mins, maxs, &trace));
    return !trace.start_solid;
}

static bool test_real_level_resources(void)
{
    HTHWorld bootstrap = {0};
    HTHWorld alternate = {0};
    HTHWorldSpawn bootstrap_spawn;
    HTHWorldSpawn alternate_spawn;
    HTHAABB bootstrap_bounds;
    HTHAABB alternate_bounds;

    CHECK(load_selected_world("bootstrap", &bootstrap));
    CHECK(load_selected_world("selection_test", &alternate));
    CHECK(hth_world_static_object_count(&bootstrap) == 11U);
    CHECK(hth_world_static_object_count(&alternate) == 6U);
    CHECK(hth_world_default_spawn(&bootstrap, &bootstrap_spawn));
    CHECK(hth_world_default_spawn(&alternate, &alternate_spawn));
    CHECK(float_equal(bootstrap_spawn.position.x, 0.0F));
    CHECK(float_equal(bootstrap_spawn.position.y, 0.05F));
    CHECK(float_equal(bootstrap_spawn.position.z, 3.0F));
    CHECK(float_equal(alternate_spawn.position.x, 0.0F));
    CHECK(float_equal(alternate_spawn.position.y, 0.05F));
    CHECK(float_equal(alternate_spawn.position.z, 5.0F));
    CHECK(float_equal(alternate_spawn.yaw_radians, 0.0F));
    CHECK(hth_world_bounds(&bootstrap, &bootstrap_bounds));
    CHECK(hth_world_bounds(&alternate, &alternate_bounds));
    CHECK(float_equal(bootstrap_bounds.min.x, -20.0F));
    CHECK(float_equal(bootstrap_bounds.max.x, 20.0F));
    CHECK(float_equal(alternate_bounds.min.x, -8.0F));
    CHECK(float_equal(alternate_bounds.min.y, -1.0F));
    CHECK(float_equal(alternate_bounds.min.z, -8.0F));
    CHECK(float_equal(alternate_bounds.max.x, 8.0F));
    CHECK(float_equal(alternate_bounds.max.y, 3.0F));
    CHECK(float_equal(alternate_bounds.max.z, 8.0F));
    CHECK(spawn_is_clear(&alternate, alternate_spawn));
    hth_world_shutdown(&alternate);
    hth_world_shutdown(&bootstrap);
    return true;
}

static bool test_valid_missing_resource(void)
{
    HTHResourceConfig config = {HTH_DEVELOPMENT_RESOURCE_ROOT};
    HTHResourceSystem *resources = hth_resource_system_create(&config);
    HTHLevelSelection selection = {0};
    HTHResourceData data = {0};

    CHECK(resources != NULL);
    CHECK(hth_level_selection_init(&selection, "does_not_exist"));
    CHECK(strcmp(selection.resource_id,
                 "levels/does_not_exist.hthlevel") == 0);
    CHECK(!hth_resource_load(resources, selection.resource_id, &data));
    hth_level_selection_destroy(&selection);
    hth_resource_system_destroy(resources);
    return true;
}

int main(void)
{
    if (!test_level_id_grammar() || !test_mapping_and_ownership() ||
        !test_long_id_and_invalid_arguments() || !test_runtime_options() ||
        !test_real_level_resources() || !test_valid_missing_resource()) {
        return EXIT_FAILURE;
    }
    puts("level selection tests passed");
    return EXIT_SUCCESS;
}
