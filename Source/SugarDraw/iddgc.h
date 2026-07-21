#pragma once

#include "sugar.h"

typedef struct ddgc ddgc;

typedef struct iddgc {
    const void* self;
    sugar*      manager;
    logger*     logger;
    GUID        id;
    s32         refs;
    ddgc*       instance;
} iddgc;

typedef HRESULT(SUGARCALL* IDDGCQUERYINTERFACE)(iddgc*, REFIID, LPVOID*);
typedef ULONG(SUGARCALL* IDDGCADDREF)(iddgc*);
typedef ULONG(SUGARCALL* IDDGCRELEASE)(iddgc*);

typedef HRESULT(SUGARCALL* IDDGCGETGAMMARAMP)(iddgc*, DWORD, LPDDGAMMARAMP);
typedef HRESULT(SUGARCALL* IDDGCSETGAMMARAMP)(iddgc*, DWORD, LPDDGAMMARAMP);

HRESULT SUGARCALL iddgc_create(sugar* manager, const GUID* riid, iddgc** object);
void SUGARCALL iddgc_release(iddgc* self);

HRESULT SUGARCALL iddgc_query_interface(iddgc* self, const GUID* riid, void** object);
ULONG SUGARCALL iddgc_add_ref(iddgc* self);
ULONG SUGARCALL iddgc_remove_ref(iddgc* self);
