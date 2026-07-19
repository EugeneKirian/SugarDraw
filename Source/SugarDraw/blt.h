#pragma once

#include "common.h"

void blt_blit(u8* dst, s32 dst_x, s32 dst_y, s32 dst_w, s32 dst_h, const DDPIXELFORMAT* dst_format, s32 dst_stride, const RGBQUAD* dst_palette,
    const u8* src, s32 src_x, s32 src_y, s32 src_w, s32 src_h, const DDPIXELFORMAT* src_format, s32 src_stride, const RGBQUAD* src_palette);
void blt_color_key(u8* dst, s32 dst_x, s32 dst_y, s32 dst_w, s32 dst_h, s32 dst_bpp, s32 dst_stride,
    const u8* src, s32 src_x, s32 src_y, s32 src_w, s32 src_h, s32 src_bpp, s32 src_stride, u32 ckl, u32 ckh);
