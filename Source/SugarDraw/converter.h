#pragma once

#include "logger.h"

const char* devmodea_to_string(const DEVMODEA* mode);

const char* ddcolorkey_to_string(const DDCOLORKEY* key);
const char* ddpixelformat_to_string(const DDPIXELFORMAT* format);
const char* ddsurfacedesc1_to_string(const DDSURFACEDESC* desc);

const char* ddsgr_to_string(const u32 flags);
const char* ddsdm_to_string(const u32 flags);
const char* ddsd_to_string(const u32 flags);
const char* ddscl_to_string(const u32 flags);
const char* ddpf_to_string(const u32 flags);
const char* ddpcaps_to_string(const u32 flags);
const char* ddlock_to_string(const u32 flags);
const char* ddenum_to_string(const u32 flags);

const char* disp_change_to_string(const s32 result);
const char* dm_to_string(const u32 flags);

const char* guid_to_string(const GUID* id);
const char* hresult_to_string(const HRESULT hr);
const char* log_level_to_string(log_level level);
const char* rect_to_string(const RECT* rect);
