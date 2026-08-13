#include "aabb.h"

#include <math.h>
#include <stddef.h>

bool hth_aabb_is_valid(const HTHAABB *bounds)
{
    return bounds != NULL &&
           isfinite(bounds->min.x) && isfinite(bounds->min.y) &&
           isfinite(bounds->min.z) && isfinite(bounds->max.x) &&
           isfinite(bounds->max.y) && isfinite(bounds->max.z) &&
           bounds->min.x < bounds->max.x &&
           bounds->min.y < bounds->max.y &&
           bounds->min.z < bounds->max.z;
}

bool hth_aabb_intersects(const HTHAABB *left, const HTHAABB *right)
{
    if (!hth_aabb_is_valid(left) || !hth_aabb_is_valid(right)) {
        return false;
    }

    /* Sharing a face is contact, not penetration. */
    return left->min.x < right->max.x && left->max.x > right->min.x &&
           left->min.y < right->max.y && left->max.y > right->min.y &&
           left->min.z < right->max.z && left->max.z > right->min.z;
}
