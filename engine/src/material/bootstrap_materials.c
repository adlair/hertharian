#include "bootstrap_materials.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    const char *material_resource_id;
    HTHMaterialDescription description;
    HTHImageData image;
} HTHBootstrapMaterialEntry;

struct HTHBootstrapMaterialSet {
    HTHBootstrapMaterialEntry entries[HTH_WORLD_VISUAL_COUNT];
};

static const char *material_id_for_visual_class(
    HTHWorldVisualClass visual_class)
{
    switch (visual_class) {
    case HTH_WORLD_VISUAL_NONE:
        return "materials/bootstrap/none.hthmat";
    case HTH_WORLD_VISUAL_FLOOR:
        return "materials/bootstrap/floor.hthmat";
    case HTH_WORLD_VISUAL_WALL:
        return "materials/bootstrap/wall.hthmat";
    case HTH_WORLD_VISUAL_BOX:
        return "materials/bootstrap/box.hthmat";
    case HTH_WORLD_VISUAL_LOW_STEP:
        return "materials/bootstrap/low_step.hthmat";
    case HTH_WORLD_VISUAL_LIMIT_STEP:
        return "materials/bootstrap/limit_step.hthmat";
    case HTH_WORLD_VISUAL_HIGH_LEDGE:
        return "materials/bootstrap/high_ledge.hthmat";
    case HTH_WORLD_VISUAL_PLATFORM:
        return "materials/bootstrap/platform.hthmat";
    case HTH_WORLD_VISUAL_CORNER:
        return "materials/bootstrap/corner.hthmat";
    case HTH_WORLD_VISUAL_CORRIDOR_CORNER:
        return "materials/bootstrap/corridor_corner.hthmat";
    case HTH_WORLD_VISUAL_COUNT:
        break;
    }
    return NULL;
}

void hth_bootstrap_materials_destroy(HTHBootstrapMaterialSet *materials)
{
    size_t index;

    if (materials == NULL) {
        return;
    }
    for (index = 0U; index < HTH_WORLD_VISUAL_COUNT; ++index) {
        hth_image_data_release(&materials->entries[index].image);
        hth_material_description_destroy(
            &materials->entries[index].description);
    }
    free(materials);
}

static bool load_material_description(
    const HTHResourceSystem *resources,
    HTHBootstrapMaterialEntry *entry)
{
    HTHMaterialError error = {0};
    HTHResourceData data = {0};

    if (!hth_resource_load(resources, entry->material_resource_id, &data)) {
        fprintf(stderr, "Failed to load material resource:\n  %s\n",
                entry->material_resource_id);
        return false;
    }
    if (!hth_material_parse(data.data, data.size, &entry->description,
                            &error)) {
        fprintf(stderr, "Material parse failed: %s:%zu:%zu:\n  %s\n",
                entry->material_resource_id, error.line, error.column,
                error.message);
        hth_resource_data_release(&data);
        return false;
    }
    hth_resource_data_release(&data);
    return true;
}

static bool load_material_image(const HTHResourceSystem *resources,
                                HTHBootstrapMaterialEntry *entry)
{
    HTHImageError error = {0};
    HTHResourceData data = {0};
    const char *resource_id = entry->description.texture_resource_id;

    if (!entry->description.has_texture) {
        return true;
    }
    if (!hth_resource_load(resources, resource_id, &data)) {
        fprintf(stderr, "Failed to load texture resource:\n  %s\n",
                resource_id);
        return false;
    }
    if (!hth_image_decode_ppm_p3(data.data, data.size, &entry->image,
                                 &error)) {
        fprintf(stderr, "Image decode failed: %s:%zu:%zu:\n  %s\n",
                resource_id, error.line, error.column, error.message);
        hth_resource_data_release(&data);
        return false;
    }
    hth_resource_data_release(&data);
    return true;
}

HTHBootstrapMaterialSet *hth_bootstrap_materials_load(
    const HTHResourceSystem *resources)
{
    HTHBootstrapMaterialSet *materials;
    size_t index;

    if (resources == NULL) {
        return NULL;
    }
    materials = calloc(1, sizeof(*materials));
    if (materials == NULL) {
        fputs("Bootstrap material allocation failed.\n", stderr);
        return NULL;
    }
    for (index = 0U; index < HTH_WORLD_VISUAL_COUNT; ++index) {
        HTHBootstrapMaterialEntry *entry = &materials->entries[index];

        entry->material_resource_id = material_id_for_visual_class(
            (HTHWorldVisualClass)index);
        if (entry->material_resource_id == NULL ||
            !load_material_description(resources, entry) ||
            !load_material_image(resources, entry)) {
            hth_bootstrap_materials_destroy(materials);
            return NULL;
        }
    }
    puts("Bootstrap materials and textures validated.");
    return materials;
}

bool hth_bootstrap_materials_get(
    const HTHBootstrapMaterialSet *materials,
    HTHWorldVisualClass visual_class,
    HTHBootstrapMaterial *out_material)
{
    const HTHBootstrapMaterialEntry *entry;

    if (materials == NULL || out_material == NULL ||
        visual_class < HTH_WORLD_VISUAL_NONE ||
        visual_class >= HTH_WORLD_VISUAL_COUNT) {
        return false;
    }
    entry = &materials->entries[(size_t)visual_class];
    if (entry->material_resource_id == NULL ||
        entry->description.format_version != HTH_MATERIAL_FORMAT_VERSION ||
        (entry->description.has_texture &&
         (entry->image.pixels == NULL || entry->image.width == 0U ||
          entry->image.height == 0U ||
          entry->image.format != HTH_IMAGE_FORMAT_RGB8))) {
        return false;
    }
    out_material->material_resource_id = entry->material_resource_id;
    out_material->description = &entry->description;
    out_material->image = entry->description.has_texture
        ? &entry->image : NULL;
    return true;
}
