#pragma once
#include "logger.h"

const char* log_level_to_string(log_level level);
const char* hresult_to_string(HRESULT hr);

u32 next_power_of_2(u32 n);

HRESULT rgb_quad_to_palette_entry(const RGBQUAD* quads, u32 count, PALETTEENTRY* entries);
HRESULT palette_entry_to_rgb_quad(const PALETTEENTRY* entries, u32 count, RGBQUAD* quads);
