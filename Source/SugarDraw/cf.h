#pragma once

#include "intfc.h"
#include "sugar.h"

typedef struct icf icf;

typedef struct cf {
    sugar*              manager;
    GUID                id;
    intfc*              interfaces;
    CRITICAL_SECTION    lock;
} cf;

HRESULT cf_create(sugar* manager, const GUID* rclsid, cf** object);
void cf_release(cf* self, u32 flags);
HRESULT cf_get_interface(cf* self, const GUID* riid, void** object);

HRESULT cf_query_interface(cf* self, const GUID* riid, void** object);
HRESULT cf_add_ref(cf* self, icf* object);
HRESULT cf_remove_ref(cf* self, icf* object);

HRESULT cf_create_instance(cf* self, const GUID* riid, void** object);
