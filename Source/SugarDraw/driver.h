#pragma once

#include "common.h"

#define DRIVERCALL  __stdcall

#define DRV_OK                  S_OK
#define DRVERR_OUTOFMEMORY      E_OUTOFMEMORY
#define DRVERR_NOTIMPLEMENTED   E_NOTIMPL
#define DRVERR_INVALIDARG       E_INVALIDARG
#define DRVERR_INVALIDOBJECT    E_POINTER
#define DRVERR_GENERIC          E_FAIL

typedef HRESULT(DRIVERCALL* DRVINITIALIZE)(void* drv);
//typedef HRESULT(DRIVERCALL* DRVGETSURFACE)(void* drv, graphics_surface* surface);
typedef HRESULT(DRIVERCALL* DRVRELEASE)(void* drv);

typedef struct driver {
    void*           object;
    DRVINITIALIZE   initialize;
    //DRVGETSURFACE   get_surface;
    DRVRELEASE      release;
} driver;
