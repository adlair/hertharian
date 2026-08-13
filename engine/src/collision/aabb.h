#ifndef HTH_AABB_H
#define HTH_AABB_H

#include "hth_math.h"

#include <stdbool.h>

typedef struct {
    HTHVec3 min;
    HTHVec3 max;
} HTHAABB;

bool hth_aabb_is_valid(const HTHAABB *bounds);
bool hth_aabb_intersects(const HTHAABB *left, const HTHAABB *right);

#endif
