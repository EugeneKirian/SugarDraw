#pragma once

#include "sugar.h"

typedef struct ddf ddf;

typedef struct iddf {
    const void* self;
    sugar*      manager;
    logger*     logger;
    GUID        id;
    s32         refs;
    ddf*        instance;
} iddf;

typedef HRESULT(SUGARCALL* IDDFQUERYINTERFACE)(iddf*, REFIID, LPVOID*);
typedef ULONG(SUGARCALL* IDDFADDREF)(iddf*);
typedef ULONG(SUGARCALL* IDDFRELEASE)(iddf*);

typedef HRESULT(SUGARCALL* IDDFCREATEDIRECTDRAW1)(iddf*, GUID*, HWND, DWORD, DWORD, LPUNKNOWN, LPDIRECTDRAW*);
typedef HRESULT(SUGARCALL* IDDFDIRECTDRAWENUMERATE1)(iddf*, LPDDENUMCALLBACKA, LPVOID);

typedef HRESULT(SUGARCALL* IDDFCREATEDIRECTDRAW2)(iddf*, GUID*, HWND, DWORD, DWORD, LPUNKNOWN, LPDIRECTDRAW4*);
typedef HRESULT(SUGARCALL* IDDFDIRECTDRAWENUMERATE2)(iddf*, LPDDENUMCALLBACKEXA, LPVOID, DWORD);

HRESULT SUGARCALL iddf_create(sugar* manager, const GUID* riid, iddf** object);
void SUGARCALL iddf_release(iddf* self);

HRESULT SUGARCALL iddf_query_interface(iddf* self, const GUID* riid, void** object);
ULONG SUGARCALL iddf_add_ref(iddf* self);
ULONG SUGARCALL iddf_remove_ref(iddf* self);
