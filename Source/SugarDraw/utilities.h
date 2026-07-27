#pragma once

#include "common.h"

u32 next_power_of_2(u32 n);

HRESULT rgb_quad_to_palette_entry(const RGBQUAD* quads, u32 count, PALETTEENTRY* entries);
HRESULT palette_entry_to_rgb_quad(const PALETTEENTRY* entries, u32 count, RGBQUAD* quads);

HRESULT ddpixelformat_validate(const DDPIXELFORMAT* format);
HRESULT ddpixelformat_validate_rgb(const DDPIXELFORMAT* format);
HRESULT ddpixelformat_validate_yuv(const DDPIXELFORMAT* format);

HRESULT ddsurfacedesc2_from_devmodea(DDSURFACEDESC2* desc, const DEVMODEA* mode);
