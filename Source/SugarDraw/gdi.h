#pragma once

#include "driver.h"

typedef struct gdi gdi;

HRESULT DRIVERCALL gdi_create(gdi** object);
void DRIVERCALL gdi_release(gdi* self);

HRESULT DRIVERCALL gdi_get_driver(gdi* self, driver** object);
