#include "idd.h"
#include "dd.h"

HRESULT dd_create(sugar* manager, const GUID* rclsid, dd** object) {
    if (manager == NULL || rclsid == NULL || object == NULL) {
        return E_INVALIDARG;
    }

    if (!IsEqualGUID(&CLSID_DirectDraw, rclsid) && !IsEqualGUID(&CLSID_DirectDraw7, rclsid)) {
        return E_NOINTERFACE;
    }

    HRESULT hr = DD_OK;
    dd* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAW, sizeof(dd), &instance))) {
        instance->manager = manager;
        CopyMemory(&instance->id, rclsid, sizeof(GUID));
        if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAW, &instance->interfaces))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void dd_release(dd* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const int count = intfc_get_count(self->interfaces);
            for (int i = 0; i < count; i++) {
                idd* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    idd_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT dd_query_interface(dd* self, const GUID* riid, void** object) {
    HRESULT hr = E_NOINTERFACE;
    EnterCriticalSection(&self->lock);

    idd* instance = NULL;
    if (SUCCEEDED(hr = intfc_query_item(self->interfaces, riid, &instance))) {
        idd_add_ref(instance);
        *object = instance;
        goto exit;
    }

    const bool valid = IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IDirectDraw, riid) || IsEqualGUID(&IID_IDirectDraw2, riid)
        || IsEqualGUID(&IID_IDirectDraw4, riid) || IsEqualGUID(&IID_IDirectDraw7, riid);

    if (valid) {
        if (SUCCEEDED(hr = idd_create(self->manager, riid, &instance))) {
            instance->instance = self;
            if (SUCCEEDED(hr = dd_add_ref(self, instance))) {
                *object = instance;
                goto exit;
            }

            idd_release(instance);
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_add_ref(dd* self, idd* object) {
    return intfc_add_item(self->interfaces, &object->id, object);
}

HRESULT dd_remove_ref(dd* self, idd* object) {
    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            dd_release(self);
        }
    }

    return hr;
}

HRESULT dd_initialize(dd* self, const GUID* riid) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->initialized) {
        return DDERR_ALREADYINITIALIZED;
    }

    // TODO
    // Actually initialize rendering device, whether GDI, D3D9, etc...

    self->initialized = TRUE;

    return DD_OK;
}
