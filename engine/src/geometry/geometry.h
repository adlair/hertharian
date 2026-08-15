#ifndef HTH_GEOMETRY_H
#define HTH_GEOMETRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    HTH_GEOMETRY_PRIMITIVE_BOX = 0,
    HTH_GEOMETRY_PRIMITIVE_WEDGE,
    HTH_GEOMETRY_PRIMITIVE_COUNT
} HTHGeometryPrimitive;

typedef struct {
    float position[3];
    float uv[2];
} HTHRenderVertex;

typedef struct {
    const HTHRenderVertex *vertices;
    size_t vertex_count;
    const uint32_t *indices;
    size_t index_count;
} HTHGeometryView;

bool hth_geometry_get(HTHGeometryPrimitive primitive,
                      HTHGeometryView *out_geometry);
bool hth_geometry_library_is_valid(void);

#endif
