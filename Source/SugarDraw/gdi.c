#include "gdi.h"

#include <stdlib.h>

typedef struct gdi {
    driver driver;
    // TODO
} gdi;

HRESULT DRIVERCALL gdi_create(gdi** object) {
    if (object == NULL) {
        return DRVERR_INVALIDARG;
    }

    gdi* instance = (gdi*)malloc(sizeof(driver)); // TODO allocator
    if (instance == NULL) {
        return DRVERR_OUTOFMEMORY;
    }

    ZeroMemory(instance, sizeof(gdi)); // TODO allocator

    instance->driver.object = instance;

    // TODO set methods

    *object = instance;

    return DRV_OK;
}

void DRIVERCALL gdi_release(gdi* self) {
    if (self != NULL) {
        // TODO
    }
}

HRESULT DRIVERCALL gdi_get_driver(gdi* self, driver** object) {
    if (self == NULL) {
        return DRVERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DRVERR_INVALIDARG;
    }

    *object = &self->driver;

    return DRV_OK;
}
