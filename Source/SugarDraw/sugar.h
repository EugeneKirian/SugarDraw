#pragma once

#include "arr.h"
#include "logger.h"

typedef struct dd dd;

typedef struct sugar {
    allocator*          allocator;
    logger*             logger;
    arr*                items;
    CRITICAL_SECTION    lock;
    struct {
        DEVMODEA        initial, current;
    } display_modes;
} sugar;

HRESULT sugar_create(allocator* allocator, logger* logger, sugar** object);
void sugar_release(sugar* self);

HRESULT sugar_create_direct_draw(sugar* self, const GUID* rclsid, const GUID* riid, void** object);
HRESULT sugar_remove_direct_draw(sugar* self, dd* object);

HRESULT sugar_get_display_mode(sugar* self, DEVMODEA* mode);
HRESULT sugar_set_display_mode(sugar* self, u32 width, u32 height, u32 bpp, u32 rate);
