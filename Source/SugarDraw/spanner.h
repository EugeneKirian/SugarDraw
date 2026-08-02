#pragma once

#include "arena.h"

// A destination span represents a contiguous horizontal run of pixels
// on a single scanline (row) of the destination surface.
// Each span is exactly 1 pixel tall and is defined by an inclusive
// start X (x0), an exclusive end X (x1), and a Y coordinate.
// Spans are produced by the spanner to decompose complex clip regions
// into simple, non-overlapping segments that can be processed in
// parallel without overdraw or complex intersection tests.

typedef struct span {
    s32 x0; // Inclusive left edge
    s32 x1; // Exclusive right edge
    s32 y;  // Scanline (row) coordinate
} span;

// The spanner object converts the input surface rectangle and a list of
// clip recrangles into a series of horizontal spans (destination spans)
// that are 1 pixel worth of height.
// This is done so that drawing pixels into the destination area(s)
// is as simple and parallelizable as possible.
// Alternative options which were explored and discarded:
// 1. Using the clip region rectangles as they provided by the region data.
//      This method creates complex validation rules, introduces
//      overdraw when clip rectangles overlap, and hard to parallelize.
// 2. Using a GPU-like stencil buffer to mark pixels as valid/invalid.
//      This method requires heavy memory use, still has an overdraw,
//      just into the stencil, and provides limited parallelization.
// 3. Using Y-Banding method that produces vertically aligned rectangles
//      out of clip region. Y-Banding method, although appealing,
//      still computationally more complex to implement, and the outputs
//      can't be as easily parallelized given oportunity for high variance
//      within the produced resulting set of rectangles.
// Therefore, the resulting spans can be independently processed in paralel
// on the CPU, and the processing of each scan line is a linear process.

typedef struct spanner {
    arena*      arena;
    span*       spans;
    u32         count, capacity;
} spanner;

HRESULT spanner_create(arena* arena, const RECT* rect,
    const RECT* dst, const RGNDATA* region, spanner** object);
