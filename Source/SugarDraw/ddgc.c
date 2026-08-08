#include "ddgc.h"
#include "dds.h"
#include "iddgc.h"

HRESULT ddgc_create(sugar* manager, ddgc** object) {
    if (manager == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    ddgc* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWGAMMACONTROL, sizeof(ddgc), &instance))) {
        instance->manager = manager;
        if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAWGAMMACONTROL, &instance->interfaces))) {
            InitializeCriticalSection(&instance->lock);

            // Linear interpolation from 0 to 65,535 (inclusive).
            for (u32 i = 0; i < 256; i++) {
                instance->control.red[i] =
                    instance->control.green[i] =
                    instance->control.blue[i] = i * 257;
            }

            *object = instance;
            return hr;
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void ddgc_release(ddgc* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const u32 item_count = intfc_get_count(self->interfaces);
            for (u32 i = 0; i < item_count; i++) {
                iddgc* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    iddgc_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            if (self->instance != NULL) {
                dds_remove_gamma_control(self->instance);
            }
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT ddgc_get_interface(ddgc* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return intfc_query_item(self->interfaces, riid, object);
}

HRESULT ddgc_query_interface(ddgc* self, const GUID* riid, void** object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    iddgc* instance = NULL;
    if (SUCCEEDED(hr = intfc_query_item(self->interfaces, riid, &instance))) {
        iddgc_add_ref(instance);
        *object = instance;
        goto exit;
    }

    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IDirectDrawGammaControl, riid)) {
        if (SUCCEEDED(hr = iddgc_create(self->manager, riid, &instance))) {
            instance->instance = self;
            if (SUCCEEDED(hr = ddgc_add_ref(self, instance))) {
                *object = instance;
                goto exit;
            }

            iddgc_release(instance);
            goto exit;
        }
    }

    hr = E_NOINTERFACE;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddgc_add_ref(ddgc* self, iddgc* object) {
    return intfc_add_item(self->interfaces, &object->id, object);
}

HRESULT ddgc_remove_ref(ddgc* self, iddgc* object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            ddgc_release(self, RELEASE_NOTIFY);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddgc_get_gamma_ramp(ddgc* self, DDGAMMARAMP* control) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (!(self->instance->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {
        return DDERR_UNSUPPORTED;
    }

    EnterCriticalSection(&self->lock);

    CopyMemory(control, &self->control, sizeof(DDGAMMARAMP));

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddgc_set_gamma_ramp(ddgc* self, DDGAMMARAMP* control) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (!(self->instance->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {
        return DDERR_UNSUPPORTED;
    }

    EnterCriticalSection(&self->lock);

    CopyMemory(&self->control, control, sizeof(DDGAMMARAMP));

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddgc_initialize(ddgc* self, dds* instance) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (instance == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    EnterCriticalSection(&self->lock);

    self->instance = instance;

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}
