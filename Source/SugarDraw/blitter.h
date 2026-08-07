#pragma once

#include "arena.h"

#define BLITTER_NONE                0x00000000
#define BLITTER_FILL                0x00000001
#define BLITTER_EFFECTS             0x00000002
#define BLITTER_SRC_COLOR_KEY       0x00000004
#define BLITTER_DEST_COLOR_KEY      0x00000008
#define BLITTER_ROTATION_ANGLE      0x00000010

typedef struct bltimg {
    u8*                 pixels;
    s32                 width, height;
    u32                 stride;
    DDPIXELFORMAT       format;
    struct {
        u32             count;
        RGBQUAD*        palette;
    } palette;
} bltimg;

typedef struct blt {
    u32                 flags;
    struct {
        bltimg          source, destination;
    } images;
    struct {
        RECT            source, destination;
    } rects;
    struct {
        DDCOLORKEY      source, destination;
    } colors;
    DDBLTFX             effects;
    RGNDATA*            region;
} blt;

typedef struct blitter blitter;

HRESULT blitter_create(allocator* allocator, blitter** object);
void blitter_release(blitter* self);

HRESULT blitter_submit(blitter* self, blt* submission);
