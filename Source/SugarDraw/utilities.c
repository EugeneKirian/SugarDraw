#include "utilities.h"

static u32 get_bit_count(const u32 mask) {
    u32 count = 0;
    u32 value = mask;

    while (value != 0) {
        value = value >> 1;
        count++;
    }

    return count;
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

HRESULT ddpixelformat_validate(const DDPIXELFORMAT* format) {
    if (format == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (format->dwSize != sizeof(DDPIXELFORMAT)) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO validate flags

    if (!(format->dwFlags & DDPF_FOURCC)) {
        if (format->dwFourCC != FOURCC_NONE) {
            return DDERR_INVALIDPIXELFORMAT;
        }
    }

    if (format->dwFlags & DDPF_RGB) {
        return ddpixelformat_validate_rgb(format);
    }

    if (format->dwFlags & DDPF_YUV) {
        return ddpixelformat_validate_yuv(format);
    }

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT ddpixelformat_validate_rgb(const DDPIXELFORMAT* format) {
    if (format == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (format->dwSize != sizeof(DDPIXELFORMAT)) {
        return DDERR_INVALIDPARAMS;
    }

    if (format->dwFlags & DDPF_PALETTEINDEXEDTO8) {
        return DDERR_UNSUPPORTED; // TODO
    }

    switch (format->dwRGBBitCount) {
    case 1: {
        if (!(format->dwFlags & DDPF_PALETTEINDEXED1)) {
            return DDERR_INVALIDPIXELFORMAT;
        }
    }break;
    case 2: {
        if (!(format->dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2))) {
            return DDERR_INVALIDPIXELFORMAT;
        }
    }break;
    case 4: {
        if (!(format->dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4))) {
            return DDERR_INVALIDPIXELFORMAT;
        }
    }break;
    case 8: {
        if (!(format->dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8))) {
            return DDERR_INVALIDPIXELFORMAT;
        }
    }break;
    case 15: {
        if (format->dwRBitMask != 0x00007C00
            || format->dwGBitMask != 0x000003E0
            || format->dwBBitMask != 0x0000001F) {
            return DDERR_INVALIDPIXELFORMAT;
        }

        if (format->dwFlags & DDPF_ALPHAPIXELS) {
            return DDERR_INVALIDPIXELFORMAT;
        }
    }break;
    case 16: {
        const u32 red_bits = get_bit_count(format->dwRBitMask);
        const u32 green_bits = get_bit_count(format->dwGBitMask);
        const u32 blue_bits = get_bit_count(format->dwBBitMask);

        if (red_bits == 5 && green_bits == 5 && blue_bits == 5) {
            if (format->dwRBitMask != 0x00007C00
                || format->dwGBitMask != 0x000003E0
                || format->dwBBitMask != 0x0000001F) {
                return DDERR_INVALIDPIXELFORMAT;
            }

            if (format->dwFlags & DDPF_ALPHAPIXELS) {
                return DDERR_INVALIDPIXELFORMAT;
            }
        }
        else if (red_bits == 5 && green_bits == 6 && blue_bits == 5) {
            if (format->dwRBitMask != 0x0000F800
                || format->dwGBitMask != 0x000007E0
                || format->dwBBitMask != 0x0000001F) {
                return DDERR_INVALIDPIXELFORMAT;
            }

            if (format->dwFlags & DDPF_ALPHAPIXELS) {
                if (format->dwRGBAlphaBitMask != 0x00008000) {
                    return DDERR_INVALIDPIXELFORMAT;
                }
            }
        }
    }break;
    case 24: {
        if (format->dwRBitMask != 0x00FF0000
            || format->dwGBitMask != 0x0000FF00
            || format->dwBBitMask != 0x000000FF) {
            return DDERR_INVALIDPIXELFORMAT;
        }

        if (format->dwFlags & DDPF_ALPHAPIXELS) {
            return DDERR_INVALIDPIXELFORMAT;
        }
    }break;
    case 32: {
        if (format->dwRBitMask != 0x00FF0000
            || format->dwGBitMask != 0x0000FF00
            || format->dwBBitMask != 0x000000FF) {
            return DDERR_INVALIDPIXELFORMAT;
        }

        if (format->dwFlags & DDPF_ALPHAPIXELS) {
            if (format->dwRGBAlphaBitMask != 0xFF000000) {
                return DDERR_INVALIDPIXELFORMAT;
            }
        }
    }break;
    default: { return DDERR_INVALIDPIXELFORMAT; }
    }

    return DD_OK;
}

HRESULT ddpixelformat_validate_yuv(const DDPIXELFORMAT* format) {
    if (format == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (format->dwSize != sizeof(DDPIXELFORMAT)) {
        return DDERR_INVALIDPARAMS;
    }

    return DDERR_UNSUPPORTED;
}

HRESULT ddsurfacedesc2_from_devmodea(DDSURFACEDESC2* desc, const DEVMODEA* mode) {
    if (desc == NULL || mode == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2) || mode->dmSize != sizeof(DEVMODEA)) {
        return DDERR_INVALIDPARAMS;
    }

    desc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    desc->dwWidth = mode->dmPelsWidth;
    desc->dwHeight = mode->dmPelsHeight;

    ZeroMemory(&desc->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
    desc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

    desc->ddpfPixelFormat.dwFlags = DDPF_RGB;
    desc->ddpfPixelFormat.dwRGBBitCount = mode->dmBitsPerPel;

    switch (mode->dmBitsPerPel) {
    case 1: {
        desc->ddsCaps.dwCaps |= DDSCAPS_PALETTE;
        desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED1;
        return DDERR_UNSUPPORTEDFORMAT; // TODO
    }break;
    case 2: {
        desc->ddsCaps.dwCaps |= DDSCAPS_PALETTE;
        desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED2;
        return DDERR_UNSUPPORTEDFORMAT; // TODO
    }break;
    case 4: {
        desc->ddsCaps.dwCaps |= DDSCAPS_PALETTE;
        desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED4;
        return DDERR_UNSUPPORTEDFORMAT; // TODO
    }break;
    case 8: {
        desc->ddsCaps.dwCaps |= DDSCAPS_PALETTE;
        desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
    }break;
    case 15: {
        desc->ddpfPixelFormat.dwRBitMask = 0x00007C00;
        desc->ddpfPixelFormat.dwGBitMask = 0x000003E0;
        desc->ddpfPixelFormat.dwBBitMask = 0x0000001F;
    }break;
    case 16: {
        desc->ddpfPixelFormat.dwRBitMask = 0x0000F800;
        desc->ddpfPixelFormat.dwGBitMask = 0x000007E0;
        desc->ddpfPixelFormat.dwBBitMask = 0x0000001F;
    }break;
    case 24:
    case 32: {
        desc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
        desc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
        desc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
    }break;
    }

    return DD_OK;
}
