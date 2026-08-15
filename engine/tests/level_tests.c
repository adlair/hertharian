#include "hth_resource_config.h"
#include "collision_trace.h"
#include "collision_world.h"
#include "level.h"
#include "player_body.h"
#include "resource.h"
#include "world.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

static bool close_float(float left, float right)
{
    return fabsf(left - right) <= 1.0e-6F;
}

static void assert_vec3(HTHVec3 actual, HTHVec3 expected)
{
    assert(close_float(actual.x, expected.x));
    assert(close_float(actual.y, expected.y));
    assert(close_float(actual.z, expected.z));
}

static HTHLevelDescription parse_success(const unsigned char *data,
                                         size_t size)
{
    HTHLevelDescription description = {0};
    HTHLevelError error = {0};

    assert(hth_level_parse(data, size, &description, &error));
    assert(error.message[0] == '\0');
    return description;
}

static void assert_parse_failure(const unsigned char *data, size_t size)
{
    HTHLevelDescription description = {0};
    HTHLevelError error = {0};

    assert(!hth_level_parse(data, size, &description, &error));
    assert(description.objects == NULL);
    assert(description.object_count == 0U);
    assert(description.object_capacity == 0U);
    assert(error.line > 0U);
    assert(error.column > 0U);
    assert(error.message[0] != '\0');
    hth_level_description_destroy(&description);
}

static HTHWorld build_success(const HTHLevelDescription *description)
{
    HTHLevelError error = {0};
    HTHWorld world = {0};

    assert(hth_level_build_world(description, &world, &error));
    assert(error.message[0] == '\0');
    assert(hth_world_is_finalized(&world));
    return world;
}

static void test_minimal_level_and_float_syntax(void)
{
    static const unsigned char level[] =
        "hthlevel 1\n"
        "spawn 0 -0 0.5 -0.25\n"
        "static_object\n"
        "bounds -1e2 -1 -1 1.0e-2 1 1\n"
        "flags none\n"
        "visual none\n"
        "end";
    HTHLevelDescription description =
        parse_success(level, sizeof(level) - 1U);
    HTHWorld world = build_success(&description);
    HTHWorldSpawn spawn;
    const HTHWorldStaticObject *object;

    assert(description.format_version == HTH_LEVEL_FORMAT_VERSION);
    assert(description.object_count == 1U);
    assert(hth_world_default_spawn(&world, &spawn));
    assert(close_float(spawn.position.x, 0.0F));
    assert(signbit(spawn.position.y));
    assert(close_float(spawn.position.z, 0.5F));
    assert(close_float(spawn.yaw_radians, -0.25F));
    object = hth_world_static_object(&world, 0U);
    assert(object != NULL);
    assert(close_float(object->bounds.min.x, -100.0F));
    assert(close_float(object->bounds.max.x, 0.01F));
    assert(object->flags == 0U);
    assert(object->visual_class == HTH_WORLD_VISUAL_NONE);
    hth_world_shutdown(&world);
    hth_level_description_destroy(&description);
}

static void test_zero_object_level(void)
{
    static const unsigned char level[] = "hthlevel 1\nspawn 0 0 0 0";
    HTHLevelDescription description =
        parse_success(level, sizeof(level) - 1U);
    HTHWorld world = build_success(&description);

    assert(hth_world_static_object_count(&world) == 0U);
    hth_world_shutdown(&world);
    hth_level_description_destroy(&description);
}

static void test_flag_forms_and_visual_classes(void)
{
    static const unsigned char level[] =
        "hthlevel 1\nspawn 0 0 0 0\n"
        "static_object bounds 0 0 0 1 1 1 flags none visual none end\n"
        "static_object bounds 2 0 0 3 1 1 flags visible visual floor end\n"
        "static_object bounds 4 0 0 5 1 1 flags collidable visual wall end\n"
        "static_object bounds 6 0 0 7 1 1 flags collidable visible visual box end\n"
        "static_object bounds 8 0 0 9 1 1 flags visible visual low_step end\n"
        "static_object bounds 10 0 0 11 1 1 flags visible visual limit_step end\n"
        "static_object bounds 12 0 0 13 1 1 flags visible visual high_ledge end\n"
        "static_object bounds 14 0 0 15 1 1 flags visible visual platform end\n"
        "static_object bounds 16 0 0 17 1 1 flags visible visual corner end\n"
        "static_object bounds 18 0 0 19 1 1 flags visible visual corridor_corner end";
    HTHLevelDescription description =
        parse_success(level, sizeof(level) - 1U);
    HTHWorld world = build_success(&description);

    assert(description.object_count == 10U);
    assert(description.objects[0].object.flags == 0U);
    assert(description.objects[1].object.flags == HTH_WORLD_OBJECT_VISIBLE);
    assert(description.objects[2].object.flags ==
           HTH_WORLD_OBJECT_COLLIDABLE);
    assert(description.objects[3].object.flags ==
           (HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE));
    assert(description.objects[9].object.visual_class ==
           HTH_WORLD_VISUAL_CORRIDOR_CORNER);
    hth_world_shutdown(&world);
    hth_level_description_destroy(&description);
}

