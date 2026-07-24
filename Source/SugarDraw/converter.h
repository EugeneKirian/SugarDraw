#pragma once

#include "logger.h"

const char* devmodea_to_string(const DEVMODEA* mode);

const char* ddbltfx_to_string(const DDBLTFX* effects);
const char* ddcolorcontrol_to_string(const DDCOLORCONTROL* control);
const char* ddcolorkey_to_string(const DDCOLORKEY* key);
const char* ddpixelformat_to_string(const DDPIXELFORMAT* format);
const char* ddscaps_to_string(const DDSCAPS* caps);
const char* ddscaps2_to_string(const DDSCAPS2* caps);
const char* ddsurfacedesc_to_string(const DDSURFACEDESC* desc);
const char* ddsurfacedesc2_to_string(const DDSURFACEDESC2* desc);

const char* ddblt_to_string(const u32 flags);
const char* ddbltfast_to_string(const u32 flags);
const char* ddedm_to_string(const u32 flags);
const char* ddenum_to_string(const u32 flags);
const char* ddenumsurfaces_to_string(const u32 flags);
const char* ddflip_to_string(const u32 flags);
const char* ddgbs_to_string(const u32 flags);
const char* ddgdi_to_string(const u32 flags);
const char* ddlock_to_string(const u32 flags);
const char* ddpcaps_to_string(const u32 flags);
const char* ddpf_to_string(const u32 flags);
const char* ddscl_to_string(const u32 flags);
const char* ddsd_to_string(const u32 flags);
const char* ddsdm_to_string(const u32 flags);
const char* ddsgr_to_string(const u32 flags);
const char* ddwaitvb_to_string(const u32 flags);

const char* disp_change_to_string(const s32 result);
const char* dm_to_string(const u32 flags);

const char* guid_to_string(const GUID* id);
const char* hresult_to_string(const HRESULT hr);
const char* log_level_to_string(log_level level);
const char* rect_to_string(const RECT* rect);
