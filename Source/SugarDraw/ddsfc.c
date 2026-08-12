#include "dds.h"
#include "ddsfc.h"

static HRESULT ddsfc_lock(ddsfc* self, u32 start);
static HRESULT ddsfc_unlock(ddsfc* self, u32 start);

static HRESULT ddsfc_rotate(ddsfc* self, u32 start);

HRESULT ddsfc_create(allocator* allocator, dds* surface, ddsfc** object) {
    if (allocator == NULL || surface == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    ddsfc* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_DIRECTDRAWSURFACEFLIPCHAIN, sizeof(ddsfc), &instance))) {
        instance->allocator = allocator;
        instance->instance = surface;
        if (SUCCEEDED(hr = arr_create(allocator, MEM_TAG_DIRECTDRAWSURFACEFLIPCHAIN, &instance->surfaces))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void ddsfc_release(ddsfc* self) {
    if (self != NULL) {
        if (self->surfaces != NULL) {
            arr_release(self->surfaces);
        }

        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self);
    }
}

HRESULT ddsfc_add_surface(ddsfc* self, dds* surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    hr = arr_add_item(self->surfaces, surface);

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsfc_remove_surface(ddsfc* self, dds* surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    hr = arr_remove_item(self->surfaces, surface);

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsfc_flip(ddsfc* self, dds* override) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (arr_get_count(self->surfaces) == 0) {
        return DDERR_NOTFLIPPABLE;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    u32 index = 0;
    if (override != NULL) {
        // When the override provided - find its index in the array.
        if (FAILED(hr = arr_get_index(self->surfaces, override, &index))) {
            goto exit;
        }
    }

    // TODO: target and source must be both in DDSCAPS_SYSTEMMEMORY or in DDSCAPS_VIDEOMEMORY

    if (SUCCEEDED(hr = ddsfc_lock(self, index))) {
        // Accorting to Flipping Surfaces in the documentation:
        // The undelying surface pointer rotation of the back buffers starts from 0 (zero),
        // or from the index of the override surface.

        hr = ddsfc_rotate(self, index);

        ddsfc_unlock(self, index);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsfc_lock(ddsfc* self, u32 start) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;

    // Attempt to lock the surface itself.
    MAKETYPE(RECT, rect);
    if (SUCCEEDED(hr = ddsd_get_rect(self->instance->surface, &rect))) {
        hr = ddsd_lock_rect(self->instance->surface, &rect, DDLOCK_READONLY);
    }

    if (FAILED(hr)) {
        return DDERR_SURFACEBUSY;
    }

    // Attempt to lock the surfaces in the swapchain starting at the provided index

    u32 locks = 0;
    const u32 item_count = arr_get_count(self->surfaces);
    for (u32 i = start; i < item_count; i++) {
        dds* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->surfaces, i, &instance))) {
            if (SUCCEEDED(hr = ddsd_get_rect(instance->surface, &rect))) {
                if (SUCCEEDED(hr = ddsd_lock_rect(instance->surface, &rect, DDLOCK_READONLY))) {
                    locks++;
                    continue;
                }
            }
        }

        goto fail;
    }

    return DD_OK;

fail:
    // Handle the failure to lock surfaces by:
    // 1. Unwinding locks of the swap chain surfaces
    // 2. Unlocking the surface itself

    for (u32 i = start; i < start + locks; i++) {
        dds* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->surfaces, i, &instance))) {
            if (SUCCEEDED(hr = ddsd_get_rect(instance->surface, &rect))) {
                hr = ddsd_unlock_rect(instance->surface, &rect);
            }
        }
    }

    if (SUCCEEDED(hr = ddsd_get_rect(self->instance->surface, &rect))) {
        hr = ddsd_unlock_rect(self->instance->surface, &rect);
    }

    return DDERR_LOCKEDSURFACES;
}

HRESULT ddsfc_unlock(ddsfc* self, u32 start) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;

    MAKETYPE(RECT, rect);
    const u32 item_count = arr_get_count(self->surfaces);
    for (u32 i = start; i < item_count; i++) {
        dds* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->surfaces, i, &instance))) {
            if (SUCCEEDED(hr = ddsd_get_rect(instance->surface, &rect))) {
                if (SUCCEEDED(hr = ddsd_unlock_rect(instance->surface, &rect))) {
                    continue;
                }
            }
        }

        goto exit;
    }

    if (SUCCEEDED(hr = ddsd_get_rect(self->instance->surface, &rect))) {
        hr = ddsd_unlock_rect(self->instance->surface, &rect);
    }

exit:
    return hr;
}

HRESULT ddsfc_rotate(ddsfc* self, u32 start) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    ddsd* current = self->instance->surface;

    dds* back = NULL;
    if (SUCCEEDED(hr = arr_get_item(self->surfaces, start, &back))) {
        self->instance->surface = back->surface;
    }

    const u32 item_count = arr_get_count(self->surfaces);
    for (u32 i = start; i < item_count - 1; i++) {
        dds* destination = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->surfaces, i, &destination))) {
            dds* source = NULL;
            if (SUCCEEDED(hr = arr_get_item(self->surfaces, i + 1, &source))) {
                destination->surface = source->surface;
            }
        }
    }

    dds* last = NULL;
    if (SUCCEEDED(hr = arr_get_item(self->surfaces, item_count - 1, &last))) {
        last->surface = current;
    }

    return hr;
}
