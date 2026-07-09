#include "utilities.h"

#include <stdio.h>

const char* log_level_to_string(log_level level) {
    switch (level) {
    case LOG_LEVEL_ERROR: { return "ERROR"; }break;
    case LOG_LEVEL_WARNING: { return "WARNING"; }break;
    case LOG_LEVEL_INFO: { return "INFO"; }break;
    case LOG_LEVEL_TRACE: { return "TRACE"; }break;
    }

    return "None";
}

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

const char* hresult_to_string(HRESULT hr) {
    return "TODO"; // TODO
}

HRESULT rgb_quad_to_palette_entry(const RGBQUAD* quads, u32 count, PALETTEENTRY* entries) {
    if (quads == NULL || entries == NULL) {
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

    // A DIB color table has its colors stored BGR not RGB.
    for (u32 i = 0; i < count; i++) {
        quads[i].rgbRed = entries[i].peRed;
        quads[i].rgbGreen = entries[i].peGreen;
        quads[i].rgbBlue = entries[i].peBlue;
        quads[i].rgbReserved = entries[i].peFlags;
    }

    return DD_OK;
}