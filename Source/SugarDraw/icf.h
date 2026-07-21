#pragma once

#include "sugar.h"

typedef struct cf cf;

typedef struct icf {
    const void* self;
    sugar*      manager;
    logger*     logger;
    GUID        id;
    s32         refs;
    cf*         instance;
} icf;

typedef HRESULT(SUGARCALL* ICFQUERYINTERFACE)(icf*, REFIID, LPVOID*);
typedef ULONG(SUGARCALL*   ICFADDREF)(icf*);
typedef ULONG(SUGARCALL*   ICFRELEASE)(icf*);

typedef HRESULT(SUGARCALL* ICFCREATEINSTANCE)(icf*, LPUNKNOWN, REFIID, LPVOID*);
typedef HRESULT(SUGARCALL* ICFLOCKSERVER)(icf*, BOOL);

HRESULT SUGARCALL icf_create(sugar* manager, const GUID* riid, icf** object);
void SUGARCALL icf_release(icf* self);

HRESULT SUGARCALL icf_query_interface(icf* self, const GUID* riid, void** object);
ULONG SUGARCALL icf_add_ref(icf* self);
ULONG SUGARCALL icf_remove_ref(icf* self);
