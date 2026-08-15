#include "hth_resource_config.h"
#include "bootstrap_materials.h"
#include "material.h"
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

static HTHMaterialDescription parse_success(const unsigned char *data,
                                            size_t size)
{
    HTHMaterialDescription description = {0};
    HTHMaterialError error = {0};

    assert(hth_material_parse(data, size, &description, &error));
    assert(error.message[0] == '\0');
    return description;
}

static void assert_parse_failure(const unsigned char *data, size_t size)
{
    HTHMaterialDescription description = {0};
    HTHMaterialError error = {0};

    assert(!hth_material_parse(data, size, &description, &error));
    assert(description.format_version == 0U);
    assert(description.texture_resource_id == NULL);
    assert(!description.has_texture);
    assert(error.line > 0U);
    assert(error.column > 0U);
    assert(error.message[0] != '\0');
    hth_material_description_destroy(&description);
}

static void test_valid_materials(void)
{
    static const unsigned char untextured[] =
        "# material\r\n"
        "hthmaterial 1\r\n"
        "base_color 0 1 0.25 1 # boundaries\r\n"
        "texture none";
    static unsigned char textured[] =
        "hthmaterial 1\n"
        "base_color 1.0 0.5 0.25 1\n"
        "texture textures/bootstrap/test.ppm";
    HTHMaterialDescription first =
        parse_success(untextured, sizeof(untextured) - 1U);
    HTHMaterialDescription second =
        parse_success(textured, sizeof(textured) - 1U);

    assert(first.format_version == HTH_MATERIAL_FORMAT_VERSION);
    assert(!first.has_texture);
    assert(first.texture_resource_id == NULL);
    assert(close_float(first.base_color[0], 0.0F));
    assert(close_float(first.base_color[1], 1.0F));
    assert(second.has_texture);
    assert(strcmp(second.texture_resource_id,
                  "textures/bootstrap/test.ppm") == 0);
    memset(textured, 'x', sizeof(textured) - 1U);
    assert(strcmp(second.texture_resource_id,
                  "textures/bootstrap/test.ppm") == 0);
    hth_material_description_destroy(&first);
    hth_material_description_destroy(&second);
    hth_material_description_destroy(&second);
}

static void test_invalid_materials(void)
{
    static const char *const invalid[] = {
        "",
        "material 1\nbase_color 1 1 1 1\ntexture none",
        "hthmaterial",
        "hthmaterial 0\nbase_color 1 1 1 1\ntexture none",
        "hthmaterial 2\nbase_color 1 1 1 1\ntexture none",
        "hthmaterial -1\nbase_color 1 1 1 1\ntexture none",
        "hthmaterial 1.0\nbase_color 1 1 1 1\ntexture none",
        "hthmaterial 1\ntexture none",
        "hthmaterial 1\nbase_color 1 1 1\ntexture none",
        "hthmaterial 1\nbase_color 1 1 1 1",
        "hthmaterial 1\nbase_color 1 1 1 1\nbase_color 1 1 1 1\ntexture none",
        "hthmaterial 1\nbase_color 1 1 1 1\ntexture none\ntexture none",
        "hthmaterial 1\nbase_color nope 1 1 1\ntexture none",
        "hthmaterial 1\nbase_color nan 1 1 1\ntexture none",
        "hthmaterial 1\nbase_color inf 1 1 1\ntexture none",
        "hthmaterial 1\nbase_color -0.01 1 1 1\ntexture none",
        "hthmaterial 1\nbase_color 1.01 1 1 1\ntexture none",
        "hthmaterial 1\nbase_color 1 1 1 1\ntexture ../bad.ppm",
        "hthmaterial 1\nbase_color 1 1 1 1\ntexture Textures/bad.ppm",
        "hthmaterial 1\nbase_color 1 1 1 1\ntexture none\ngarbage"
    };
    static const unsigned char embedded_nul[] =
        "hthmaterial 1\nbase_color 1 1\0 1 1\ntexture none";
    static const unsigned char bom[] = {
        0xEFU, 0xBBU, 0xBFU, 'h', 't', 'h', 'm', 'a', 't', 'e', 'r',
        'i', 'a', 'l', ' ', '1'
    };
    size_t index;

    for (index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        assert_parse_failure((const unsigned char *)invalid[index],
                             strlen(invalid[index]));
    }
    assert_parse_failure(embedded_nul, sizeof(embedded_nul) - 1U);
    assert_parse_failure(bom, sizeof(bom));
}