static void test_comments_newlines_and_eof(void)
{
    static const unsigned char comments[] =
        "# before header\n\n"
        "hthlevel 1 # format\n"
        "# before spawn\n"
        "spawn 0 0.5 0 0 # inline\n"
        "# comment at EOF";
    static const unsigned char crlf[] =
        "hthlevel 1\r\n\r\nspawn 0 0.5 0 0\r\n";
    static const unsigned char bare_cr[] =
        "hthlevel 1\rspawn 0 0.5 0 0\r";
    HTHLevelDescription first =
        parse_success(comments, sizeof(comments) - 1U);
    HTHLevelDescription second = parse_success(crlf, sizeof(crlf) - 1U);
    HTHLevelDescription third =
        parse_success(bare_cr, sizeof(bare_cr) - 1U);

    assert(close_float(first.default_spawn.position.y, 0.5F));
    assert(close_float(second.default_spawn.position.y, 0.5F));
    assert(close_float(third.default_spawn.position.y, 0.5F));
    hth_level_description_destroy(&first);
    hth_level_description_destroy(&second);
    hth_level_description_destroy(&third);
}

static void test_crlf_error_location(void)
{
    static const unsigned char level[] =
        "hthlevel 1\r\nspawn 0 nope 0 0\r\n";
    HTHLevelDescription description = {0};
    HTHLevelError error = {0};

    assert(!hth_level_parse(level, sizeof(level) - 1U,
                            &description, &error));
    assert(error.line == 2U);
    assert(error.column == 9U);
    assert(error.message[0] != '\0');
}

static void test_version_and_header_failures(void)
{
    static const char *const invalid[] = {
        "", "spawn 0 0 0 0", "hthlevel", "hthlevel 0\nspawn 0 0 0 0",
        "hthlevel 2\nspawn 0 0 0 0",
        "hthlevel -1\nspawn 0 0 0 0",
        "hthlevel 1.0\nspawn 0 0 0 0",
        "hthlevel 999999999999999999999999\nspawn 0 0 0 0",
        "hthlevel version\nspawn 0 0 0 0",
        "hthlevel 1\nhthlevel 1\nspawn 0 0 0 0",
        "hthlevel 1\nspawn 0 0 0 0\nhthlevel 1"
    };
    size_t index;

    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        assert_parse_failure((const unsigned char *)invalid[index],
                             strlen(invalid[index]));
    }
}

static void test_float_failures(void)
{
    static const char *const invalid[] = {
        "hthlevel 1\nspawn nan 0 0 0",
        "hthlevel 1\nspawn NaN 0 0 0",
        "hthlevel 1\nspawn inf 0 0 0",
        "hthlevel 1\nspawn INF 0 0 0",
        "hthlevel 1\nspawn infinity 0 0 0",
        "hthlevel 1\nspawn -Infinity 0 0 0",
        "hthlevel 1\nspawn 1foo 0 0 0",
        "hthlevel 1\nspawn 1e999999 0 0 0",
        "hthlevel 1\nspawn 1e 0 0 0",
        "hthlevel 1\nspawn . 0 0 0",
        "hthlevel 1\nspawn 0 0 0"
    };
    size_t index;

    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        assert_parse_failure((const unsigned char *)invalid[index],
                             strlen(invalid[index]));
    }
}

static void test_spawn_and_object_failures(void)
{
    static const char *const invalid[] = {
        "hthlevel 1",
        "hthlevel 1\nstatic_object bounds 0 0 0 1 1 1 flags none visual none end",
        "hthlevel 1\nspawn 0 0 0 0\nspawn 0 0 0 0",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object flags none visual none end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 visual none end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags none end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags none visual none",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 bounds 0 0 0 1 1 1 flags none visual none end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags none flags none visual none end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags none visual none visual none end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags mystery visual none end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags visible visible visual wall end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags visible collidable visual wall end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags none visible visual wall end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags visible visual mystery end",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object static_object bounds 0 0 0 1 1 1 flags none visual none end end",
        "hthlevel 1\nspawn 0 0 0 0\nend",
        "hthlevel 1\nspawn 0 0 0 0\ngarbage"
    };
    size_t index;

    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        assert_parse_failure((const unsigned char *)invalid[index],
                             strlen(invalid[index]));
    }
}

