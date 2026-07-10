#pragma once
#include "common.h"

void blt_blit(u8* dst, int dst_x, int dst_y, int dst_w, int dst_h, int dst_bpp, int dst_stride,
    const u8* src, int src_x, int src_y, int src_w, int src_h, int src_bpp, int src_stride);
void blt_color_key(u8* dst, int dst_x, int dst_y, int dst_w, int dst_h, int dst_bpp, int dst_stride,
    const u8* src, int src_x, int src_y, int src_w, int src_h, int src_bpp, int src_stride, u32 ckl, u32 ckh);
