#pragma once

#include "sugar.h"

typedef struct ddc ddc;

typedef struct iddc {
    const void* self;
    sugar*      manager;
    logger*     logger;
    GUID        id;
    s32         refs;
    ddc*        instance;
} iddc;

typedef struct iddcconn {
    GUID                id;
    ddc*                instance;
} iddcconn;

typedef HRESULT(SUGARCALL* IDDCQUERYINTERFACE)(iddc*, REFIID, LPVOID*);
typedef ULONG(SUGARCALL*   IDDCADDREF)(iddc*);
typedef ULONG(SUGARCALL*   IDDCRELEASE)(iddc*);
typedef HRESULT(SUGARCALL* IDDCGETCLIPLIST)(iddc*, LPRECT, LPRGNDATA, LPDWORD);
typedef HRESULT(SUGARCALL* IDDCGETHWND)(iddc*, HWND*);
typedef HRESULT(SUGARCALL* IDDCINITIALIZE)(iddc*, LPDIRECTDRAW, DWORD);
typedef HRESULT(SUGARCALL* IDDCISCLIPLISTCHANGED)(iddc*, LPBOOL);
typedef HRESULT(SUGARCALL* IDDCSETCLIPLIST)(iddc*, LPRGNDATA, DWORD);
typedef HRESULT(SUGARCALL* IDDCSETHWND)(iddc*, DWORD, HWND);

HRESULT SUGARCALL iddc_create(sugar* manager, const GUID* riid, iddc** object);
void SUGARCALL iddc_release(iddc* self);

HRESULT SUGARCALL iddc_query_interface(iddc* self, const GUID* riid, void** object);
ULONG SUGARCALL iddc_add_ref(iddc* self);
ULONG SUGARCALL iddc_remove_ref(iddc* self);
