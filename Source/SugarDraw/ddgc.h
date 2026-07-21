#pragma once

#include "intfc.h"
#include "sugar.h"

typedef struct dds dds;
typedef struct iddgc iddgc;

typedef struct ddgc {
    sugar*              manager;
    dds*                instance;
    intfc*              interfaces;
    CRITICAL_SECTION    lock;
    DDGAMMARAMP         control;
} ddgc;

HRESULT ddgc_create(sugar* manager, ddgc** object);
void ddgc_release(ddgc* self, u32 flags);
HRESULT ddgc_get_interface(ddgc* self, const GUID* riid, void** object);

HRESULT ddgc_query_interface(ddgc* self, const GUID* riid, void** object);
HRESULT ddgc_add_ref(ddgc* self, iddgc* object);
HRESULT ddgc_remove_ref(ddgc* self, iddgc* object);

HRESULT ddgc_get_gamma_ramp(ddgc* self, DDGAMMARAMP* control);
HRESULT ddgc_set_gamma_ramp(ddgc* self, DDGAMMARAMP* control);

HRESULT ddgc_initialize(ddgc* self, dds* instance);
