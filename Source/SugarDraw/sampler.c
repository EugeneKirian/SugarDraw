#include "sampler.h"

#define _USE_MATH_DEFINES
#include <math.h>

// Directly computes an affine sampler that maps the destination rectangle
// onto the source rectangle, optionally rotated by angle degrees
// clockwise around the source center.
//
// Instead of building a forward affine matrix and then inverting it, this
// function derives the partial derivatives (dudx, dudy, dvdx, dvdy) and
// the origin offset (u0, v0) directly from the geometry. This shortcut
// removes a full 3×3 matrix setup + inversion, improving both speed and
// numerical stability for the 2D blitting case.

HRESULT sampler_create(arena* arena, const RECT* dst,
    const RECT* src, f32 angle, u32 flags, sampler** object) {
    if (arena == NULL || src == NULL || dst == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    sampler* instance = NULL;
    if (SUCCEEDED(hr = arena_allocate(arena, sizeof(sampler), &instance))) {
        // 1. Calculate dimensions
        const f32 src_w = (f32)(src->right - src->left);
        const f32 src_h = (f32)(src->bottom - src->top);
        const f32 dst_w = (f32)(dst->right - dst->left);
        const f32 dst_h = (f32)(dst->bottom - dst->top);

        // Reject zero or negative rectangle dimensions
        if (src_w <= 0.0f || src_h <= 0.0f
            || dst_w <= 0.0f || dst_h <= 0.0f) {
            return DDERR_INVALIDPARAMS;
        }

        // 2. Calculate center points of rectangles
        const f32 src_cx = (f32)src->left + src_w * 0.5f;
        const f32 src_cy = (f32)src->top + src_h * 0.5f;
        const f32 dst_cx = (f32)dst->left + dst_w * 0.5f;
        const f32 dst_cy = (f32)dst->top + dst_h * 0.5f;

        // 3. Scale factors including flipping flags
        const f32 sx = (dst_w / src_w) * ((flags & DDBLTFX_MIRRORLEFTRIGHT) ? -1.0f : 1.0f);
        const f32 sy = (dst_h / src_h) * ((flags & DDBLTFX_MIRRORUPDOWN) ? -1.0f : 1.0f);

        // 4. Rotation terms
        const f32 rad = angle * (f32)(M_PI / 180.0);
        const f32 cos_a = cosf(rad);
        const f32 sin_a = sinf(rad);

        // 5. Partial derivatives (source step size per destination pixel)
        instance->dudx = cos_a / sx;
        instance->dvdx = -sin_a / sy;
        instance->dudy = sin_a / sx;
        instance->dvdy = cos_a / sy;

        // 6. Source coordinate at Destination (0, 0)
        instance->u0 = src_cx - (instance->dudx * dst_cx + instance->dudy * dst_cy);
        instance->v0 = src_cy - (instance->dvdx * dst_cx + instance->dvdy * dst_cy);

        *object = instance;
    }

    return hr;
}
