#include "dd.h"

HRESULT sugar_create(allocator* allocator, logger* logger, sugar** object) {
    if (allocator == NULL || logger == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    sugar* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_SUGAR, sizeof(sugar), &instance))) {
        instance->allocator = allocator;
        instance->logger = logger;
        InitializeCriticalSection(&instance->lock);
        if (SUCCEEDED(hr = arr_create(allocator, MEM_TAG_SUGAR, &instance->items))) {
            logger_log(logger, LOG_LEVEL_TRACE, "SugarDraw started successfully.");
            *object = instance;
            return hr;
        }

        logger_log(logger, LOG_LEVEL_ERROR, "SugarDraw could not start: %s.", hresult_to_string(hr));
        sugar_release(instance);
    }

    return hr;
}

void sugar_release(sugar* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);
        logger_log(self->logger, LOG_LEVEL_TRACE, "SugarDraw is shutting down.");

        if (self->items != NULL) {
            const int count = arr_get_count(self->items);
            for (int i = 0; i < count; i++) {
                dd* instance = NULL;
                if (SUCCEEDED(arr_get_item(self->items, i, &instance))) {
                    dd_release(instance);
                }
            }

            arr_release(self->items);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self);
        logger_release(self->logger);
        allocator_release(self->allocator);
    }
}

HRESULT sugar_create_direct_draw(sugar* self, const GUID* rclsid, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rclsid == NULL || riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsEqualGUID(&CLSID_DirectDraw, rclsid)
        && !IsEqualGUID(&CLSID_DirectDraw7, rclsid)) {
        return E_NOINTERFACE;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    dd* instance = NULL;
    if (SUCCEEDED(hr = dd_create(self, rclsid, &instance))) {
        instance->manager = self;
        if (SUCCEEDED(hr = dd_initialize(instance, riid))) {
            idd* intfc = NULL;
            if (SUCCEEDED(hr = dd_query_interface(instance, riid, &intfc))) {
                if (SUCCEEDED(hr = arr_add_item(self->items, instance))) {
                    *object = intfc;
                    goto exit;
                }
            }
        }

        dd_release(instance);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}
