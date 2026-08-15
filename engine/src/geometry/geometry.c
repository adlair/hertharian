#include "geometry.h"

#include <math.h>

/* Unit primitives occupy [-0.5, 0.5] on every axis. */
static const HTHRenderVertex box_vertices[] = {
    {{-0.5F, -0.5F,  0.5F}, {0.0F, 0.0F}},
    {{ 0.5F, -0.5F,  0.5F}, {1.0F, 0.0F}},
    {{ 0.5F,  0.5F,  0.5F}, {1.0F, 1.0F}},
    {{-0.5F,  0.5F,  0.5F}, {0.0F, 1.0F}},

    {{ 0.5F, -0.5F, -0.5F}, {0.0F, 0.0F}},
    {{-0.5F, -0.5F, -0.5F}, {1.0F, 0.0F}},
    {{-0.5F,  0.5F, -0.5F}, {1.0F, 1.0F}},
    {{ 0.5F,  0.5F, -0.5F}, {0.0F, 1.0F}},

    {{-0.5F, -0.5F, -0.5F}, {0.0F, 0.0F}},
    {{-0.5F, -0.5F,  0.5F}, {1.0F, 0.0F}},
    {{-0.5F,  0.5F,  0.5F}, {1.0F, 1.0F}},
    {{-0.5F,  0.5F, -0.5F}, {0.0F, 1.0F}},

    {{ 0.5F, -0.5F,  0.5F}, {0.0F, 0.0F}},
    {{ 0.5F, -0.5F, -0.5F}, {1.0F, 0.0F}},
    {{ 0.5F,  0.5F, -0.5F}, {1.0F, 1.0F}},
    {{ 0.5F,  0.5F,  0.5F}, {0.0F, 1.0F}},

    {{-0.5F,  0.5F,  0.5F}, {0.0F, 0.0F}},
    {{ 0.5F,  0.5F,  0.5F}, {1.0F, 0.0F}},
    {{ 0.5F,  0.5F, -0.5F}, {1.0F, 1.0F}},
    {{-0.5F,  0.5F, -0.5F}, {0.0F, 1.0F}},

    {{-0.5F, -0.5F, -0.5F}, {0.0F, 0.0F}},
    {{ 0.5F, -0.5F, -0.5F}, {1.0F, 0.0F}},
    {{ 0.5F, -0.5F,  0.5F}, {1.0F, 1.0F}},
    {{-0.5F, -0.5F,  0.5F}, {0.0F, 1.0F}}
};

static const uint32_t box_indices[] = {
     0U,  1U,  2U,  0U,  2U,  3U,
     4U,  5U,  6U,  4U,  6U,  7U,
     8U,  9U, 10U,  8U, 10U, 11U,
    12U, 13U, 14U, 12U, 14U, 15U,
    16U, 17U, 18U, 16U, 18U, 19U,
    20U, 21U, 22U, 20U, 22U, 23U
};

/* The wedge's inclined face rises from y=-0.5 to y=0.5 toward +Z. */
static const HTHRenderVertex wedge_vertices[] = {
    /* Bottom. */
    {{-0.5F, -0.5F, -0.5F}, {0.0F, 0.0F}},
    {{ 0.5F, -0.5F, -0.5F}, {1.0F, 0.0F}},
    {{ 0.5F, -0.5F,  0.5F}, {1.0F, 1.0F}},
    {{-0.5F, -0.5F,  0.5F}, {0.0F, 1.0F}},
    /* High +Z face. */
    {{-0.5F, -0.5F,  0.5F}, {0.0F, 0.0F}},
    {{ 0.5F, -0.5F,  0.5F}, {1.0F, 0.0F}},
    {{ 0.5F,  0.5F,  0.5F}, {1.0F, 1.0F}},
    {{-0.5F,  0.5F,  0.5F}, {0.0F, 1.0F}},
    /* -X triangle. */
    {{-0.5F, -0.5F, -0.5F}, {0.0F, 0.0F}},
    {{-0.5F, -0.5F,  0.5F}, {1.0F, 0.0F}},
    {{-0.5F,  0.5F,  0.5F}, {1.0F, 1.0F}},
    /* +X triangle. */
    {{ 0.5F, -0.5F, -0.5F}, {0.0F, 0.0F}},
    {{ 0.5F,  0.5F,  0.5F}, {1.0F, 1.0F}},
    {{ 0.5F, -0.5F,  0.5F}, {1.0F, 0.0F}},
    /* Inclined face. */
    {{-0.5F, -0.5F, -0.5F}, {0.0F, 0.0F}},
    {{-0.5F,  0.5F,  0.5F}, {0.0F, 1.0F}},
    {{ 0.5F,  0.5F,  0.5F}, {1.0F, 1.0F}},
    {{ 0.5F, -0.5F, -0.5F}, {1.0F, 0.0F}}
};

