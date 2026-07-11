#include "blt.h"

// TODO blitter:
// TODO internal arena for overlapped memory copying
// CPU flags for proper method selection: none, MMS, SSE, AVX

void static blt_copy(u8* dst, const u8* src, u32 size) {
    // TODO SIMD
    CopyMemory(dst, src, size);
}

void blt_blit(u8* dst, s32 dst_x, s32 dst_y, s32 dst_w, s32 dst_h, s32 dst_bpp, s32 dst_stride,
    const u8* src, s32 src_x, s32 src_y, s32 src_w, s32 src_h, s32 src_bpp, s32 src_stride) {
    // TODO incomplete, proper implementation

    if (dst_bpp == src_bpp) {
        // TODO support 1,2,4-bit colors
        const u32 bytes = dst_bpp / 8;
        const u32 min_w = max(0, min(dst_w - dst_x, src_w));
        const u32 min_h = max(0, min(dst_h - dst_y, src_h));
        const u32 length = min_w * bytes;

        for (u32 i = 0; i < min_h; i++) {
            const u8* source = (src + (i + src_y) * src_stride + src_x * bytes);
            u8* destination = (dst + (i + dst_y) * dst_stride + dst_x * bytes);
            blt_copy(destination, source, length);
        }
    }
    else {
        // TODO
    }
}

void blt_color_key(u8* dst, s32 dst_x, s32 dst_y, s32 dst_w, s32 dst_h, s32 dst_bpp, s32 dst_stride,
    const u8* src, s32 src_x, s32 src_y, s32 src_w, s32 src_h, s32 src_bpp, s32 src_stride, u32 ckl, u32 ckh) {
    // TODO incomplete, proper implementation

    if (dst_bpp == src_bpp) {
        // TODO support 1,2,4-bit colors
        const u32 bytes = dst_bpp / 8;
        const u32 min_w = max(0, min(dst_w - dst_x, src_w));
        const u32 min_h = max(0, min(dst_h - dst_y, src_h));
        const u32 length = min_w * bytes;

        // TODO SIMD

        if (dst_bpp == 8) {
            for (u32 i = 0; i < min_h; i++) {
                const u8* source = (src + (i + src_y) * src_stride + src_x * bytes);
                u8* destination = (dst + (i + dst_y) * dst_stride + dst_x * bytes);
                for (u32 x = 0; x < length; x++) {
                    const u8 color = source[x];
                    if (color < ckl || ckh < color) {
                        destination[x] = color;
                    }
                }
            }
        }

        // TODO other bpps
    }
    else {
        // TODO
    }
}
