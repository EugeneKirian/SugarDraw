#pragma once

#include "connector.h"
#include "iddp.h"
#include "idds.h"
#include "intfc.h"
#include "lock.h"

typedef struct dd dd;
typedef struct iddp iddp;
typedef struct idds idds;

typedef struct ddsbmp {
    BITMAPINFOHEADER    header;
    RGBQUAD             palette[PALETTE_MAX_ENTRY_COUNT];
} ddsbmp;

typedef struct ddsinfo {
    HDC                 dc;
    bool                exposed;
    s32                 checkpoint;
    HANDLE              mapping;
    ddsbmp*             info;
    HBITMAP             bitmap;
    u8*                 data;
    u32                 stride;
} ddsinfo;

typedef struct ddsoverlayinfo {
    u32                 flags;
    RECT                src, dst;
    DDOVERLAYFX         effects;
    iddsconn            target;
} ddsoverlayinfo;

typedef struct dds {
    sugar*              manager;
    dd*                 instance;
    intfc*              interfaces;
    arr*                attachments;
    CRITICAL_SECTION    lock;
    DDSURFACEDESC2      desc;
    iddpconn            palette;
    ddsinfo*            surface;
    void*               clipper;
    lock*               locks;
    ddsoverlayinfo      overlay;
    connector*          overlays;
    u32                 uniqueness;
} dds;

HRESULT dds_create(sugar* manager, dds** object);
void dds_release(dds* self, u32 flags);
HRESULT dds_get_interface(dds* self, const GUID* riid, void** object);

HRESULT dds_query_interface(dds* self, const GUID* riid, void** object);
HRESULT dds_add_ref(dds* self, idds* object);
HRESULT dds_remove_ref(dds* self, idds* object);

HRESULT dds_blt_fast(dds* self, u32 x, u32 y, dds* surface, RECT* rect, u32 transfer);

HRESULT dds_flip(dds* self, dds* override, u32 flags);
HRESULT dds_get_attached_surface(dds* self, DDSCAPS2* caps, dds** surface);

HRESULT dds_get_color_key(dds* self, u32 flags, DDCOLORKEY* key);
HRESULT dds_get_dc(dds* self, HDC* hdc);

HRESULT dds_get_palette(dds* self, iddpconn* palette);

HRESULT dds_get_surface_desc(dds* self, DDSURFACEDESC2* desc);
HRESULT dds_initialize(dds* self, dd* object, DDSURFACEDESC2* desc);
HRESULT dds_lock(dds* self, RECT* rect, DDSURFACEDESC2* desc, u32 flags);
HRESULT dds_release_dc(dds* self, HDC hdc);

HRESULT dds_set_color_key(dds* self, u32 flags, DDCOLORKEY* key);

HRESULT dds_set_palette(dds* self, iddpconn* palette);
HRESULT dds_unlock(dds* self, RECT* rect);
HRESULT dds_update_overlay(dds* self, const GUID* riid,
    RECT* src, iddsconn* surface, RECT* dst, u32 flags, DDOVERLAYFX* effects);

HRESULT dds_get_uniqueness_value(dds* self, u32* value);
HRESULT dds_change_uniqueness_value(dds* self);

HRESULT dds_get_rect(dds* self, RECT* rect, RECT* result);
HRESULT dds_get_surface_rect(dds* self, RECT* rect);

HRESULT dds_remove_palette(dds* self);
HRESULT dds_update_palette_entries(dds* self);

HRESULT dds_register_overlay(dds* self, iddsconn* overlay);
HRESULT dds_unregister_overlay(dds* self, iddsconn* overlay);
