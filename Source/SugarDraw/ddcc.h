#pragma once

#include "intfc.h"
#include "sugar.h"

typedef struct dds dds;
typedef struct iddcc iddcc;

typedef struct ddcc {
    sugar*              manager;
    dds*                instance;
    intfc*              interfaces;
    CRITICAL_SECTION    lock;
    DDCOLORCONTROL      control;
} ddcc;

HRESULT ddcc_create(sugar* manager, ddcc** object);
void ddcc_release(ddcc* self, u32 flags);
HRESULT ddcc_get_interface(ddcc* self, const GUID* riid, void** object);

HRESULT ddcc_query_interface(ddcc* self, const GUID* riid, void** object);
HRESULT ddcc_add_ref(ddcc* self, iddcc* object);
HRESULT ddcc_remove_ref(ddcc* self, iddcc* object);

HRESULT ddcc_get_color_controls(ddcc* self, DDCOLORCONTROL* control);
HRESULT ddcc_set_color_controls(ddcc* self, DDCOLORCONTROL* control);

HRESULT ddcc_initialize(ddcc* self, dds* instance);
