#pragma once

#include "sugar.h"

typedef struct dd dd;
typedef struct ddsd ddsd;
typedef struct driver driver;

typedef struct ddg {
    sugar*              manager;
    dd*                 instance;
    driver*             driver;
    CRITICAL_SECTION    lock;
    HANDLE              worker;
    ddsd*               surface;
    volatile bool       update, updating;
    HANDLE              stop, done, waitable;
} ddg;

#define DDGSTATUS_NONE          0x00000000
#define DDGSTATUS_SIGNALED      0x00000001
#define DDGSTATUS_UPDATING      0x00000002

HRESULT ddg_create(sugar* manager, driver* driver, ddg** object);
void ddg_release(ddg* self);

HRESULT ddg_initialize(ddg* self, dd* object);

HRESULT ddg_set_driver(ddg* self, driver* driver);

HRESULT ddg_recreate_surface(ddg* self);

HRESULT ddg_get_status(ddg* self, u32 flags, u32* status);
HRESULT ddg_signal_update(ddg* self);
HRESULT ddg_can_update(ddg* self, bool wait);

// TODO - vsync is here
