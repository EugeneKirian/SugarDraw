#include "blt.h"

// TODO blitter:
// CPU flags for proper method selection: none, MMS, SSE, AVX

// TODO internal arena for overlapped memory copying
static u8 data[4 * 1024 * 1024]; // TODO

void static blt_copy(u8* dst, const u8* src, u32 size) {
    // TODO SIMD
    CopyMemory(dst, src, size);
}

void blt_blit(u8* dst, u32 dst_x, u32 dst_y, u32 dst_w, u32 dst_h, const DDPIXELFORMAT* dst_format, u32 dst_stride, const RGBQUAD* dst_palette,
    const u8* src, u32 src_x, u32 src_y, u32 src_w, u32 src_h, const DDPIXELFORMAT* src_format, u32 src_stride, const RGBQUAD* src_palette) {
    // TODO incomplete, proper implementation

    // TODO overlaps

    if (dst_format->dwRGBBitCount == src_format->dwRGBBitCount) {
        // TODO support 1,2,4-bit colors
        const u32 bytes = dst_format->dwRGBBitCount / 8;
        const u32 min_w = max(0, min(dst_w - dst_x, src_w - src_x));
        const u32 min_h = max(0, min(dst_h - dst_y, src_h - src_y));
        const u32 length = min_w * bytes;

        for (u32 i = 0; i < min_h; i++) {
            const u8* source = (src + (i + src_y) * src_stride + src_x * bytes);
            u8* destination = (dst + (i + dst_y) * dst_stride + dst_x * bytes);
            blt_copy(destination, source, length);
        }
    }
    else if (dst_format->dwRGBBitCount == 32 && src_format->dwRGBBitCount == 16) {
        const u32 dst_bytes = dst_format->dwRGBBitCount / 8;
        const u32 src_bytes = src_format->dwRGBBitCount / 8;
        const u32 min_w = max(0, min(dst_w - dst_x, src_w - src_x));
        const u32 min_h = max(0, min(dst_h - dst_y, src_h - src_y));

        // 5-5-5
        if (src_format->dwRBitMask == 0x7C00
            && src_format->dwGBitMask == 0x03E0
            && src_format->dwBBitMask == 0x001F)
        {
            for (u32 i = 0; i < min_h; i++) {
                const u16* source = (u16*)(src + (i + src_y) * src_stride + src_x * src_bytes);
                u32* destination = (u32*)(dst + (i + dst_y) * dst_stride + dst_x * dst_bytes);
                for (u32 ii = 0; ii < min_w; ii++) {
                    const u16 color = (u32)source[ii];
                    const u32 r = (color >> 10) & 0x1F;
                    const u32 g = (color >> 5) & 0x1F;
                    const u32 b = (color >> 0) & 0x1F;

                    const u32 nr = (r << 3) | (r >> 2);
                    const u32 ng = (g << 3) | (g >> 2);
                    const u32 nb = (b << 3) | (b >> 2);

                    destination[ii] = (0xFF << 24) | (nr << 16) | (ng << 8) | nb;
                }
            }

            return;
        }

        // 5-6-5
        if (src_format->dwRBitMask == 0xF800
            && src_format->dwGBitMask == 0x07E0
            && src_format->dwBBitMask == 0x001F)
        {
            for (u32 i = 0; i < min_h; i++) {
                const u16* source = (u16*)(src + (i + src_y) * src_stride + src_x * src_bytes);
                u32* destination = (u32*)(dst + (i + dst_y) * dst_stride + dst_x * dst_bytes);
                for (u32 ii = 0; ii < min_w; ii++) {
                    const u16 color = (u32)source[ii];
                    const u32 r = (color >> 10) & 0x1F;
                    const u32 g = (color >> 5) & 0x1F;
                    const u32 b = (color >> 0) & 0x1F;

                    const u32 nr = (r << 3) | (r >> 2);
                    const u32 ng = (g << 2) | (g >> 4);
                    const u32 nb = (b << 3) | (b >> 2);

                    destination[ii] = (0xFF << 24) | (nr << 16) | (ng << 8) | nb;
                }
            }

            return;
        }
    }
    else if (dst_format->dwRGBBitCount == 32 && src_format->dwRGBBitCount == 8) {
        const u32 dst_bytes = dst_format->dwRGBBitCount / 8;
        const u32 src_bytes = src_format->dwRGBBitCount / 8;
        const u32 min_w = max(0, min(dst_w - dst_x, src_w - src_x));
        const u32 min_h = max(0, min(dst_h - dst_y, src_h - src_y));

        for (u32 i = 0; i < min_h; i++) {
            const u8* source = (src + (i + src_y) * src_stride + src_x * src_bytes);
            u32* destination = (u32*)(dst + (i + dst_y) * dst_stride + dst_x * dst_bytes);
            for (u32 ii = 0; ii < min_w; ii++) {
                const u8 index = source[ii];
                const RGBQUAD quad = src_palette[index];
                destination[ii] = (0xFF << 24)
                    | (quad.rgbRed << 16) | (quad.rgbGreen << 8) | quad.rgbBlue;
            }
        }
    }
    else {
        // TODO
    }
}

void blt_dst_color_key(u8* dst, u32 dst_x, u32 dst_y, u32 dst_w, u32 dst_h, u32 dst_bpp, u32 dst_stride,
    const u8* src, u32 src_x, u32 src_y, u32 src_w, u32 src_h, u32 src_bpp, u32 src_stride, u32 ckl, u32 ckh) {
    // TODO incomplete, proper implementation

    // TODO handle overlaps

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
                    const u8 target = destination[x];
                    if (ckl <= target && target <= ckh) {
                        destination[x] = source[x];
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

void blt_src_color_key(u8* dst, u32 dst_x, u32 dst_y, u32 dst_w, u32 dst_h, u32 dst_bpp, u32 dst_stride,
    const u8* src, u32 src_x, u32 src_y, u32 src_w, u32 src_h, u32 src_bpp, u32 src_stride, u32 ckl, u32 ckh) {
    // TODO incomplete, proper implementation

    // TODO handle overlaps

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
