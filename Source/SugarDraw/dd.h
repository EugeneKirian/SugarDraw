#pragma once
#include "intfc.h"
#include "sugar.h"

typedef struct idd idd;
typedef struct ddp ddp;
typedef struct dds dds;

typedef struct dd {
    sugar*              manager;
    bool                initialized; // TODO initialize rendering device
    GUID                id;
    intfc*              interfaces;
    CRITICAL_SECTION    lock;
    
    struct {
        HWND            hwnd;
        DWORD           flags;
    } cooperative_level;

    dds*                primary;
    arr*                surfaces;
    arr*                palettes;
} dd;

HRESULT dd_create(sugar* manager, const GUID* rclsid, dd** object);
void dd_release(dd* self, u32 flags);

HRESULT dd_query_interface(dd* self, const GUID* riid, void** object);
HRESULT dd_add_ref(dd* self, idd* object);
HRESULT dd_remove_ref(dd* self, idd* object);

HRESULT dd_create_surface(dd* self, const GUID* riid, DDSURFACEDESC2* desc, void** object);
HRESULT dd_remove_surface(dd* self, dds* object);
// TODO Clipper
HRESULT dd_create_palette(dd* self, u32 flags, PALETTEENTRY* entries, void** object);
HRESULT dd_remove_palette(dd* self, ddp* object);

HRESULT dd_initialize(dd* self, const GUID* riid);
HRESULT dd_restore_display_mode(dd* self);
HRESULT dd_set_cooperative_level(dd* self, HWND hwnd, u32 flags);

HRESULT dd_set_display_mode(dd* self, u32 width, u32 height, u32 bpp, u32 rate, u32 flags);
