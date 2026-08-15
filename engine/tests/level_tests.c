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
    assert(error.line > 0U && error.column > 0U);
    assert(error.message[0] != '\0');
    hth_level_description_destroy(&description);
}

static HTHWorld build_success(const HTHLevelDescription *description)
{
    HTHLevelError error = {0};
    HTHWorld world = {0};

    assert(hth_level_build_world(description, &world, &error));
    assert(error.message[0] == '\0');
    return world;
}

static void test_v2_valid_matrix_and_layouts(void)
{
    static const unsigned char level[] =
        "# header\r\nhthlevel 2\r\n"
        "spawn 0 -0 0.5 -0.25\r\n"
        "static_object bounds -1e2 -1 -1 1e-2 1 1 "
        "collision aabb render box flags collidable visible visual box end\r\n"
        "static_object bounds 2 0 0 3 1 1 "
        "collision none render wedge flags visible visual wall end\r\n"
        "static_object bounds 4 0 0 5 1 1 "
        "collision aabb render none flags collidable visual none end\r\n"
        "static_object bounds 6 0 0 7 1 1 "
        "collision none render none flags none visual none end";
    HTHLevelDescription description =
        parse_success(level, sizeof(level) - 1U);
    HTHWorld world = build_success(&description);
    const HTHWorldStaticObject *object;

    assert(description.format_version == 2U);
    assert(description.object_count == 4U);
    assert(signbit(description.default_spawn.position.y));
    object = hth_world_static_object(&world, 1U);
    assert(object != NULL);
    assert(object->collision_shape == HTH_WORLD_COLLISION_NONE);
    assert(object->render_shape == HTH_WORLD_RENDER_WEDGE);
    assert(object->flags == HTH_WORLD_OBJECT_VISIBLE);
    object = hth_world_static_object(&world, 2U);
    assert(object != NULL);
    assert(object->collision_shape == HTH_WORLD_COLLISION_AABB);
    assert(object->render_shape == HTH_WORLD_RENDER_NONE);
    hth_world_shutdown(&world);
    hth_level_description_destroy(&description);
}

static void test_comments_newlines_and_zero_objects(void)
{
    static const unsigned char comments[] =
        "# before\nhthlevel 2 # format\nspawn 0 0 0 0 # EOF comment";
    static const unsigned char bare_cr[] =
        "hthlevel 2\rspawn 0 0 0 0\r";
    HTHLevelDescription first =
        parse_success(comments, sizeof(comments) - 1U);
    HTHLevelDescription second =
        parse_success(bare_cr, sizeof(bare_cr) - 1U);

    assert(first.object_count == 0U);
    assert(second.object_count == 0U);
    hth_level_description_destroy(&first);
    hth_level_description_destroy(&second);
}

static void test_version_rejection(void)
{
    static const unsigned char v1[] = "hthlevel 1\nspawn 0 0 0 0";
    HTHLevelDescription description = {0};
    HTHLevelError error = {0};

    assert(!hth_level_parse(v1, sizeof(v1) - 1U, &description, &error));
    assert(strstr(error.message, "unsupported") != NULL);
    assert_parse_failure((const unsigned char *)"hthlevel 3\nspawn 0 0 0 0",
                         strlen("hthlevel 3\nspawn 0 0 0 0"));
    assert_parse_failure((const unsigned char *)"hthlevel 2.0\nspawn 0 0 0 0",
                         strlen("hthlevel 2.0\nspawn 0 0 0 0"));
}

static void test_shape_and_order_failures(void)
{
    static const char *const invalid[] = {
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 render box flags visible visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision none flags visible visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision capsule render box "
        "flags visible visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision none render mesh "
        "flags visible visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 render box collision none "
        "flags visible visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision none collision none render box "
        "flags visible visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision none render box render box "
        "flags visible visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision none render none "
        "flags visible visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision aabb render box "
        "flags visible visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision none render wedge "
        "flags none visual box end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision aabb render none "
        "flags none visual none end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision none render box "
        "flags visible visual mystery end",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object "
        "bounds 0 0 0 1 1 1 collision none render box "
        "flags visible visual box",
        "hthlevel 2\nspawn 0 0 0 0\ngarbage"
    };
    size_t index;

    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        assert_parse_failure((const unsigned char *)invalid[index],
                             strlen(invalid[index]));
    }
}

