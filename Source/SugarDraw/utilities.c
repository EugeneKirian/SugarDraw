#include "utilities.h"

u32 next_power_of_2(u32 n) {
    if (n == 0) {
        return 1;
    }

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return (n + 1);
}

HRESULT rgb_quad_to_palette_entry(const RGBQUAD* quads, u32 count, PALETTEENTRY* entries) {
    if (quads == NULL || entries == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (count > PALETTE_MAX_ENTRY_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    // A DIB color table has its colors stored BGR not RGB.
    for (u32 i = 0; i < count; i++) {
        entries[i].peRed = quads[i].rgbRed;
        entries[i].peGreen = quads[i].rgbGreen;
        entries[i].peBlue = quads[i].rgbBlue;
        entries[i].peFlags = quads[i].rgbReserved;
    }

    return DD_OK;
}

HRESULT palette_entry_to_rgb_quad(const PALETTEENTRY* entries, u32 count, RGBQUAD* quads) {
    if (quads == NULL || entries == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (count > PALETTE_MAX_ENTRY_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    // A DIB color table has its colors stored BGR not RGB.
    for (u32 i = 0; i < count; i++) {
        quads[i].rgbRed = entries[i].peRed;
        quads[i].rgbGreen = entries[i].peGreen;
        quads[i].rgbBlue = entries[i].peBlue;
        quads[i].rgbReserved = entries[i].peFlags;
    }

    return DD_OK;
}
