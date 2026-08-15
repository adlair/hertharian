#include "geometry.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stddef.h>

static void validate_geometry(HTHGeometryPrimitive primitive,
                              size_t expected_vertices,
                              size_t expected_indices)
{
    HTHGeometryView geometry;
    float minimum[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    float maximum[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    size_t index;

    assert(hth_geometry_get(primitive, &geometry));
    assert(geometry.vertex_count == expected_vertices);
    assert(geometry.index_count == expected_indices);
    assert(geometry.index_count % 3U == 0U);
    for (index = 0U; index < geometry.vertex_count; ++index) {
        size_t component;

        for (component = 0U; component < 3U; ++component) {
            float value = geometry.vertices[index].position[component];

            assert(isfinite(value));
            assert(value >= -0.5F && value <= 0.5F);
            minimum[component] = fminf(minimum[component], value);
            maximum[component] = fmaxf(maximum[component], value);
        }
        for (component = 0U; component < 2U; ++component) {
            float value = geometry.vertices[index].uv[component];

            assert(isfinite(value));
            assert(value >= 0.0F && value <= 1.0F);
        }
    }
    for (index = 0U; index < 3U; ++index) {
        assert(minimum[index] == -0.5F);
        assert(maximum[index] == 0.5F);
    }
    for (index = 0U; index < geometry.index_count; index += 3U) {
        const float *a;
        const float *b;
        const float *c;
        float ab[3];
        float ac[3];
        float cross[3];

        assert(geometry.indices[index] < geometry.vertex_count);
        assert(geometry.indices[index + 1U] < geometry.vertex_count);
        assert(geometry.indices[index + 2U] < geometry.vertex_count);
        a = geometry.vertices[geometry.indices[index]].position;
        b = geometry.vertices[geometry.indices[index + 1U]].position;
        c = geometry.vertices[geometry.indices[index + 2U]].position;
        ab[0] = b[0] - a[0];
        ab[1] = b[1] - a[1];
        ab[2] = b[2] - a[2];
        ac[0] = c[0] - a[0];
        ac[1] = c[1] - a[1];
        ac[2] = c[2] - a[2];
        cross[0] = ab[1] * ac[2] - ab[2] * ac[1];
        cross[1] = ab[2] * ac[0] - ab[0] * ac[2];
        cross[2] = ab[0] * ac[1] - ab[1] * ac[0];
        assert(cross[0] * cross[0] + cross[1] * cross[1] +
                   cross[2] * cross[2] > 0.0F);
    }
}

int main(void)
{
    HTHGeometryView geometry = {0};

    assert(hth_geometry_library_is_valid());
    validate_geometry(HTH_GEOMETRY_PRIMITIVE_BOX, 24U, 36U);
    validate_geometry(HTH_GEOMETRY_PRIMITIVE_WEDGE, 18U, 24U);
    assert(!hth_geometry_get(HTH_GEOMETRY_PRIMITIVE_COUNT, &geometry));
    assert(!hth_geometry_get((HTHGeometryPrimitive)-1, &geometry));
    assert(!hth_geometry_get(HTH_GEOMETRY_PRIMITIVE_BOX, NULL));
    return 0;
}
