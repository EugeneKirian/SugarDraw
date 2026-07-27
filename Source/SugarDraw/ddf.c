#include "dd.h"
#include "ddf.h"
#include "idd.h"
#include "iddf.h"

HRESULT ddf_create(sugar* manager, const GUID* rclsid, ddf** object) {
    if (manager == NULL || rclsid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = S_OK;
    ddf* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWFACTORY, sizeof(ddf), &instance))) {
        instance->manager = manager;
        CopyMemory(&instance->id, rclsid, sizeof(GUID));
        if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAWFACTORY, &instance->interfaces))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void ddf_release(ddf* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const s32 count = intfc_get_count(self->interfaces);
            for (s32 i = 0; i < count; i++) {
                iddf* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    iddf_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            sugar_remove_ddf(self->manager, self);
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT ddf_get_interface(ddf* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return intfc_query_item(self->interfaces, riid, object);
}

HRESULT ddf_query_interface(ddf* self, const GUID* riid, void** object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    iddf* instance = NULL;
    if (SUCCEEDED(hr = intfc_query_item(self->interfaces, riid, &instance))) {
        iddf_add_ref(instance);
        *object = instance;
        goto exit;
    }

    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IDirectDrawFactory, riid)
        || IsEqualGUID(&IID_IDirectDrawFactory2, riid)) {
        if (SUCCEEDED(hr = iddf_create(self->manager, riid, &instance))) {
            instance->instance = self;
            if (SUCCEEDED(hr = ddf_add_ref(self, instance))) {
                *object = instance;
                goto exit;
            }

            iddf_release(instance);
            goto exit;
        }
    }

    hr = E_NOINTERFACE;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddf_add_ref(ddf* self, iddf* object) {
    return intfc_add_item(self->interfaces, &object->id, object);
}

HRESULT ddf_remove_ref(ddf* self, iddf* object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            ddf_release(self, RELEASE_NOTIFY);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddf_create_direct_draw(ddf* self, const GUID* riid, const GUID* device, HWND hwnd, u32 flags, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO verify ref counts on interfaces

    // This method creates DirectDraw objects in the same way that the DirectDraw DirectDrawCreate function
    // is used to create DirectDraw objects, and sets cooperative levels the same way the IDirectDraw2::SetCooperativeLevel
    // method sets cooperative levels. However, in addition to creating a DirectDraw object, successful calls
    // to the IDirectDrawFactory::CreateDirectDraw method will obtain a pointer to the IUnknown and IDirectDraw interfaces,
    // which are exposed on the DirectDraw object.
    // Applications can now query the DirectDraw object to obtain the address of a pointer to an IDirectDraw3 interface.


    idd* instance = NULL;
    if (SUCCEEDED(hr = sugar_create_dd(self->manager, device, &CLSID_DirectDraw, riid, &instance))) {
        if (SUCCEEDED(hr = dd_set_cooperative_level(instance->instance, hwnd, flags))) {
            *object = instance;
            goto exit;
        }

        dd_release(instance->instance, RELEASE_NOTIFY);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}
