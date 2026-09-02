#ifndef HTH_RENDERER_TYPES_H
#define HTH_RENDERER_TYPES_H

#include "aabb.h"
#include "geometry.h"
#include "hth_math.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    HTHGeometryPrimitive primitive;
    HTHAABB bounds;
    float base_color[4];
    const unsigned char *texture_pixels;
    uint32_t texture_width;
    uint32_t texture_height;
    bool has_texture;
} HTHRendererStaticDraw;

typedef struct {
    HTHGeometryPrimitive primitive;
    HTHMat4 model;
    float base_color[4];
} HTHRendererTransientDraw;

#endif