static void test_arguments(void)
{
    static const unsigned char valid[] =
        "hthmaterial 1\nbase_color 1 1 1 1\ntexture none";
    HTHMaterialDescription description = {0};
    HTHMaterialDescription occupied = {0};
    HTHMaterialError error = {0};

    assert(!hth_material_parse(NULL, 0U, &description, &error));
    assert(error.message[0] != '\0');
    assert(!hth_material_parse(NULL, 1U, &description, &error));
    assert(!hth_material_parse(valid, sizeof(valid) - 1U, NULL, &error));
    assert(!hth_material_parse(valid, sizeof(valid) - 1U,
                               &description, NULL));
    occupied.format_version = 99U;
    assert(!hth_material_parse(valid, sizeof(valid) - 1U,
                               &occupied, &error));
    assert(occupied.format_version == 99U);
    hth_material_description_destroy(NULL);
}

static void test_bootstrap_material_graph(void)
{
    static const char *const expected_ids[HTH_WORLD_VISUAL_COUNT] = {
        "materials/bootstrap/none.hthmat",
        "materials/bootstrap/floor.hthmat",
        "materials/bootstrap/wall.hthmat",
        "materials/bootstrap/box.hthmat",
        "materials/bootstrap/low_step.hthmat",
        "materials/bootstrap/limit_step.hthmat",
        "materials/bootstrap/high_ledge.hthmat",
        "materials/bootstrap/platform.hthmat",
        "materials/bootstrap/corner.hthmat",
        "materials/bootstrap/corridor_corner.hthmat"
    };
    static const float expected_colors[HTH_WORLD_VISUAL_COUNT][4] = {
        {1.00F, 1.00F, 1.00F, 1.00F},
        {0.22F, 0.24F, 0.28F, 1.00F},
        {0.30F, 0.45F, 0.62F, 1.00F},
        {0.90F, 0.20F, 0.48F, 1.00F},
        {0.18F, 0.78F, 0.32F, 1.00F},
        {1.00F, 0.72F, 0.10F, 1.00F},
        {0.95F, 0.30F, 0.12F, 1.00F},
        {0.16F, 0.68F, 0.78F, 1.00F},
        {0.55F, 0.28F, 0.75F, 1.00F},
        {0.48F, 0.30F, 0.72F, 1.00F}
    };
    static const char *const expected_texture_ids[HTH_WORLD_VISUAL_COUNT] = {
        NULL,
        "textures/bootstrap/floor.ppm",
        "textures/bootstrap/wall.ppm",
        "textures/bootstrap/box.ppm",
        NULL, NULL, NULL, NULL, NULL, NULL
    };
    HTHResourceConfig config = {HTH_DEVELOPMENT_RESOURCE_ROOT};
    HTHResourceSystem *resources = hth_resource_system_create(&config);
    HTHBootstrapMaterialSet *materials;
    size_t index;

    assert(resources != NULL);
    materials = hth_bootstrap_materials_load(resources);
    hth_resource_system_destroy(resources);
    assert(materials != NULL);
    for (index = 0U; index < HTH_WORLD_VISUAL_COUNT; ++index) {
        HTHBootstrapMaterial material;
        size_t component;

        assert(hth_bootstrap_materials_get(
            materials, (HTHWorldVisualClass)index, &material));
        assert(strcmp(material.material_resource_id,
                      expected_ids[index]) == 0);
        for (component = 0U; component < 4U; ++component) {
            assert(close_float(material.description->base_color[component],
                               expected_colors[index][component]));
        }
        if (index == HTH_WORLD_VISUAL_FLOOR ||
            index == HTH_WORLD_VISUAL_WALL ||
            index == HTH_WORLD_VISUAL_BOX) {
            assert(material.description->has_texture);
            assert(strcmp(material.description->texture_resource_id,
                          expected_texture_ids[index]) == 0);
            assert(material.image != NULL);
            assert(material.image->pixels != NULL);
            assert(material.image->format == HTH_IMAGE_FORMAT_RGB8);
            if (index == HTH_WORLD_VISUAL_FLOOR) {
                assert(material.image->width == 5U);
                assert(material.image->height == 4U);
                assert(material.image->pixels[0] == 220U);
            } else {
                assert(material.image->width == 4U);
                assert(material.image->height == 4U);
                assert(material.image->pixels[0] == 255U);
            }
        } else {
            assert(!material.description->has_texture);
            assert(material.image == NULL);
        }
    }
    {
        HTHBootstrapMaterial material;

        assert(!hth_bootstrap_materials_get(
            materials, HTH_WORLD_VISUAL_COUNT, &material));
        assert(!hth_bootstrap_materials_get(NULL,
                                            HTH_WORLD_VISUAL_FLOOR,
                                            &material));
    }
    hth_bootstrap_materials_destroy(materials);
    hth_bootstrap_materials_destroy(NULL);
}

int main(void)
{
    test_valid_materials();
    test_invalid_materials();
    test_arguments();
    test_bootstrap_material_graph();
    return 0;
}