static const uint32_t wedge_indices[] = {
     0U,  1U,  2U,  0U,  2U,  3U,
     4U,  5U,  6U,  4U,  6U,  7U,
     8U,  9U, 10U,
    11U, 12U, 13U,
    14U, 15U, 16U, 14U, 16U, 17U
};

bool hth_geometry_get(HTHGeometryPrimitive primitive,
                      HTHGeometryView *out_geometry)
{
    HTHGeometryView geometry;

    if (out_geometry == NULL) {
        return false;
    }
    switch (primitive) {
    case HTH_GEOMETRY_PRIMITIVE_BOX:
        geometry.vertices = box_vertices;
        geometry.vertex_count = sizeof(box_vertices) / sizeof(box_vertices[0]);
        geometry.indices = box_indices;
        geometry.index_count = sizeof(box_indices) / sizeof(box_indices[0]);
        break;
    case HTH_GEOMETRY_PRIMITIVE_WEDGE:
        geometry.vertices = wedge_vertices;
        geometry.vertex_count =
            sizeof(wedge_vertices) / sizeof(wedge_vertices[0]);
        geometry.indices = wedge_indices;
        geometry.index_count =
            sizeof(wedge_indices) / sizeof(wedge_indices[0]);
        break;
    case HTH_GEOMETRY_PRIMITIVE_COUNT:
    default:
        return false;
    }
    *out_geometry = geometry;
    return true;
}

static bool geometry_is_valid(HTHGeometryView geometry)
{
    size_t index;

    if (geometry.vertices == NULL || geometry.vertex_count == 0U ||
        geometry.indices == NULL || geometry.index_count == 0U ||
        geometry.index_count % 3U != 0U) {
        return false;
    }
    for (index = 0U; index < geometry.vertex_count; ++index) {
        size_t component;

        for (component = 0U; component < 3U; ++component) {
            float value = geometry.vertices[index].position[component];

            if (!isfinite(value) || value < -0.5F || value > 0.5F) {
                return false;
            }
        }
        for (component = 0U; component < 2U; ++component) {
            float value = geometry.vertices[index].uv[component];

            if (!isfinite(value) || value < 0.0F || value > 1.0F) {
                return false;
            }
        }
    }
    for (index = 0U; index < geometry.index_count; ++index) {
        if (geometry.indices[index] >= geometry.vertex_count) {
            return false;
        }
    }
    for (index = 0U; index < geometry.index_count; index += 3U) {
        const float *a = geometry.vertices[geometry.indices[index]].position;
        const float *b =
            geometry.vertices[geometry.indices[index + 1U]].position;
        const float *c =
            geometry.vertices[geometry.indices[index + 2U]].position;
        float ab_x = b[0] - a[0];
        float ab_y = b[1] - a[1];
        float ab_z = b[2] - a[2];
        float ac_x = c[0] - a[0];
        float ac_y = c[1] - a[1];
        float ac_z = c[2] - a[2];
        float cross_x = ab_y * ac_z - ab_z * ac_y;
        float cross_y = ab_z * ac_x - ab_x * ac_z;
        float cross_z = ab_x * ac_y - ab_y * ac_x;

        if (cross_x * cross_x + cross_y * cross_y +
                cross_z * cross_z <= 0.0F) {
            return false;
        }
    }
    return true;
}

bool hth_geometry_library_is_valid(void)
{
    int primitive;

    for (primitive = 0; primitive < (int)HTH_GEOMETRY_PRIMITIVE_COUNT;
         ++primitive) {
        HTHGeometryView geometry;

        if (!hth_geometry_get((HTHGeometryPrimitive)primitive, &geometry) ||
            !geometry_is_valid(geometry)) {
            return false;
        }
    }
    return true;
}
