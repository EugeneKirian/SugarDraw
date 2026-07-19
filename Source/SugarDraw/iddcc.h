#pragma once

#include "sugar.h"

typedef struct ddcc ddcc;

typedef struct iddcc {
    const void* self;
    sugar*      manager;
    logger*     logger;
    GUID        id;
    s32         refs;
    ddcc*       instance;
} iddcc;

typedef HRESULT(SUGARCALL* IDDCCQUERYINTERFACE)(iddcc*, REFIID, LPVOID*);
typedef ULONG(SUGARCALL* IDDCCADDREF)(iddcc*);
typedef ULONG(SUGARCALL* IDDCCRELEASE)(iddcc*);

typedef HRESULT(SUGARCALL* IDDCCGETCOLORCONTROLS)(iddcc*, LPDDCOLORCONTROL);
typedef HRESULT(SUGARCALL* IDDCCSETCOLORCONTROLS)(iddcc*, LPDDCOLORCONTROL);

HRESULT SUGARCALL iddcc_create(sugar* manager, const GUID* riid, iddcc** object);
void SUGARCALL iddcc_release(iddcc* self);

HRESULT SUGARCALL iddcc_query_interface(iddcc* self, const GUID* riid, void** object);
ULONG SUGARCALL iddcc_add_ref(iddcc* self);
ULONG SUGARCALL iddcc_remove_ref(iddcc* self);
