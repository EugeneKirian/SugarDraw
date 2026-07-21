#include "ddgc.h"
#include "iddgc.h"

static HRESULT SUGARCALL iddgc_get_gamma_ramp(iddgc*, DWORD, LPDDGAMMARAMP);
static HRESULT SUGARCALL iddgc_set_gamma_ramp(iddgc*, DWORD, LPDDGAMMARAMP);

typedef struct iddgcu_vft {
    IDDGCQUERYINTERFACE     QueryInterface;
    IDDGCADDREF             AddRef;
    IDDGCRELEASE            Release;
} iddgcu_vft;

const static iddgcu_vft iddgcu_self = {
    iddgc_query_interface,
    iddgc_add_ref,
    iddgc_remove_ref
};

typedef struct iddgc_vft {
    IDDGCQUERYINTERFACE     QueryInterface;
    IDDGCADDREF             AddRef;
    IDDGCRELEASE            Release;
    IDDGCGETGAMMARAMP       GetGammaRamp;
    IDDGCSETGAMMARAMP       SetGammaRamp;
} iddgc_vft;

const static iddgc_vft iddgc_self = {
    iddgc_query_interface,
    iddgc_add_ref,
    iddgc_remove_ref,
    iddgc_get_gamma_ramp,
    iddgc_set_gamma_ramp
};

HRESULT SUGARCALL iddgc_create(sugar* manager, const GUID* riid, iddgc** object) {
    if (manager == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    LOGENTER(manager->logger, "%s, 0x%p", guid_to_string(riid), object);

    if (riid == NULL || object == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (!IsEqualGUID(&IID_IUnknown, riid)
        && !IsEqualGUID(&IID_IDirectDrawGammaControl, riid)) {
        LOGLEAVE(manager->logger, E_NOINTERFACE);
    }

    HRESULT hr = DD_OK;
    iddgc* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_IDIRECTDRAWGAMMACONTROL, sizeof(iddgc), &instance))) {
        if (IsEqualGUID(&IID_IUnknown, riid)) {
            instance->self = &iddgcu_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawGammaControl, riid)) {
            instance->self = &iddgc_self;
        }

        instance->manager = manager;
        instance->logger = manager->logger;
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    LOGLEAVESELF(manager->logger, hr, instance);
}


void SUGARCALL iddgc_release(iddgc* self) {
    if (self != NULL) {
        allocator_free(self->manager->allocator, self);
    }
}

HRESULT SUGARCALL iddgc_query_interface(iddgc* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p", guid_to_string(riid), object);

    LEAVE(ddgc_query_interface(self->instance, riid, object));
}

ULONG SUGARCALL iddgc_add_ref(iddgc* self) {
    if (self == NULL) {
        return 0;
    }

    return InterlockedIncrement(&self->refs);
}

ULONG SUGARCALL iddgc_remove_ref(iddgc* self) {
    if (self == NULL || self->refs == 0) {
        return 0;
    }

    s32 result = InterlockedDecrement(&self->refs);

    if ((result = max(result, 0)) == 0) {
        self->refs = 0;

        if (self->instance != NULL) {
            ddgc_remove_ref(self->instance, self);
        }

        iddgc_release(self);
    }

    return result;
}

HRESULT SUGARCALL iddgc_get_gamma_ramp(iddgc* self, DWORD dwFlags, LPDDGAMMARAMP lpRampData) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%08X 0x%p", dwFlags, lpRampData);

    if (dwFlags != DDGGR_NONE) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpRampData == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddgc_get_gamma_ramp(self->instance, lpRampData));
}

HRESULT SUGARCALL iddgc_set_gamma_ramp(iddgc* self, DWORD dwFlags, LPDDGAMMARAMP lpRampData) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s 0x%p", ddsgr_to_string(dwFlags), lpRampData);

    if (dwFlags != DDSGR_NONE && dwFlags != DDSGR_CALIBRATE) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpRampData == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddgc_set_gamma_ramp(self->instance, lpRampData));
}