static void test_numeric_bom_nul_and_truncation(void)
{
    static const char *const invalid[] = {
        "", "hthlevel 2", "hthlevel 2\nspawn nan 0 0 0",
        "hthlevel 2\nspawn 1e9999 0 0 0",
        "hthlevel 2\nspawn 0 0 0",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object",
        "hthlevel 2\nspawn 0 0 0 0\nstatic_object bounds 0 0"
    };
    static const unsigned char nul[] = "hthlevel 2\nspawn 0 0\0 0 0";
    static const unsigned char bom[] = {
        0xEFU, 0xBBU, 0xBFU, 'h', 't', 'h', 'l', 'e', 'v', 'e', 'l',
        ' ', '2', '\n', 's', 'p', 'a', 'w', 'n', ' ', '0', ' ', '0',
        ' ', '0', ' ', '0'
    };
    size_t index;

    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        assert_parse_failure((const unsigned char *)invalid[index],
                             strlen(invalid[index]));
    }
    assert_parse_failure(nul, sizeof(nul) - 1U);
    assert_parse_failure(bom, sizeof(bom));
}

static void test_world_build_transactionality_and_ownership(void)
{
    static const unsigned char source[] =
        "hthlevel 2\nspawn 1 2 3 0.5\nstatic_object "
        "bounds 0 0 0 1 1 1 collision aabb render none "
        "flags collidable visual none end";
    HTHLevelDescription description =
        parse_success(source, sizeof(source) - 1U);
    HTHLevelError error = {0};
    HTHWorld world = {0};

    description.objects[0].object.collision_shape = HTH_WORLD_COLLISION_NONE;
    assert(!hth_level_build_world(&description, &world, &error));
    assert(!hth_world_is_finalized(&world));
    assert(world.objects == NULL);
    description.objects[0].object.collision_shape = HTH_WORLD_COLLISION_AABB;
    world = build_success(&description);
    hth_level_description_destroy(&description);
    assert(hth_world_static_object_count(&world) == 1U);
    hth_world_shutdown(&world);
}

