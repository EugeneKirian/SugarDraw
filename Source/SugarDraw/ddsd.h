#pragma once

#include "blitter.h"

typedef struct ddsd ddsd;

HRESULT ddsd_create(allocator* allocator, blitter* blitter, ddsd** object);
void ddsd_release(ddsd* self);

ULONG ddsd_add_ref(ddsd* self);
ULONG ddsd_remove_ref(ddsd* self);

HRESULT ddsd_initialize(ddsd* self, DDSURFACEDESC2* desc);

HRESULT ddsd_blt(ddsd* self, RECT* dst, ddsd* surface, RECT* src, RGNDATA* region, u32 flags, DDBLTFX* effects);

HRESULT ddsd_blt_fast(ddsd* self, RECT* dst, ddsd* surface, RECT* src, u32 transfer);

HRESULT ddsd_get_palette(ddsd* self, u32 start, u32 count, RGBQUAD* quads);
HRESULT ddsd_set_palette(ddsd* self, u32 start, u32 count, RGBQUAD* quads);

HRESULT ddsd_get_dc(ddsd* self, HDC* hdc);
HRESULT ddsd_release_dc(ddsd* self, HDC hdc);

HRESULT ddsd_lock(ddsd* self, RECT* rect, DDSURFACEDESC2* desc);
HRESULT ddsd_unlock(ddsd* self, RECT* rect);

HRESULT ddsd_page_lock(ddsd* self);
HRESULT ddsd_page_unlock(ddsd* self);

HRESULT ddsd_set_surface_desc(ddsd* self, DDSURFACEDESC2* desc);

// TODO Private data?

HRESULT ddsd_get_uniqueness_value(ddsd* self, u32* value);
HRESULT ddsd_change_uniqueness_value(ddsd* self);

HRESULT ddsd_get_lock_count(ddsd* self, u32* count);
HRESULT ddsd_get_rect(ddsd* self, RECT* rect);
HRESULT ddsd_inside_rect(ddsd* self, RECT* rect);
HRESULT ddsd_lock_rect(ddsd* self, RECT* rect);
HRESULT ddsd_unlock_rect(ddsd* self, RECT* rect);

HRESULT ddsd_restore_surface(ddsd* self);
