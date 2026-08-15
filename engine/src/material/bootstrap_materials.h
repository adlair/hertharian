#ifndef HTH_BOOTSTRAP_MATERIALS_H
#define HTH_BOOTSTRAP_MATERIALS_H

#include "image.h"
#include "material.h"
#include "resource.h"
#include "world.h"

#include <stdbool.h>

typedef struct HTHBootstrapMaterialSet HTHBootstrapMaterialSet;

typedef struct {
    const char *material_resource_id;
    const HTHMaterialDescription *description;
    const HTHImageData *image;
} HTHBootstrapMaterial;

HTHBootstrapMaterialSet *hth_bootstrap_materials_load(
    const HTHResourceSystem *resources);
void hth_bootstrap_materials_destroy(HTHBootstrapMaterialSet *materials);
bool hth_bootstrap_materials_get(
    const HTHBootstrapMaterialSet *materials,
    HTHWorldVisualClass visual_class,
    HTHBootstrapMaterial *out_material);

#endif
