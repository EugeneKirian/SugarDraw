#pragma once

#include "intfc.h"
#include "sugar.h"

typedef struct dd dd;
typedef struct dds dds;
typedef struct iddp iddp;

typedef struct ddp {
    sugar*              manager;
    dd*                 instance;
    intfc*              interfaces;
    arr*                surfaces;
    CRITICAL_SECTION    lock;
    u32                 caps;
    u32                 uniqueness;
    RGBQUAD             quads[PALETTE_MAX_ENTRY_COUNT];
    PALETTEENTRY        entries[PALETTE_MAX_ENTRY_COUNT];
} ddp;

HRESULT ddp_create(sugar* manager, ddp** object);
void ddp_release(ddp* self, u32 flags);
HRESULT ddp_get_interface(ddp* self, const GUID* riid, void** object);

HRESULT ddp_query_interface(ddp* self, const GUID* riid, void** object);
HRESULT ddp_add_ref(ddp* self, iddp* object);
HRESULT ddp_remove_ref(ddp* self, iddp* object);

HRESULT ddp_get_caps(ddp* self, u32* caps);
HRESULT ddp_get_entries(ddp* self, u32 flags, u32 base, u32 count, PALETTEENTRY* entries);
HRESULT ddp_initialize(ddp* self, dd* object, u32 flags);
HRESULT ddp_set_entries(ddp* self, u32 flags, u32 start, u32 count, PALETTEENTRY* entries);

HRESULT ddp_register_surface(ddp* self, dds* surface);
HRESULT ddp_unregister_surface(ddp* self, dds* surface);