static void test_bom_nul_and_truncated_inputs(void)
{
    static const unsigned char bom[] = {
        0xEFU, 0xBBU, 0xBFU, 'h', 't', 'h', 'l', 'e', 'v', 'e', 'l',
        ' ', '1', '\n', 's', 'p', 'a', 'w', 'n', ' ', '0', ' ', '0',
        ' ', '0', ' ', '0'
    };
    static const unsigned char embedded_nul[] =
        "hthlevel 1\nspawn 0 0\0 0 0\n";
    static const char *const truncated[] = {
        "hthlevel", "hthlevel 1\nspawn 0 0 0 1e",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0",
        "hthlevel 1\nspawn 0 0 0 0\nstatic_object bounds 0 0 0 1 1 1 flags collidable visible visual wall",
        "hthlevel 1\nspawn 0 0 0 0\n# unfinished comment"
    };
    HTHLevelDescription comment_description;
    size_t index;

    assert_parse_failure(bom, sizeof(bom));
    assert_parse_failure(embedded_nul, sizeof(embedded_nul) - 1U);
    for (index = 0U;
         index + 1U < sizeof(truncated) / sizeof(truncated[0]); ++index) {
        assert_parse_failure((const unsigned char *)truncated[index],
                             strlen(truncated[index]));
    }
    comment_description = parse_success(
        (const unsigned char *)truncated[index], strlen(truncated[index]));
    hth_level_description_destroy(&comment_description);
}

static void test_world_validation_and_ownership(void)
{
    unsigned char level[] =
        "hthlevel 1\nspawn 1 2 3 0.5\n"
        "static_object bounds 0 0 0 0 1 1 flags collidable visual wall end";
    HTHLevelDescription description =
        parse_success(level, sizeof(level) - 1U);
    HTHLevelError error = {0};
    HTHWorld world = {0};

    memset(level, 'x', sizeof(level) - 1U);
    assert(!hth_level_build_world(&description, &world, &error));
    assert(error.message[0] != '\0');
    assert(!hth_world_is_finalized(&world));
    hth_world_shutdown(&world);
    hth_level_description_destroy(&description);
    hth_level_description_destroy(&description);

    {
        static const unsigned char valid[] =
            "hthlevel 1\nspawn 1 2 3 0.5\n"
            "static_object bounds 0 0 0 1 1 1 flags collidable visual wall end";
        HTHLevelDescription owned =
            parse_success(valid, sizeof(valid) - 1U);
        HTHWorld built = build_success(&owned);
        HTHWorldSpawn spawn;

        hth_level_description_destroy(&owned);
        assert(hth_world_default_spawn(&built, &spawn));
        assert_vec3(spawn.position, hth_vec3(1.0F, 2.0F, 3.0F));
        assert(hth_world_static_object_count(&built) == 1U);
        hth_world_shutdown(&built);
    }
}

static void test_null_arguments_and_output_preconditions(void)
{
    static const unsigned char valid[] = "hthlevel 1\nspawn 0 0 0 0";
    HTHLevelDescription description = {0};
    HTHLevelDescription occupied = {0};
    HTHLevelError error = {0};
    HTHWorld world = {0};
    HTHWorld occupied_world = {0};

    assert(!hth_level_parse(NULL, 0U, &description, NULL));
    assert(!hth_level_parse(NULL, 1U, &description, &error));
    assert(!hth_level_parse(valid, sizeof(valid) - 1U, NULL, &error));
    occupied.format_version = 99U;
    assert(!hth_level_parse(valid, sizeof(valid) - 1U, &occupied, &error));
    assert(occupied.format_version == 99U);
    assert(!hth_level_build_world(NULL, &world, &error));
    description = parse_success(valid, sizeof(valid) - 1U);
    assert(!hth_level_build_world(&description, NULL, &error));
    description.object_count = 1U;
    assert(!hth_level_build_world(&description, &world, &error));
    description.object_count = 0U;
    occupied_world.finalized = true;
    assert(!hth_level_build_world(&description, &occupied_world, &error));
    assert(occupied_world.finalized);
    assert(!hth_level_build_world(&description, &world, NULL));
    hth_level_description_destroy(&description);
    hth_level_description_destroy(NULL);
}

