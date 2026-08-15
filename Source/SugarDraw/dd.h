#pragma once

#include "blitter.h"
#include "intfc.h"
#include "sugar.h"

typedef struct ddg ddg;
typedef struct ddp ddp;
typedef struct dds dds;
typedef struct idd idd;

typedef struct dd {
    sugar*              manager;
    GUID                id;
    intfc*              interfaces;
    CRITICAL_SECTION    lock;
    blitter*            blitter;
    ddg*                graphics;
    
    struct {
        DWORD           flags;
        HWND            hwnd;
        DEVMODEA        mode;
        RECT            rect;
    } cooperation;

    dds*                primary;
    arr*                clippers;
    arr*                palettes;
    arr*                surfaces;
} dd;

typedef struct ddedmc {
    LPVOID callback, context;
} ddedmc;

typedef HRESULT(SUGARCALL* ENUMDISPLAYMODESCALLBACK)(ddedmc* context, DDSURFACEDESC2* desc);

HRESULT dd_create(sugar* manager, const GUID* rclsid, dd** object);
void dd_release(dd* self, u32 flags);
HRESULT dd_get_interface(dd* self, const GUID* riid, void** object);

HRESULT dd_query_interface(dd* self, const GUID* riid, void** object);
HRESULT dd_add_ref(dd* self, idd* object);
HRESULT dd_remove_ref(dd* self, idd* object);

HRESULT dd_compact(dd* self);
HRESULT dd_create_clipper(dd* self, void** object);
HRESULT dd_create_palette(dd* self, u32 flags, PALETTEENTRY* entries, void** object);
HRESULT dd_create_surface(dd* self, const GUID* riid, DDSURFACEDESC2* desc, void** object);
HRESULT dd_duplicate_surface(dd* self, dds* surface, const GUID* riid, void** object);
HRESULT dd_enum_display_modes(dd* self, u32 flags, DDSURFACEDESC2* desc, ENUMDISPLAYMODESCALLBACK callback, ddedmc* context);
HRESULT dd_enum_surfaces(dd* self); // TODO
HRESULT dd_flip_to_gdi_surface(dd* self);
HRESULT dd_get_caps(dd* self, DDCAPS* caps);
HRESULT dd_get_display_mode(dd* self, DDSURFACEDESC2* desc);
HRESULT dd_get_fourcc_codes(dd* self, u32* count, u32* codes);
HRESULT dd_get_gdi_surface(dd* self, const GUID* riid, void** object);
HRESULT dd_get_monitor_frequency(dd* self, u32* frequency);
HRESULT dd_get_scan_line(dd* self, u32* line);
HRESULT dd_get_vertical_blank_status(dd* self, bool* status);
HRESULT dd_initialize(dd* self, const GUID* device);
HRESULT dd_restore_display_mode(dd* self);
HRESULT dd_set_cooperative_level(dd* self, HWND hwnd, u32 flags);
HRESULT dd_set_display_mode(dd* self, u32 width, u32 height, u32 bpp, u32 rate, u32 flags);
HRESULT dd_wait_for_vertical_blank(dd* self, u32 flags, HANDLE event);
HRESULT dd_get_available_vid_mem(dd* self, DDSCAPS2* caps, u32* total, u32* free);
HRESULT dd_get_surface_from_dc(dd* self, HDC hdc, const GUID* riid, void** object);
HRESULT dd_restore_all_surfaces(dd* self);
HRESULT dd_test_cooperative_level(dd* self);
HRESULT dd_get_device_identifier(dd* self, DDDEVICEIDENTIFIER2* identifier);
HRESULT dd_start_mode_test(dd* self, SIZE* modes, u32 count, u32 flags);
HRESULT dd_evaluate_mode(dd* self, u32 flags, u32* timeout);

HRESULT dd_attach_clipper(dd* self, ddc* clipper);
HRESULT dd_remove_clipper(dd* self, ddc* clipper);

HRESULT dd_remove_palette(dd* self, ddp* palette);
HRESULT dd_remove_surface(dd* self, dds* surface);

HRESULT dd_lose_all_surfaces(dd* self);
