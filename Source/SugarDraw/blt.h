#pragma once

#include "common.h"

void blt_blit(u8* dst, u32 dst_x, u32 dst_y, u32 dst_w, u32 dst_h, const DDPIXELFORMAT* dst_format, u32 dst_stride, const RGBQUAD* dst_palette,
    const u8* src, u32 src_x, u32 src_y, u32 src_w, u32 src_h, const DDPIXELFORMAT* src_format, u32 src_stride, const RGBQUAD* src_palette);
void blt_dst_color_key(u8* dst, u32 dst_x, u32 dst_y, u32 dst_w, u32 dst_h, const DDPIXELFORMAT* dst_format, u32 dst_stride,
    const u8* src, u32 src_x, u32 src_y, u32 src_w, u32 src_h, const DDPIXELFORMAT* src_format, u32 src_stride, u32 ckl, u32 ckh);
void blt_src_color_key(u8* dst, u32 dst_x, u32 dst_y, u32 dst_w, u32 dst_h, const DDPIXELFORMAT* dst_format, u32 dst_stride,
    const u8* src, u32 src_x, u32 src_y, u32 src_w, u32 src_h, const DDPIXELFORMAT* src_format, u32 src_stride, u32 ckl, u32 ckh);
