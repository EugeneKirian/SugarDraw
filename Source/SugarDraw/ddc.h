#pragma once

#include "intfc.h"
#include "sugar.h"

typedef struct dd dd;
typedef struct iddc iddc;

typedef struct ddc {
    sugar*              manager;
    GUID                id;
    dd*                 instance;
    intfc*              interfaces;
    CRITICAL_SECTION    lock;
    HWND                hwnd;
    RGNDATA*            region;
} ddc;

HRESULT ddc_create(sugar* manager, const GUID* rclsid, ddc** object);
void ddc_release(ddc* self, u32 flags);
HRESULT ddc_get_interface(ddc* self, const GUID* riid, void** object);

HRESULT ddc_query_interface(ddc* self, const GUID* riid, void** object);
HRESULT ddc_add_ref(ddc* self, iddc* object);
HRESULT ddc_remove_ref(ddc* self, iddc* object);

HRESULT ddc_get_clip_list(ddc* self, RECT* rect, RGNDATA* region, u32* size);
HRESULT ddc_get_hwnd(ddc* self, HWND* hwnd);
HRESULT ddc_initialize(ddc* self, dd* object);
HRESULT ddc_is_clip_list_changed(ddc* self, bool* changed);
HRESULT ddc_set_clip_list(ddc* self, RGNDATA* region);
HRESULT ddc_set_hwnd(ddc* self, HWND hwnd);
