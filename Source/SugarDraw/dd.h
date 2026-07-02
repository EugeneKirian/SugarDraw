#pragma once
#include "intfc.h"
#include "sugar.h"

typedef struct idd idd;

typedef struct dd {
    sugar*              manager;
    bool                initialized; // TODO initialize rendering device
    GUID                id;
    intfc*              interfaces;
    CRITICAL_SECTION    lock;
} dd;

HRESULT dd_create(sugar* manager, const GUID* rclsid, dd** object);
void dd_release(dd* self);

HRESULT dd_query_interface(dd* self, const GUID* riid, void** object);
HRESULT dd_add_ref(dd* self, idd* object);
HRESULT dd_remove_ref(dd* self, idd* object);

HRESULT dd_initialize(dd* self, const GUID* riid);
