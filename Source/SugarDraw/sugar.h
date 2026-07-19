#pragma once

#include "arr.h"
#include "converter.h"
#include "driver.h"

typedef struct dd dd;
typedef struct ddc ddc;
typedef struct dds dds;

typedef struct sugar {
    allocator*          allocator;
    logger*             logger;
    driver*             driver;
    arr*                clippers;
    arr*                items;
    CRITICAL_SECTION    lock;
    struct {
        DEVMODEA        initial, current;
        u32             count;
        DEVMODEA*       modes;
    } modes;
    dd*                 exclusive;
} sugar;

HRESULT sugar_create(allocator* allocator, logger* logger, driver* driver, sugar** object);
void sugar_release(sugar* self);

HRESULT sugar_set_driver(sugar* self, driver* driver);

HRESULT sugar_create_dd(sugar* self, const GUID* rclsid, const GUID* riid, void** object);
HRESULT sugar_remove_dd(sugar* self, dd* object);

HRESULT sugar_create_ddc(sugar* self, const GUID* rclsid, const GUID* riid, void** object);
HRESULT sugar_remove_ddc(sugar* self, ddc* object);

HRESULT sugar_get_exclusive(sugar* self, dd** object);
HRESULT sugar_set_exclusive(sugar* self, dd* object);

HRESULT sugar_get_display_mode(sugar* self, DEVMODEA* mode);
HRESULT sugar_set_display_mode(sugar* self, u32 width, u32 height, u32 bpp, u32 rate);
HRESULT sugar_supports_display_mode(sugar* self, const DEVMODEA* mode);
HRESULT sugar_restore_display_mode(sugar* self);

