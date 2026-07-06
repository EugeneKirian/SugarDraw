#pragma once
#include "sugar.h"

typedef struct ddp ddp;

typedef struct iddp {
    const void* self;
    sugar*      manager;
    GUID        id;
    s32         refs;
    ddp*        instance;
} iddp;

typedef HRESULT(SUGARCALL* IDDPQUERYINTERFACE)(iddp*, REFIID, LPVOID*);
typedef ULONG(SUGARCALL* IDDPADDREF)(iddp*);
typedef ULONG(SUGARCALL* IDDPRELEASE) (iddp*);
typedef HRESULT(SUGARCALL* IDDPGETCAPS)(iddp*, LPDWORD);
typedef HRESULT(SUGARCALL* IDDPGETENTRIES)(iddp*, DWORD, DWORD, DWORD, LPPALETTEENTRY);
typedef HRESULT(SUGARCALL* IDDPINITIALIZE)(iddp*, LPDIRECTDRAW, DWORD, LPPALETTEENTRY);
typedef HRESULT(SUGARCALL* IDDPSETENTRIES)(iddp*, DWORD, DWORD, DWORD, LPPALETTEENTRY);

HRESULT SUGARCALL iddp_create(sugar* manager, const GUID* riid, iddp** object);
void SUGARCALL iddp_release(iddp* self);

HRESULT SUGARCALL iddp_query_interface(iddp* self, const GUID* riid, void** object);
ULONG SUGARCALL iddp_add_ref(iddp* self);
ULONG SUGARCALL iddp_remove_ref(iddp* self);
