#pragma once

#include "arena.h"

// 3D Color Palette Lookup Table (LUT) for 24-bit to 8-bit color quantization.
//
// Maps a 15-bit quantized RGB space (5 bits per channel: 32x32x32) directly to an 
// 8 - bit palette index. Memory footprint is exactly 32 KB (32,768 bytes), which fits
// comfortably inside modern CPU L1 data caches for high - throughput spatial indexing.
//
// Indexing: index = colors[R >> 3][G >> 3][B >> 3]

typedef struct plt {
    u8 colors[32][32][32]; // Stores palette indices [0..255]
} plt;

#define PLT_X8R8G8B8_INDEX(PAL, COL)    (PAL->colors[((COL & 0x00FF0000) >> 16) >> 3][((COL & 0x0000FF00) >> 8) >> 3][(COL & 0x00FF00FF) >> 3])
#define PLT_QUAD_INDEX(PAL, QUAD)       (PAL->colors[QUAD.rgbRed >> 3][QUAD.rgbGreen >> 3][QUAD.rgbBlue >> 3])

// Generates a pre-computed grid mapping every 5-bit RGB cell to its nearest match 
// in the provided palette. Uses the Redmean weighted Euclidean distance metric
// to approximate human visual sensitivity (perceptually favoring green over blue).

static HRESULT plt_create(arena* arena, u32 count, const RGBQUAD* quads, plt** object);