static void test_bootstrap_resource_integration_and_golden_content(void)
{
    static const HTHWorldStaticObject expected[] = {
        {{{-20.0F, -1.0F, -20.0F}, {20.0F, 0.0F, 20.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_FLOOR},
        {{{-6.0F, 0.0F, -14.0F}, {-5.0F, 3.0F, 2.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_WALL},
        {{{5.0F, 0.0F, -14.0F}, {6.0F, 3.0F, 2.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_WALL},
        {{{-6.0F, 0.0F, -14.0F}, {1.0F, 3.0F, -13.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_CORNER},
        {{{-1.0F, 0.0F, -2.5F}, {1.0F, 0.20F, -1.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_LOW_STEP},
        {{{2.0F, 0.0F, -3.5F}, {4.0F, 0.60F, -2.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_HIGH_LEDGE},
        {{{-4.0F, 0.0F, -5.5F}, {-2.5F, 1.20F, -4.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_BOX},
        {{{-1.0F, 0.0F, -7.0F}, {1.0F, 0.30F, -5.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_LIMIT_STEP},
        {{{2.5F, 0.0F, -10.0F}, {3.5F, 2.5F, -5.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_PLATFORM},
        {{{0.5F, 0.0F, -10.0F}, {1.5F, 2.5F, -9.0F}},
         HTH_WORLD_OBJECT_COLLIDABLE | HTH_WORLD_OBJECT_VISIBLE,
         HTH_WORLD_VISUAL_CORRIDOR_CORNER}
    };
    HTHResourceConfig resource_config = {HTH_DEVELOPMENT_RESOURCE_ROOT};
    HTHResourceSystem *resources =
        hth_resource_system_create(&resource_config);
    HTHResourceData data = {0};
    HTHLevelDescription description = {0};
    HTHLevelError error = {0};
    HTHWorld world = {0};
    HTHWorldSpawn spawn;
    HTHAABB bounds;
    HTHCollisionWorld collision;
    HTHPlayerBody body;
    HTHTrace trace;
    HTHVec3 player_mins;
    HTHVec3 player_maxs;
    size_t index;

    assert(resources != NULL);
    assert(!hth_resource_load(resources, "levels/missing.hthlevel", &data));
    assert(hth_resource_load(resources, "levels/bootstrap.hthlevel", &data));
    assert(hth_level_parse(data.data, data.size, &description, &error));
    hth_resource_data_release(&data);
    hth_resource_system_destroy(resources);
    assert(hth_level_build_world(&description, &world, &error));
    hth_level_description_destroy(&description);

    assert(hth_world_static_object_count(&world) ==
           sizeof(expected) / sizeof(expected[0]));
    for (index = 0U; index < sizeof(expected) / sizeof(expected[0]); ++index) {
        const HTHWorldStaticObject *actual =
            hth_world_static_object(&world, index);

        assert(actual != NULL);
        assert_vec3(actual->bounds.min, expected[index].bounds.min);
        assert_vec3(actual->bounds.max, expected[index].bounds.max);
        assert(actual->flags == expected[index].flags);
        assert(actual->visual_class == expected[index].visual_class);
    }
    assert(hth_world_bounds(&world, &bounds));
    assert_vec3(bounds.min, hth_vec3(-20.0F, -1.0F, -20.0F));
    assert_vec3(bounds.max, hth_vec3(20.0F, 3.0F, 20.0F));
    assert(hth_world_default_spawn(&world, &spawn));
    assert_vec3(spawn.position, hth_vec3(0.0F, 0.05F, 3.0F));
    assert(close_float(spawn.yaw_radians, 0.0F));
    assert(hth_collision_world_build_from_world(&collision, &world));
    assert(collision.obstacle_count == 10U);
    assert(hth_player_body_init(&body, spawn.position));
    player_mins = hth_vec3(-body.half_width, 0.0F, -body.half_width);
    player_maxs = hth_vec3(body.half_width, body.height, body.half_width);
    assert(hth_collision_world_trace_aabb(
        &collision, body.position, body.position,
        player_mins, player_maxs, &trace));
    assert(!trace.start_solid);
    hth_world_shutdown(&world);
}

int main(void)
{
    test_minimal_level_and_float_syntax();
    test_zero_object_level();
    test_flag_forms_and_visual_classes();
    test_comments_newlines_and_eof();
    test_crlf_error_location();
    test_version_and_header_failures();
    test_float_failures();
    test_spawn_and_object_failures();
    test_bom_nul_and_truncated_inputs();
    test_world_validation_and_ownership();
    test_null_arguments_and_output_preconditions();
    test_bootstrap_resource_integration_and_golden_content();
    return 0;
}
