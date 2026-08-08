#include "cf.h"
#include "dd.h"
#include "ddc.h"
#include "ddf.h"
#include "icf.h"

HRESULT cf_create(sugar* manager, const GUID* rclsid, cf** object) {
    if (manager == NULL || rclsid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = S_OK;
    cf* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_CLASSFACTORY, sizeof(cf), &instance))) {
        instance->manager = manager;
        CopyMemory(&instance->id, rclsid, sizeof(GUID));
        if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_CLASSFACTORY, &instance->interfaces))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void cf_release(cf* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const u32 item_count = intfc_get_count(self->interfaces);
            for (u32 i = 0; i < item_count; i++) {
                icf* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    icf_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            sugar_remove_cf(self->manager, self);
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT cf_get_interface(cf* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return intfc_query_item(self->interfaces, riid, object);
}

HRESULT cf_query_interface(cf* self, const GUID* riid, void** object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    icf* instance = NULL;
    if (SUCCEEDED(hr = intfc_query_item(self->interfaces, riid, &instance))) {
        icf_add_ref(instance);
        *object = instance;
        goto exit;
    }

    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IClassFactory, riid)) {
        if (SUCCEEDED(hr = icf_create(self->manager, riid, &instance))) {
            instance->instance = self;
            if (SUCCEEDED(hr = cf_add_ref(self, instance))) {
                *object = instance;
                goto exit;
            }

            icf_release(instance);
            goto exit;
        }
    }

    hr = E_NOINTERFACE;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT cf_add_ref(cf* self, icf* object) {
    return intfc_add_item(self->interfaces, &object->id, object);
}

HRESULT cf_remove_ref(cf* self, icf* object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            cf_release(self, RELEASE_NOTIFY);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT cf_create_instance(cf* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (IsEqualGUID(&CLSID_DirectDraw, &self->id)
        || IsEqualGUID(&CLSID_DirectDraw7, &self->id)) {
        dd* instance = NULL;
        if (SUCCEEDED(hr = dd_create(self->manager, &self->id, self->manager->driver, &instance))) {
            idd* intfc = NULL;
            if (SUCCEEDED(hr = dd_query_interface(instance, riid, &intfc))) {
                if (SUCCEEDED(hr = arr_add_item(self->manager->items, instance))) {
                    *object = intfc;
                    goto exit;
                }
            }

            dd_release(instance, RELEASE_NONE);
            goto exit;
        }
    }
    else if (IsEqualGUID(&CLSID_DirectDrawClipper, &self->id)) {
        ddc* instance = NULL;
        if (SUCCEEDED(hr = ddc_create(self->manager, &self->id, &instance))) {
            iddc* intfc = NULL;
            if (SUCCEEDED(hr = ddc_query_interface(instance, riid, &intfc))) {
                if (SUCCEEDED(hr = arr_add_item(self->manager->clippers, instance))) {
                    *object = intfc;
                    goto exit;
                }
            }

            ddc_release(instance, RELEASE_NONE);
            goto exit;
        }
    }
    else if (IsEqualGUID(&CLSID_DirectDrawFactory, &self->id)
        || IsEqualGUID(&CLSID_DirectDrawFactory2, &self->id)) {
        ddf* instance = NULL;
        if (SUCCEEDED(hr = ddf_create(self->manager, &self->id, &instance))) {
            iddf* intfc = NULL;
            if (SUCCEEDED(hr = ddf_query_interface(instance, riid, &intfc))) {
                if (SUCCEEDED(hr = arr_add_item(self->manager->ddfs, instance))) {
                    *object = intfc;
                    goto exit;
                }
            }

            ddf_release(instance, RELEASE_NONE);
            goto exit;
        }
    }

    hr = E_NOINTERFACE;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}
