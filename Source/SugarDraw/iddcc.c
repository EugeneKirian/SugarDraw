#include "ddcc.h"
#include "iddcc.h"

static HRESULT SUGARCALL iddcc_get_color_controls(iddcc*, LPDDCOLORCONTROL);
static HRESULT SUGARCALL iddcc_set_color_controls(iddcc*, LPDDCOLORCONTROL);

typedef struct iddccu_vft {
    IDDCCQUERYINTERFACE     QueryInterface;
    IDDCCADDREF             AddRef;
    IDDCCRELEASE            Release;
} iddccu_vft;

const static iddccu_vft iddccu_self = {
    iddcc_query_interface,
    iddcc_add_ref,
    iddcc_remove_ref
};

typedef struct iddcc_vft {
    IDDCCQUERYINTERFACE     QueryInterface;
    IDDCCADDREF             AddRef;
    IDDCCRELEASE            Release;
    IDDCCGETCOLORCONTROLS   GetColorControls;
    IDDCCSETCOLORCONTROLS   SetColorControls;
} iddcc_vft;

const static iddcc_vft iddcc_self = {
    iddcc_query_interface,
    iddcc_add_ref,
    iddcc_remove_ref,
    iddcc_get_color_controls,
    iddcc_set_color_controls
};

HRESULT SUGARCALL iddcc_create(sugar* manager, const GUID* riid, iddcc** object) {
    if (manager == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    LOGENTER(manager->logger, "%s, 0x%p", guid_to_string(riid), object);

    if (riid == NULL || object == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (!IsEqualGUID(&IID_IUnknown, riid)
        && !IsEqualGUID(&IID_IDirectDrawColorControl, riid)) {
        LOGLEAVE(manager->logger, E_NOINTERFACE);
    }

    HRESULT hr = DD_OK;
    iddcc* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_IDIRECTDRAWCOLORCONTROL, sizeof(iddcc), &instance))) {
        if (IsEqualGUID(&IID_IUnknown, riid)) {
            instance->self = &iddccu_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawColorControl, riid)) {
            instance->self = &iddcc_self;
        }

        instance->manager = manager;
        instance->logger = manager->logger;
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    LOGLEAVESELF(manager->logger, hr, instance);
}


void SUGARCALL iddcc_release(iddcc* self) {
    if (self != NULL) {
        allocator_free(self->manager->allocator, self);
    }
}

HRESULT SUGARCALL iddcc_query_interface(iddcc* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p", guid_to_string(riid), object);

    LEAVE(ddcc_query_interface(self->instance, riid, object));
}

ULONG SUGARCALL iddcc_add_ref(iddcc* self) {
    if (self == NULL) {
        return 0;
    }

    return InterlockedIncrement(&self->refs);
}

ULONG SUGARCALL iddcc_remove_ref(iddcc* self) {
    if (self == NULL || self->refs == 0) {
        return 0;
    }

    s32 result = InterlockedDecrement(&self->refs);

    if ((result = max(result, 0)) == 0) {
        self->refs = 0;

        if (self->instance != NULL) {
            ddcc_remove_ref(self->instance, self);
        }

        iddcc_release(self);
    }

    return result;
}

HRESULT SUGARCALL iddcc_get_color_controls(iddcc* self, LPDDCOLORCONTROL lpColorControl) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpColorControl);

    if (lpColorControl == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpColorControl->dwSize != sizeof(DDCOLORCONTROL)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddcc_get_color_controls(self->instance, lpColorControl));
}

HRESULT SUGARCALL iddcc_set_color_controls(iddcc* self, LPDDCOLORCONTROL lpColorControl) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpColorControl);

    if (lpColorControl == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpColorControl->dwSize != sizeof(DDCOLORCONTROL)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddcc_set_color_controls(self->instance, lpColorControl));
}
