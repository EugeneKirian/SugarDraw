#pragma once

#include "arena.h"

// Affine source-coordinate sampler for scanline-oriented blitting.
// Maps each destination pixel (x, y) to continuous source coordinates (u, v)
// through the affine relations:
//     u(x, y) = u0 + dudx * x + dudy * y
//     v(x, y) = v0 + dvdx * x + dvdy * y
// The coefficients are chosen so that the sampler can be advanced
// incrementally both across a scanline and between scanlines:
//     u(x+1, y) = u(x, y) + dudx     v(x+1, y) = v(x, y) + dvdx
//     u(x, y+1) = u(x, y) + dudy     v(x, y+1) = v(x, y) + dvdy
// This avoids per-pixel matrix multiplication during the blit loop.

typedef struct sampler {
    f32 u0;       /* source U coordinate */
    f32 v0;       /* source V coordinate */
    f32 dudx;     /* partial derivative: du/dx */
    f32 dvdx;     /* partial derivative: dv/dx */
    f32 dudy;     /* partial derivative: du/dy */
    f32 dvdy;     /* partial derivative: dv/dy */
} sampler;

HRESULT sampler_create(arena* arena, const RECT* dst,
    const RECT* src, f32 angle, u32 flags, sampler** object);
