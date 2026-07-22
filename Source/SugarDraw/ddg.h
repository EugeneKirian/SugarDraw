#pragma once

#include "sugar.h"

typedef struct dd dd;
typedef struct ddsd ddsd;
typedef struct driver driver;

#define DDGSTATUS_NONE          0x00000000
#define DDGSTATUS_SIGNALED      0x00000001
#define DDGSTATUS_UPDATING      0x00000002

typedef struct ddg {
    sugar*              manager;
    dd*                 instance;
    driver*             driver;
    CRITICAL_SECTION    lock;
    HANDLE              worker;
    u32                 status;
    ddsd*               surface;
    HANDLE              stop, done, ready, updating;
} ddg;

HRESULT ddg_create(sugar* manager, driver* driver, ddg** object);
void ddg_release(ddg* self);

HRESULT ddg_get_status(ddg* self, u32* status);

HRESULT ddg_is_ready(ddg* self, bool wait);
HRESULT ddg_is_updating(ddg* self, bool wait);

HRESULT ddg_initialize(ddg* self, dd* object);

HRESULT ddg_set_driver(ddg* self, driver* driver);

HRESULT ddg_recreate_surface(ddg* self);

HRESULT ddg_signal_update(ddg* self);
