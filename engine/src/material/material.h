#ifndef HTH_MATERIAL_H
#define HTH_MATERIAL_H

#include <stdbool.h>
#include <stddef.h>

#define HTH_MATERIAL_FORMAT_VERSION 1U
#define HTH_MATERIAL_ERROR_MESSAGE_CAPACITY 160U

typedef struct {
    unsigned int format_version;
    float base_color[4];
    char *texture_resource_id;
    bool has_texture;
} HTHMaterialDescription;

typedef struct {
    size_t line;
    size_t column;
    char message[HTH_MATERIAL_ERROR_MESSAGE_CAPACITY];
} HTHMaterialError;

bool hth_material_parse(const unsigned char *data, size_t size,
                        HTHMaterialDescription *out_description,
                        HTHMaterialError *out_error);
void hth_material_description_destroy(HTHMaterialDescription *description);

#endif
