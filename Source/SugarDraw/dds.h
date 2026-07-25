#pragma once

#include "connector.h"
#include "ddgc.h"
#include "ddsd.h"
#include "iddc.h"
#include "iddp.h"
#include "idds.h"
#include "intfc.h"

typedef struct dd dd;
typedef struct ddcc ddcc;
typedef struct ddg ddg;
typedef struct ddsfc ddsfc;

#define DDS_NONE                0x00000000
#define DDS_IMPLICIT            0x00000001
#define DDS_LOST                0x00000002
#define DDS_VALID               (DDS_IMPLICIT | DDS_LOST)

typedef struct overlayinfo {
    u32                 flags;
    RECT                src, dst;
    DDOVERLAYFX         effects;
    iddsconn            target;
} overlayinfo;

typedef struct dds {
    sugar*              manager;
    dd*                 instance;
    intfc*              interfaces;
    connector*          attachments;
    ddsfc*              chain;
    u32                 flags;
    ddg*                graphics;
    CRITICAL_SECTION    lock;
    DDSURFACEDESC2      desc;
    ddcc*               color;
    iddcconn            clipper;
    ddgc*               gamma;
    iddpconn            palette;
    ddsd*               surface;
    overlayinfo         overlay;
    connector*          overlays;
    DEVMODEA            mode;
} dds;

HRESULT dds_create(sugar* manager, u32 flags, dds** object);
void dds_release(dds* self, u32 flags);
HRESULT dds_get_interface(dds* self, const GUID* riid, void** object);

HRESULT dds_query_interface(dds* self, const GUID* riid, void** object);
HRESULT dds_add_ref(dds* self, idds* object);
HRESULT dds_remove_ref(dds* self, idds* object);

HRESULT dds_add_attached_surface(dds* self, iddsconn* surface);
HRESULT dds_add_overlay_dirty_rect(dds* self, RECT* rect);
HRESULT dds_blt(dds* self, RECT* dst, dds* surface, RECT* src, u32 flags, DDBLTFX* effects);
HRESULT dds_blt_batch(dds* self, DDBLTBATCH* batch, u32 count, u32 flags);
HRESULT dds_blt_fast(dds* self, u32 x, u32 y, dds* surface, RECT* rect, u32 transfer);
HRESULT dds_delete_attached_surface(dds* self, iddsconn* surface);
HRESULT dds_enum_attached_surfaces(dds* self); // TODO
HRESULT dds_enum_overlay_z_orders(dds* self); // TODO
HRESULT dds_flip(dds* self, dds* override, u32 flags);
HRESULT dds_get_attached_surface(dds* self, DDSCAPS2* caps, dds** surface);
HRESULT dds_get_blt_status(dds* self, u32 flags);
HRESULT dds_get_caps(dds* self, DDSCAPS2* caps);
HRESULT dds_get_clipper(dds* self, iddcconn* clipper);
HRESULT dds_get_color_key(dds* self, u32 flags, DDCOLORKEY* key);
HRESULT dds_get_dc(dds* self, HDC* hdc);
HRESULT dds_get_flip_status(dds* self, u32 flags);
HRESULT dds_get_overlay_position(dds* self, s32* x, s32* y);
HRESULT dds_get_palette(dds* self, iddpconn* palette);
HRESULT dds_get_pixel_format(dds* self, DDPIXELFORMAT* format);
HRESULT dds_get_surface_desc(dds* self, DDSURFACEDESC2* desc);
HRESULT dds_initialize(dds* self, dd* object, DDSURFACEDESC2* desc);
HRESULT dds_is_lost(dds* self);
HRESULT dds_lock(dds* self, RECT* rect, DDSURFACEDESC2* desc, u32 flags);
HRESULT dds_release_dc(dds* self, HDC hdc);
HRESULT dds_restore(dds* self);
HRESULT dds_set_clipper(dds* self, iddcconn* clipper);
HRESULT dds_set_color_key(dds* self, u32 flags, DDCOLORKEY* key);
HRESULT dds_set_overlay_position(dds* self, s32 x, s32 y);
HRESULT dds_set_palette(dds* self, iddpconn* palette);
HRESULT dds_unlock(dds* self, RECT* rect);
HRESULT dds_update_overlay(dds* self, const GUID* riid,
    RECT* src, iddsconn* surface, RECT* dst, u32 flags, DDOVERLAYFX* effects);
HRESULT dds_update_overlay_display(dds* self);
HRESULT dds_update_overlay_z_order(dds* self, u32 flags, iddsconn* surface);
HRESULT dds_get_dd_interface(dds* self, void** object);
HRESULT dds_page_lock(dds* self);
HRESULT dds_page_unlock(dds* self);
HRESULT dds_set_surface_desc(dds* self, DDSURFACEDESC2* desc);
HRESULT dds_set_private_data(dds* self, const GUID* tag, void* data, u32 size, u32 flags);
HRESULT dds_get_private_data(dds* self, const GUID* tag, void* buffer, u32* size);
HRESULT dds_free_private_data(dds* self, const GUID* tag);
HRESULT dds_get_uniqueness_value(dds* self, u32* value);
HRESULT dds_change_uniqueness_value(dds* self);
HRESULT dds_set_priority(dds* self, u32 priority);
HRESULT dds_get_priority(dds* self, u32* priority);
HRESULT dds_set_lod(dds* self, u32 lod);
HRESULT dds_get_lod(dds* self, u32* lod);

HRESULT dds_get_rect(dds* self, RECT* rect, RECT* result);

HRESULT dds_query_color_control(dds* self, const GUID* riid, void** object);
HRESULT dds_query_gamma_control(dds* self, const GUID* riid, void** object);

HRESULT dds_remove_color_control(dds* self);
HRESULT dds_remove_gamma_control(dds* self);
HRESULT dds_remove_palette(dds* self);

HRESULT dds_set_lost(dds* self);
HRESULT dds_set_palette_entries(dds* self, u32 start, u32 count, RGBQUAD* quads);

HRESULT dds_register_overlay(dds* self, iddsconn* overlay);
HRESULT dds_unregister_overlay(dds* self, iddsconn* overlay);