static void test_bootstrap_golden_content(void)
{
    static const HTHAABB historical_bounds[] = {
        {{-20.0F, -1.0F, -20.0F}, {20.0F, 0.0F, 20.0F}},
        {{-6.0F, 0.0F, -14.0F}, {-5.0F, 3.0F, 2.0F}},
        {{5.0F, 0.0F, -14.0F}, {6.0F, 3.0F, 2.0F}},
        {{-6.0F, 0.0F, -14.0F}, {1.0F, 3.0F, -13.0F}},
        {{-1.0F, 0.0F, -2.5F}, {1.0F, 0.20F, -1.0F}},
        {{2.0F, 0.0F, -3.5F}, {4.0F, 0.60F, -2.0F}},
        {{-4.0F, 0.0F, -5.5F}, {-2.5F, 1.20F, -4.0F}},
        {{-1.0F, 0.0F, -7.0F}, {1.0F, 0.30F, -5.0F}},
        {{2.5F, 0.0F, -10.0F}, {3.5F, 2.5F, -5.0F}},
        {{0.5F, 0.0F, -10.0F}, {1.5F, 2.5F, -9.0F}}
    };
    static const HTHWorldVisualClass historical_visuals[] = {
        HTH_WORLD_VISUAL_FLOOR, HTH_WORLD_VISUAL_WALL,
        HTH_WORLD_VISUAL_WALL, HTH_WORLD_VISUAL_CORNER,
        HTH_WORLD_VISUAL_LOW_STEP, HTH_WORLD_VISUAL_HIGH_LEDGE,
        HTH_WORLD_VISUAL_BOX, HTH_WORLD_VISUAL_LIMIT_STEP,
        HTH_WORLD_VISUAL_PLATFORM, HTH_WORLD_VISUAL_CORRIDOR_CORNER
    };
    HTHResourceConfig config = {HTH_DEVELOPMENT_RESOURCE_ROOT};
    HTHResourceSystem *resources = hth_resource_system_create(&config);
    HTHResourceData data = {0};
    HTHLevelDescription description = {0};
    HTHLevelError error = {0};
    HTHWorld world = {0};
    HTHCollisionWorld collision;
    HTHAABB world_bounds;
    size_t index;

    assert(resources != NULL);
    assert(hth_resource_load(resources, "levels/bootstrap.hthlevel", &data));
    assert(hth_level_parse(data.data, data.size, &description, &error));
    hth_resource_data_release(&data);
    hth_resource_system_destroy(resources);
    assert(description.format_version == 2U);
    assert(hth_level_build_world(&description, &world, &error));
    hth_level_description_destroy(&description);
    assert(hth_world_static_object_count(&world) == 11U);
    for (index = 0U; index < 10U; ++index) {
        const HTHWorldStaticObject *object =
            hth_world_static_object(&world, index);

        assert(object != NULL);
        assert_vec3(object->bounds.min, historical_bounds[index].min);
        assert_vec3(object->bounds.max, historical_bounds[index].max);
        assert(object->collision_shape == HTH_WORLD_COLLISION_AABB);
        assert(object->render_shape == HTH_WORLD_RENDER_BOX);
        assert(object->flags == (HTH_WORLD_OBJECT_COLLIDABLE |
                                 HTH_WORLD_OBJECT_VISIBLE));
        assert(object->visual_class == historical_visuals[index]);
    }
    {
        const HTHWorldStaticObject *wedge =
            hth_world_static_object(&world, 10U);

        assert(wedge != NULL);
        assert_vec3(wedge->bounds.min, hth_vec3(-4.5F, 0.0F, -11.0F));
        assert_vec3(wedge->bounds.max, hth_vec3(-2.5F, 1.5F, -9.0F));
        assert(wedge->collision_shape == HTH_WORLD_COLLISION_NONE);
        assert(wedge->render_shape == HTH_WORLD_RENDER_WEDGE);
        assert(wedge->flags == HTH_WORLD_OBJECT_VISIBLE);
        assert(wedge->visual_class == HTH_WORLD_VISUAL_BOX);
    }
    assert(hth_world_bounds(&world, &world_bounds));
    assert_vec3(world_bounds.min, hth_vec3(-20.0F, -1.0F, -20.0F));
    assert_vec3(world_bounds.max, hth_vec3(20.0F, 3.0F, 20.0F));
    assert(hth_collision_world_build_from_world(&collision, &world));
    assert(collision.obstacle_count == 10U);
    for (index = 0U; index < 10U; ++index) {
        assert_vec3(collision.obstacles[index].min,
                    historical_bounds[index].min);
        assert_vec3(collision.obstacles[index].max,
                    historical_bounds[index].max);
    }
    hth_world_shutdown(&world);
}

static void test_arguments(void)
{
    static const unsigned char valid[] = "hthlevel 2\nspawn 0 0 0 0";
    HTHLevelDescription description = {0};
    HTHLevelDescription occupied = {0};
    HTHLevelError error = {0};
    HTHWorld world = {0};

    assert(!hth_level_parse(NULL, 0U, &description, NULL));
    assert(!hth_level_parse(NULL, 1U, &description, &error));
    assert(!hth_level_parse(valid, sizeof(valid) - 1U, NULL, &error));
    occupied.format_version = 99U;
    assert(!hth_level_parse(valid, sizeof(valid) - 1U, &occupied, &error));
    description = parse_success(valid, sizeof(valid) - 1U);
    assert(!hth_level_build_world(NULL, &world, &error));
    assert(!hth_level_build_world(&description, NULL, &error));
    assert(!hth_level_build_world(&description, &world, NULL));
    hth_level_description_destroy(&description);
    hth_level_description_destroy(NULL);
}

int main(void)
{
    test_v2_valid_matrix_and_layouts();
    test_comments_newlines_and_zero_objects();
    test_version_rejection();
    test_shape_and_order_failures();
    test_numeric_bom_nul_and_truncation();
    test_world_build_transactionality_and_ownership();
    test_bootstrap_golden_content();
    test_arguments();
    return 0;
}
