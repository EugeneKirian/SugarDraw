#pragma once

#include "intfc.h"
#include "sugar.h"

typedef struct iddf iddf;

typedef struct ddf {
    sugar*              manager;
    GUID                id;
    intfc*              interfaces;
    CRITICAL_SECTION    lock;
} ddf;

HRESULT ddf_create(sugar* manager, const GUID* rclsid, ddf** object);
void ddf_release(ddf* self, u32 flags);
HRESULT ddf_get_interface(ddf* self, const GUID* riid, void** object);

HRESULT ddf_query_interface(ddf* self, const GUID* riid, void** object);
HRESULT ddf_add_ref(ddf* self, iddf* object);
HRESULT ddf_remove_ref(ddf* self, iddf* object);

HRESULT ddf_create_direct_draw(ddf* self, const GUID* riid, const GUID* device, HWND hwnd, u32 flags, void** object);
