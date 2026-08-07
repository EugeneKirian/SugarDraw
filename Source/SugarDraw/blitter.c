#include "blitter.h"
#include "plt.h"
#include "sampler.h"
#include "spanner.h"
#include "utilities.h"

// The best way to think of blitter is to think of it as an extensive
// rasterization step in a fixed function 3D rasterization pipeline, or,
// alternatively, as a collection of pixel shaders that work together
// to support various pixel formats.
//
// The blitter is a pipeline of it own, where the source surface (rectangle)
// can be though of as a texture, and the target surface (rectangle) is
// the space on the screen that has to be filled.
typedef struct blitter {
    allocator*          allocator;
    arena*              arena;
    CRITICAL_SECTION    lock;
} blitter;

static HRESULT blitter_pixel_copy(blt* submission, spanner* spanner, const RECT* dst, const RECT* src);
static HRESULT blitter_pixel_fill(blt* submission, spanner* spanner);
static HRESULT blitter_pixel_transform(blt* submission, arena* arena, spanner* spanner, sampler* sampler);

static HRESULT blitter_update_source_surface(blitter* self, blt* submission);

HRESULT blitter_create(allocator* allocator, blitter** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    blitter* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_BLITTER, sizeof(blitter), &instance))) {
        instance->allocator = allocator;
        if (SUCCEEDED(hr = arena_create(allocator, MEM_TAG_BLITTER, &instance->arena))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void blitter_release(blitter* self) {
    if (self != NULL) {
        DeleteCriticalSection(&self->lock);
        arena_release(self->arena);
        allocator_free(self->allocator, self);
    }
}

HRESULT blitter_submit(blitter* self, blt* submission) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (submission == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = arena_clear(self->arena))) {
        // In case the source and the destination surfaces are the same surface,
        // and there is an overlap between the destination and the source rectangles,
        // it is necessary to make a copy of the source data, so that it is available
        // as is throughout the blitting process.
        if (submission->images.destination.pixels == submission->images.source.pixels) {
            if (FAILED(hr = blitter_update_source_surface(self, submission))) {
                goto exit;
            }
        }

        MAKETYPE(RECT, rect);
        rect.right = submission->images.destination.width;
        rect.bottom = submission->images.destination.height;

        spanner* spanner = NULL;
        if (SUCCEEDED(hr = spanner_create(self->arena, &rect,
            &submission->rects.destination, submission->region, &spanner))) {
            // Color and depth fill only.
            if (submission->flags & BLITTER_FILL) {
                // TODO can color fill be mixed with color keying?
                EXITCODE(blitter_pixel_fill(submission, spanner));
            }

            // Same pixel format blitting with color keying, but w/o rotations, stretching, or shrinking.
            if (!(submission->flags & (BLITTER_ROTATION_ANGLE | BLITTER_EFFECTS))) {
                if (IsSameSizeRect(&submission->rects.destination, &submission->rects.source)) {
                    const DDPIXELFORMAT* dst = &submission->images.destination.format;
                    const DDPIXELFORMAT* src = &submission->images.source.format;
                    if (SUCCEEDED(hr = ddpixelformat_equal(dst, src))) {
                        EXITCODE(blitter_pixel_copy(submission, spanner,
                            &submission->rects.destination, &submission->rects.source));
                    }
                }
            }

            // General case with transformations, or with pixel format conversion.

            // TODO: The general case is extremely slow even for small pictures (i.e. 640 x 480)
            // Need a thread pool and multi-threading for it.

            f32 angle = 0.0f;
            if (submission->flags & BLITTER_ROTATION_ANGLE) {
                angle = (f32)submission->effects.dwRotationAngle / 100.0f;
            }
            else if (submission->flags & BLITTER_EFFECTS) {
                if (submission->effects.dwDDFX & DDBLTFX_ROTATE90) {
                    angle = 90.0f;
                }
                else if (submission->effects.dwDDFX & DDBLTFX_ROTATE180) {
                    angle = 180.0f;
                }
                else if (submission->effects.dwDDFX & DDBLTFX_ROTATE270) {
                    angle = 270.0f;
                }
            }

            sampler* sampler = NULL;
            if (SUCCEEDED(hr = sampler_create(self->arena,
                &submission->rects.destination, &submission->rects.source,
                angle, submission->effects.dwDDFX, &sampler))) {
                EXITCODE(blitter_pixel_transform(submission, self->arena, spanner, sampler));
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT blitter_update_source_surface(blitter* self, blt* submission) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (submission == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    MAKETYPE(RECT, intersect);
    if (IntersectRect(&intersect, &submission->rects.destination, &submission->rects.source)) {
        const s32 width = submission->rects.source.right - submission->rects.source.left;
        const s32 heigth = submission->rects.source.bottom - submission->rects.source.top;
        const u32 stride = submission->images.source.stride;
        const u32 size = (u32)(width * heigth * stride);

        u8* pixels = NULL;
        if (SUCCEEDED(hr = arena_allocate(self->arena, size, &pixels))) {
            const u32 bpp = (submission->images.source.format.dwRGBBitCount == 15)
                ? 16 : submission->images.source.format.dwRGBBitCount;
            const u32 start = submission->rects.source.top * stride
                + submission->rects.source.left * (bpp / 8);

            CopyMemory(pixels, submission->images.source.pixels + start, size);

            // Update the source image so that it only has the pixels
            // from the source rectangle, and update the source image dimensions.
            submission->images.source.pixels = pixels;
            submission->images.source.width = width;
            submission->images.source.height = heigth;

            // Update the source rectangle so that it matches the new image dimensions.
            submission->rects.source.left = 0;
            submission->rects.source.top = 0;
            submission->rects.source.right = width;
            submission->rects.source.bottom = heigth;
        }
    }

    return hr;
}

typedef void (PIXELCOPY)(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck);

static void pixel_copy_1byte_dst_src(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    for (u32 i = 0; i < count; i++) {
        const u8 dst_pixel = dst[i];
        const u8 src_pixel = src[i];
        if (dst_ck->dwColorSpaceLowValue <= dst_pixel && dst_pixel <= dst_ck->dwColorSpaceHighValue) {
            if (src_pixel < src_ck->dwColorSpaceLowValue || src_ck->dwColorSpaceHighValue < src_pixel) {
                dst[i] = src_pixel;
            }
        }
    }
}

static void pixel_copy_1byte_dst(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    for (u32 i = 0; i < count; i++) {
        const u8 dst_pixel = dst[i];
        if (dst_ck->dwColorSpaceLowValue <= dst_pixel && dst_pixel <= dst_ck->dwColorSpaceHighValue) {
            dst[i] = src[i];
        }
    }
}

static void pixel_copy_1byte_src(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    for (u32 i = 0; i < count; i++) {
        const u8 src_pixel = src[i];
        if (src_pixel < src_ck->dwColorSpaceLowValue || src_ck->dwColorSpaceHighValue < src_pixel) {
            dst[i] = src[i];
        }
    }
}

static void pixel_copy_1byte(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    CopyMemory(dst, src, count);
}

static void pixel_copy_2byte_dst_src(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    u16* dst_pixels = (u16*)dst;
    const u16* src_pixels = (u16*)src;
    
    for (u32 i = 0; i < count; i++) {
        const u16 dst_pixel = dst_pixels[i];
        const u16 src_pixel = src_pixels[i];
        if (dst_ck->dwColorSpaceLowValue <= dst_pixel && dst_pixel <= dst_ck->dwColorSpaceHighValue) {
            if (src_pixel < src_ck->dwColorSpaceLowValue || src_ck->dwColorSpaceHighValue < src_pixel) {
                dst_pixels[i] = src_pixel;
            }
        }
    }
}

static void pixel_copy_2byte_dst(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    u16* dst_pixels = (u16*)dst;
    const u16* src_pixels = (u16*)src;

    for (u32 i = 0; i < count; i++) {
        const u16 dst_pixel = dst_pixels[i];
        if (dst_ck->dwColorSpaceLowValue <= dst_pixel && dst_pixel <= dst_ck->dwColorSpaceHighValue) {
            dst_pixels[i] = src_pixels[i];
        }
    }
}

static void pixel_copy_2byte_src(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    u16* dst_pixels = (u16*)dst;
    const u16* src_pixels = (u16*)src;

    for (u32 i = 0; i < count; i++) {
        const u16 src_pixel = src_pixels[i];
        if (src_pixel < src_ck->dwColorSpaceLowValue || src_ck->dwColorSpaceHighValue < src_pixel) {
            dst_pixels[i] = src_pixel;
        }
    }
}

static void pixel_copy_2byte(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    CopyMemory(dst, src, 2 * count);
}

static void pixel_copy_3byte_dst_src(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    RGBTRIPLE* dst_pixels = (RGBTRIPLE*)dst;
    const RGBTRIPLE* src_pixels = (RGBTRIPLE*)src;

    for (u32 i = 0; i < count; i++) {
        const RGBTRIPLE dst_pixel = dst_pixels[i];
        const RGBTRIPLE src_pixel = src_pixels[i];
        const u32 dst_color = (dst_pixel.rgbtRed << 16) | (dst_pixel.rgbtGreen << 8) | dst_pixel.rgbtBlue;
        const u32 src_color = (src_pixel.rgbtRed << 16) | (src_pixel.rgbtGreen << 8) | src_pixel.rgbtBlue;
        if (dst_ck->dwColorSpaceLowValue <= dst_color && dst_color <= dst_ck->dwColorSpaceHighValue) {
            if (src_color < src_ck->dwColorSpaceLowValue || src_ck->dwColorSpaceHighValue < src_color) {
                dst_pixels[i] = src_pixels[i];
            }
        }
    }
}

static void pixel_copy_3byte_dst(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    RGBTRIPLE* dst_pixels = (RGBTRIPLE*)dst;
    const RGBTRIPLE* src_pixels = (RGBTRIPLE*)src;

    for (u32 i = 0; i < count; i++) {
        const RGBTRIPLE dst_pixel = dst_pixels[i];
        const u32 dst_color = (dst_pixel.rgbtRed << 16) | (dst_pixel.rgbtGreen << 8) | dst_pixel.rgbtBlue;
        if (dst_ck->dwColorSpaceLowValue <= dst_color && dst_color <= dst_ck->dwColorSpaceHighValue) {
            dst_pixels[i] = src_pixels[i];
        }
    }
}

static void pixel_copy_3byte_src(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    RGBTRIPLE* dst_pixels = (RGBTRIPLE*)dst;
    const RGBTRIPLE* src_pixels = (RGBTRIPLE*)src;

    for (u32 i = 0; i < count; i++) {
        const RGBTRIPLE src_pixel = src_pixels[i];
        const u32 src_color = (src_pixel.rgbtRed << 16) | (src_pixel.rgbtGreen << 8) | src_pixel.rgbtBlue;
        if (src_color < src_ck->dwColorSpaceLowValue || src_ck->dwColorSpaceHighValue < src_color) {
            dst_pixels[i] = src_pixels[i];
        }
    }
}

static void pixel_copy_3byte(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    CopyMemory(dst, src, 3 * count);
}

static void pixel_copy_4byte_dst_src(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    u32* dst_pixels = (u32*)dst;
    const u32* src_pixels = (u32*)src;

    for (u32 i = 0; i < count; i++) {
        const u32 dst_pixel = dst_pixels[i];
        const u32 src_pixel = src_pixels[i];
        if (dst_ck->dwColorSpaceLowValue <= dst_pixel && dst_pixel <= dst_ck->dwColorSpaceHighValue) {
            if (src_pixel < src_ck->dwColorSpaceLowValue || src_ck->dwColorSpaceHighValue < src_pixel) {
                dst_pixels[i] = src_pixel;
            }
        }
    }
}

static void pixel_copy_4byte_dst(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    u32* dst_pixels = (u32*)dst;
    const u32* src_pixels = (u32*)src;

    for (u32 i = 0; i < count; i++) {
        const u32 dst_pixel = dst_pixels[i];
        if (dst_ck->dwColorSpaceLowValue <= dst_pixel && dst_pixel <= dst_ck->dwColorSpaceHighValue) {
            dst_pixels[i] = src_pixels[i];
        }
    }
}

static void pixel_copy_4byte_src(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    u32* dst_pixels = (u32*)dst;
    const u32* src_pixels = (u32*)src;

    for (u32 i = 0; i < count; i++) {
        const u32 src_pixel = src_pixels[i];
        if (src_pixel < src_ck->dwColorSpaceLowValue || src_ck->dwColorSpaceHighValue < src_pixel) {
            dst_pixels[i] = src_pixel;
        }
    }
}

static void pixel_copy_4byte(u8* dst, const u8* src, u32 count, const DDCOLORKEY* dst_ck, const DDCOLORKEY* src_ck) {
    CopyMemory(dst, src, 4 * count);
}

HRESULT blitter_pixel_copy(blt* submission, spanner* spanner, const RECT* dst, const RECT* src) {
    if (submission == NULL || spanner == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    const u32 bits =
        submission->images.destination.format.dwRGBBitCount;

    PIXELCOPY* function = NULL;
    if (bits == 8) {
        function = pixel_copy_1byte;
        if ((submission->flags & BLITTER_SRC_COLOR_KEY)
            && (submission->flags & BLITTER_DEST_COLOR_KEY)) {
            function = pixel_copy_1byte_dst_src;
        }
        else if (submission->flags & BLITTER_DEST_COLOR_KEY) {
            function = pixel_copy_1byte_dst;
        }
        else if (submission->flags & BLITTER_SRC_COLOR_KEY) {
            function = pixel_copy_1byte_src;
        }
    }
    else if (bits == 15 || bits == 16) {
        function = pixel_copy_2byte;
        if ((submission->flags & BLITTER_SRC_COLOR_KEY)
            && (submission->flags & BLITTER_DEST_COLOR_KEY)) {
            function = pixel_copy_2byte_dst_src;
        }
        else if (submission->flags & BLITTER_DEST_COLOR_KEY) {
            function = pixel_copy_2byte_dst;
        }
        else if (submission->flags & BLITTER_SRC_COLOR_KEY) {
            function = pixel_copy_2byte_src;
        }
    }
    else if (bits == 24) {
        function = pixel_copy_3byte;
        if ((submission->flags & BLITTER_SRC_COLOR_KEY)
            && (submission->flags & BLITTER_DEST_COLOR_KEY)) {
            function = pixel_copy_3byte_dst_src;
        }
        else if (submission->flags & BLITTER_DEST_COLOR_KEY) {
            function = pixel_copy_3byte_dst;
        }
        else if (submission->flags & BLITTER_SRC_COLOR_KEY) {
            function = pixel_copy_3byte_src;
        }
    }
    else if (bits == 32) {
        function = pixel_copy_4byte;
        if ((submission->flags & BLITTER_SRC_COLOR_KEY)
            && (submission->flags & BLITTER_DEST_COLOR_KEY)) {
            function = pixel_copy_4byte_dst_src;
        }
        else if (submission->flags & BLITTER_DEST_COLOR_KEY) {
            function = pixel_copy_4byte_dst;
        }
        else if (submission->flags & BLITTER_SRC_COLOR_KEY) {
            function = pixel_copy_4byte_src;
        }
    }

    const u32 bytes = (bits == 15) ? 2 : (bits / 8);
    const u32 dst_stride = submission->images.destination.stride;
    const u32 src_stride = submission->images.source.stride;

    u8* dst_pixels = submission->images.destination.pixels;
    const u8* src_pixels = submission->images.source.pixels;

    const DDCOLORKEY* dst_ck = &submission->colors.destination;
    const DDCOLORKEY* src_ck = &submission->colors.source;

    const u32 count = spanner->count;
    for (u32 i = 0; i < count; i++) {
        const span* s = &spanner->spans[i];
        const u32 offset_x = s->x0 - dst->left;
        const u32 offset_y = s->y - dst->top;

        u8* dst_surface = dst_pixels + s->y * dst_stride + s->x0 * bytes;
        const u8* src_surface = src_pixels + (offset_y + src->top) * src_stride + (offset_x + src->left) * bytes;
        function(dst_surface, src_surface, s->x1 - s->x0, dst_ck, src_ck);
    }

    return DD_OK;
}

typedef void (PIXELFILL)(u8* surface, u32 count, u32 color);

static void pixel_fill_1byte(u8* surface, u32 count, u32 color) {
    const u8 pixel = (u8)color;
    for (u32 i = 0; i < count; i++) {
        surface[i] = pixel;
    }
}

static void pixel_fill_2byte(u8* surface, u32 count, u32 color) {
    u16* pixels = (u16*)surface;
    const u16 pixel = (u16)color;
    for (u32 i = 0; i < count; i++) {
        pixels[i] = pixel;
    }
}

static void pixel_fill_3byte(u8* surface, u32 count, u32 color) {
    RGBTRIPLE* pixels = (RGBTRIPLE*)surface;
    RGBTRIPLE pixel;
    pixel.rgbtRed = ((color >> 16) & 0xFF);
    pixel.rgbtGreen = ((color >> 8) & 0xFF);
    pixel.rgbtBlue = (color & 0xFF);

    for (u32 i = 0; i < count; i++) {
        pixels[i] = pixel;
    }
}

static void pixel_fill_4byte(u8* surface, u32 count, u32 color) {
    u32* pixels = (u32*)surface;
    for (u32 i = 0; i < count; i++) {
        pixels[i] = color;
    }
}

HRESULT blitter_pixel_fill(blt* submission, spanner* spanner) {
    if (submission == NULL || spanner == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    const u32 bits =
        submission->images.destination.format.dwRGBBitCount;

    PIXELFILL* function = NULL;
    switch (bits) {
    case 8: { function = pixel_fill_1byte; }break;
    case 15:
    case 16: { function = pixel_fill_2byte; }break;
    case 24: { function = pixel_fill_3byte; }break;
    case 32: { function = pixel_fill_4byte; }break;
    default: { return DDERR_UNSUPPORTEDFORMAT; }
    }

    const u32 color = submission->effects.dwFillColor;
    // TODO Z Buffer Depth Fill

    const u32 stride = submission->images.destination.stride;
    const u32 bytes = (bits == 15) ? 2 : (bits / 8);

    u8* pixels = submission->images.destination.pixels;

    const u32 count = spanner->count;
    for (u32 i = 0; i < count; i++) {
        const span* s = &spanner->spans[i];
        u8* surface = pixels + s->y * stride + s->x0 * bytes;
        function(surface, s->x1 - s->x0, color);
    }

    return DD_OK;
}

typedef bool(PIXELCHECKDST)(const u8* surface, u32 stride, u32 x, u32 y, const DDCOLORKEY* key);

static bool pixel_check_src_1byte(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const DDCOLORKEY* key) {
    const u32 u = (u32)(sampler->u0 + (f32)x * sampler->dudx + (f32)y * sampler->dudy);
    const u32 v = (u32)(sampler->v0 + (f32)x * sampler->dvdx + (f32)y * sampler->dvdy);

    const u8 color = *(surface + v * stride + u);
    return color < key->dwColorSpaceLowValue || key->dwColorSpaceHighValue < color;
}

static bool pixel_check_src_2byte(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const DDCOLORKEY* key) {
    const u32 u = (u32)(sampler->u0 + (f32)x * sampler->dudx + (f32)y * sampler->dudy);
    const u32 v = (u32)(sampler->v0 + (f32)x * sampler->dvdx + (f32)y * sampler->dvdy);

    const u16 color = *(surface + v * stride + u * 2);
    return color < key->dwColorSpaceLowValue || key->dwColorSpaceHighValue < color;
}

static bool pixel_check_src_3byte(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const DDCOLORKEY* key) {
    const u32 u = (u32)(sampler->u0 + (f32)x * sampler->dudx + (f32)y * sampler->dudy);
    const u32 v = (u32)(sampler->v0 + (f32)x * sampler->dvdx + (f32)y * sampler->dvdy);

    const RGBTRIPLE pixel = *(RGBTRIPLE*)(surface + v * stride + u * 3);
    const u32 color = (pixel.rgbtRed << 16) | (pixel.rgbtGreen << 8) | pixel.rgbtBlue;
    return color < key->dwColorSpaceLowValue || key->dwColorSpaceHighValue < color;
}

typedef bool(PIXELCHECKSRC)(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const DDCOLORKEY* key);

static bool pixel_check_src_4byte(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const DDCOLORKEY* key) {
    const u32 u = (u32)(sampler->u0 + (f32)x * sampler->dudx + (f32)y * sampler->dudy);
    const u32 v = (u32)(sampler->v0 + (f32)x * sampler->dvdx + (f32)y * sampler->dvdy);

    const u32 color = *(u32*)(surface + v * stride + u * 4);
    return color < key->dwColorSpaceLowValue || key->dwColorSpaceHighValue < color;
}

static bool pixel_check_dst_1byte(const u8* surface, u32 stride, u32 x, u32 y, const DDCOLORKEY* key) {
    const u8 color = *(surface + y * stride + x);
    return key->dwColorSpaceLowValue <= color && color <= key->dwColorSpaceHighValue;
}

static bool pixel_check_dst_2byte(const u8* surface, u32 stride, u32 x, u32 y, const DDCOLORKEY* key) {
    const u16 color = *(u16*)(surface + y * stride + x * 2);
    return key->dwColorSpaceLowValue <= color && color <= key->dwColorSpaceHighValue;
}

static bool pixel_check_dst_3byte(const u8* surface, u32 stride, u32 x, u32 y, const DDCOLORKEY* key) {
    const RGBTRIPLE pixel = *(RGBTRIPLE*)(surface + y * stride + x * 3);
    const u32 color = (pixel.rgbtRed << 16) | (pixel.rgbtGreen << 8) | pixel.rgbtBlue;
    return key->dwColorSpaceLowValue <= color && color <= key->dwColorSpaceHighValue;
}

static bool pixel_check_dst_4byte(const u8* surface, u32 stride, u32 x, u32 y, const DDCOLORKEY* key) {
    const u32 color = *(u32*)(surface + y * stride + x * 2);
    return key->dwColorSpaceLowValue <= color && color <= key->dwColorSpaceHighValue;
}

typedef RGBQUAD (PIXELREAD)(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const RGBQUAD* palette);

static RGBQUAD pixel_read_1byte(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const RGBQUAD* palette) {
    const u32 u = (u32)(sampler->u0 + (f32)x * sampler->dudx + (f32)y * sampler->dudy);
    const u32 v = (u32)(sampler->v0 + (f32)x * sampler->dvdx + (f32)y * sampler->dvdy);
    const u8 color = *(surface + v * stride + u);
    return palette[color];
}

static RGBQUAD pixel_read_2byte_555(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const RGBQUAD* palette) {
    const u32 u = (u32)(sampler->u0 + (f32)x * sampler->dudx + (f32)y * sampler->dudy);
    const u32 v = (u32)(sampler->v0 + (f32)x * sampler->dvdx + (f32)y * sampler->dvdy);
    const u16 src_color = *(u16*)(surface + v * stride + u * 2);

    const u8 r = (u8)(src_color >> 10) & 0x1F;
    const u8 g = (u8)(src_color >> 5) & 0x1F;
    const u8 b = (u8)(src_color & 0x1F);
    
    RGBQUAD dst_color;
    dst_color.rgbReserved = 0xFF;
    dst_color.rgbRed = (r << 3) | (r >> 2);
    dst_color.rgbGreen = (g << 3) | (g >> 2);
    dst_color.rgbBlue = (b << 3) | (b >> 2);
    return dst_color;
}

static RGBQUAD pixel_read_2byte_565(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const RGBQUAD* palette) {
    const u32 u = (u32)(sampler->u0 + (f32)x * sampler->dudx + (f32)y * sampler->dudy);
    const u32 v = (u32)(sampler->v0 + (f32)x * sampler->dvdx + (f32)y * sampler->dvdy);
    const u16 src_color = *(u16*)(surface + v * stride + u * 2);

    const u8 r = (u8)(src_color >> 11) & 0x1F;
    const u8 g = (u8)(src_color >> 5) & 0x3F;
    const u8 b = (u8)(src_color & 0x1F);

    RGBQUAD dst_color;
    dst_color.rgbReserved = 0xFF;
    dst_color.rgbRed = (r << 3) | (r >> 2);
    dst_color.rgbGreen = (g << 2) | (g >> 4);
    dst_color.rgbBlue = (b << 3) | (b >> 2);
    return dst_color;
}

static RGBQUAD pixel_read_3byte(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const RGBQUAD* palette) {
    const u32 u = (u32)(sampler->u0 + (f32)x * sampler->dudx + (f32)y * sampler->dudy);
    const u32 v = (u32)(sampler->v0 + (f32)x * sampler->dvdx + (f32)y * sampler->dvdy);
    const RGBTRIPLE src_color = *(RGBTRIPLE*)(surface + v * stride + u * 3);

    RGBQUAD dst_color;
    dst_color.rgbReserved = 0xFF;
    dst_color.rgbRed = src_color.rgbtRed;
    dst_color.rgbGreen = src_color.rgbtGreen;
    dst_color.rgbBlue = src_color.rgbtBlue;
    return dst_color;
}

static RGBQUAD pixel_read_4byte(const u8* surface, u32 stride, u32 x, u32 y, const sampler* sampler, const RGBQUAD* palette) {
    const u32 u = (u32)(sampler->u0 + (f32)x * sampler->dudx + (f32)y * sampler->dudy);
    const u32 v = (u32)(sampler->v0 + (f32)x * sampler->dvdx + (f32)y * sampler->dvdy);
    return *(RGBQUAD*)(surface + v * stride + u * 4);
}

typedef void (PIXELWRITE)(u8* surface, u32 stride, u32 x, u32 y, RGBQUAD color, const plt* lookup);

static void pixel_write_1byte(u8* surface, u32 stride, u32 x, u32 y, RGBQUAD color, const plt* lookup) {
    *(surface + y * stride + x) = lookup->colors[color.rgbRed >> 3][color.rgbGreen >> 3][color.rgbBlue >> 3];
}

static void pixel_write_2byte_555(u8* surface, u32 stride, u32 x, u32 y, RGBQUAD color, const plt* lookup) {
    const u32 r = (color.rgbRed >> 3) & 0x1F;
    const u32 g = (color.rgbGreen >> 3) & 0x1F;
    const u32 b = (color.rgbBlue >> 3) & 0x1F;
    const u16 dst_color = (r << 10) | (g << 5) | b;
    *(u16*)(surface + y * stride + x * 2) = 0x8000 | dst_color;
}

static void pixel_write_2byte_565(u8* surface, u32 stride, u32 x, u32 y, RGBQUAD color, const plt* lookup) {
    const u32 r = (color.rgbRed >> 3) & 0x1F;
    const u32 g = (color.rgbGreen >> 2) & 0x3F;
    const u32 b = (color.rgbBlue >> 3) & 0x1F;
    const u16 dst_color = (r << 11) | (g << 5) | b;
    *(u16*)(surface + y * stride + x * 2) = dst_color;
}

static void pixel_write_3byte(u8* surface, u32 stride, u32 x, u32 y, RGBQUAD color, const plt* lookup) {
    RGBTRIPLE dst_color;
    dst_color.rgbtRed = color.rgbRed;
    dst_color.rgbtGreen = color.rgbGreen;
    dst_color.rgbtBlue = color.rgbBlue;

    *(RGBTRIPLE*)(surface + y * stride + x * 3) = dst_color;
}

static void pixel_write_4byte(u8* surface, u32 stride, u32 x, u32 y, RGBQUAD color, const plt* lookup) {
    *(RGBQUAD*)(surface + y * stride + x * 4) = color;
}

typedef void (PIXELTRANSFORM)(u8* dst, u32 dst_stride, const u8* src, u32 src_stride,
    u32 x, u32 y, u32 count, const sampler* sampler, const RGBQUAD* palette, const plt* lookup,
    PIXELREAD* read_src, PIXELWRITE* write_dst, PIXELCHECKSRC* check_src, PIXELCHECKDST* check_dst,
    const DDCOLORKEY* src_key, const DDCOLORKEY* dst_key);

static void pixel_transform_dst_src(u8* dst, u32 dst_stride, const u8* src, u32 src_stride,
    u32 x, u32 y, u32 count, const sampler* sampler, const RGBQUAD* palette, const plt* lookup,
    PIXELREAD* read_src, PIXELWRITE* write_dst, PIXELCHECKSRC* check_src, PIXELCHECKDST* check_dst,
    const DDCOLORKEY* src_key, const DDCOLORKEY* dst_key) {
    for (u32 i = x; i < x + count; i++) {
        if (check_dst(dst, dst_stride, i, y, dst_key)
            && check_src(src, src_stride, i, y, sampler, src_key)) {
            write_dst(dst, dst_stride, i, y, read_src(src, src_stride, i, y, sampler, palette), lookup);
        }
    }
}

static void pixel_transform_dst(u8* dst, u32 dst_stride, const u8* src, u32 src_stride,
    u32 x, u32 y, u32 count, const sampler* sampler, const RGBQUAD* palette, const plt* lookup,
    PIXELREAD* read_src, PIXELWRITE* write_dst, PIXELCHECKSRC* check_src, PIXELCHECKDST* check_dst,
    const DDCOLORKEY* src_key, const DDCOLORKEY* dst_key) {
    for (u32 i = x; i < x + count; i++) {
        if (check_dst(dst, dst_stride, i, y, dst_key)) {
            write_dst(dst, dst_stride, i, y, read_src(src, src_stride, i, y, sampler, palette), lookup);
        }
    }
}

static void pixel_transform_src(u8* dst, u32 dst_stride, const u8* src, u32 src_stride,
    u32 x, u32 y, u32 count, const sampler* sampler, const RGBQUAD* palette, const plt* lookup,
    PIXELREAD* read_src, PIXELWRITE* write_dst, PIXELCHECKSRC* check_src, PIXELCHECKDST* check_dst,
    const DDCOLORKEY* src_key, const DDCOLORKEY* dst_key) {
    for (u32 i = x; i < x + count; i++) {
        if (check_src(src, src_stride, i, y, sampler, src_key)) {
            write_dst(dst, dst_stride, i, y, read_src(src, src_stride, i, y, sampler, palette), lookup);
        }
    }
}

static void pixel_transform(u8* dst, u32 dst_stride, const u8* src, u32 src_stride,
    u32 x, u32 y, u32 count, const sampler* sampler, const RGBQUAD* palette, const plt* lookup,
    PIXELREAD* read_src, PIXELWRITE* write_dst, PIXELCHECKSRC* check_src, PIXELCHECKDST* check_dst,
    const DDCOLORKEY* src_key, const DDCOLORKEY* dst_key) {
    for (u32 i = x; i < x + count; i++) {
        write_dst(dst, dst_stride, i, y, read_src(src, src_stride, i, y, sampler, palette), lookup);
    }
}

static HRESULT blitter_pixel_transform(blt* submission, arena* arena, spanner* spanner, sampler* sampler) {
    if (arena == NULL || submission == NULL || spanner == NULL || sampler == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    plt* lookup = NULL;

    void* read_src_function = NULL;
    void* write_dst_function = NULL;
    void* check_src_color_key = NULL;
    void* check_dst_color_key = NULL;

    const u32 src_bits =
        submission->images.source.format.dwRGBBitCount;
    switch (src_bits) {
    case 8: {
        read_src_function = pixel_read_1byte;
        check_src_color_key = pixel_check_src_1byte;
    }break;
    case 15: {
        if (submission->images.source.format.dwRBitMask == 0x00007C00
            && submission->images.source.format.dwGBitMask == 0x000003E0
            && submission->images.source.format.dwBBitMask == 0x0000001F) {
            read_src_function = pixel_read_2byte_555;
        }

        check_src_color_key = pixel_check_src_2byte;
    }break;
    case 16: {
        if (submission->images.source.format.dwRBitMask == 0x00007C00
            && submission->images.source.format.dwGBitMask == 0x000003E0
            && submission->images.source.format.dwBBitMask == 0x0000001F) {
            read_src_function = pixel_read_2byte_555;
        }
        else if (submission->images.source.format.dwRBitMask == 0x0000F800
            && submission->images.source.format.dwGBitMask == 0x000007E0
            && submission->images.source.format.dwBBitMask == 0x0000001F) {
            read_src_function = pixel_read_2byte_565;
        }

        check_src_color_key = pixel_check_src_2byte;
    }break;
    case 24: {
        read_src_function = pixel_read_3byte;
        check_src_color_key = pixel_check_src_3byte;
    }break;
    case 32: {
        read_src_function = pixel_read_4byte;
        check_src_color_key = pixel_check_src_4byte;
    }break;
    default: { return DDERR_UNSUPPORTEDFORMAT; }
    }

    u32 dst_bytes = 0;
    const u32 dst_bits =
        submission->images.destination.format.dwRGBBitCount;
    switch (dst_bits) {
    case 8: {
        write_dst_function = pixel_write_1byte;
        check_dst_color_key = pixel_check_dst_1byte;

        if (FAILED(hr = plt_create(arena, submission->images.destination.palette.count,
            submission->images.destination.palette.palette, &lookup))) {
            return hr;
        }
    }break;
    case 15:
    case 16: {
        if (submission->images.source.format.dwRBitMask == 0x00007C00
            && submission->images.source.format.dwGBitMask == 0x000003E0
            && submission->images.source.format.dwBBitMask == 0x0000001F) {
            write_dst_function = pixel_write_2byte_555;
        }
        else if (submission->images.source.format.dwRBitMask == 0x0000F800
            && submission->images.source.format.dwGBitMask == 0x000007E0
            && submission->images.source.format.dwBBitMask == 0x0000001F) {
            write_dst_function = pixel_write_2byte_565;
        }

        check_dst_color_key = pixel_check_dst_2byte;
    }break;
    case 24: {
        write_dst_function = pixel_write_3byte;
        check_dst_color_key = pixel_check_dst_3byte;
    }break;
    case 32: {
        write_dst_function = pixel_write_4byte;
        check_dst_color_key = pixel_check_dst_4byte;
    }break;
    default: { return DDERR_UNSUPPORTEDFORMAT; }
    }

    const u32 dst_stride = submission->images.destination.stride;
    const u32 src_stride = submission->images.source.stride;

    u8* dst_pixels = submission->images.destination.pixels;
    const u8* src_pixels = submission->images.source.pixels;

    const DDCOLORKEY* dst_ck = &submission->colors.destination;
    const DDCOLORKEY* src_ck = &submission->colors.source;

    PIXELTRANSFORM* transform = pixel_transform;
    if ((submission->flags & BLITTER_DEST_COLOR_KEY)
        && (submission->flags & BLITTER_SRC_COLOR_KEY)) {
        transform = pixel_transform_dst_src;
    }
    else if (submission->flags & BLITTER_DEST_COLOR_KEY) {
        transform = pixel_transform_dst;
    }
    else if (submission->flags & BLITTER_SRC_COLOR_KEY) {
        transform = pixel_transform_src;
    }

    // TODO: it seems like general case doesn't work great with color keying
    // ddex5.exe ddoverlay.exe wormhole.exe

    const u32 count = spanner->count;
    for (u32 i = 0; i < count; i++) {
        const span* s = &spanner->spans[i];
        transform(dst_pixels, dst_stride, src_pixels, src_stride,
            s->x0, s->y, s->x1 - s->x0, sampler, submission->images.source.palette.palette, lookup,
            read_src_function, write_dst_function, check_src_color_key, check_dst_color_key, src_ck, dst_ck);
    }

    return hr;
}
