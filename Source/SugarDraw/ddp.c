#include "dd.h"
#include "ddp.h"
#include "dds.h"
#include "iddp.h"
#include "utilities.h"

HRESULT ddp_create(sugar* manager, ddp** object) {
    if (manager == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    ddp* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWPALETTE, sizeof(ddp), &instance))) {
        instance->manager = manager;
        if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAWPALETTE, &instance->interfaces))) {
            if (SUCCEEDED(hr = arr_create(manager->allocator, MEM_TAG_DIRECTDRAWPALETTE, &instance->surfaces))) {
                InitializeCriticalSection(&instance->lock);
                *object = instance;
                return hr;
            }

            intfc_release(instance->interfaces);
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void ddp_release(ddp* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const s32 count = intfc_get_count(self->interfaces);
            for (s32 i = 0; i < count; i++) {
                iddp* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    iddp_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        if (self->surfaces != NULL) {
            // This should never happen...
            const s32 item_count = arr_get_count(self->surfaces);
            for (s32 i = 0; i < item_count; i++) {
                dds* instance = NULL;
                if (SUCCEEDED(arr_get_item(self->surfaces, i, &instance))) {
                    dds_remove_palette(instance);
                }
            }

            arr_release(self->surfaces);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            if (self->instance != NULL) {
                dd_remove_palette(self->instance, self);
            }
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT ddp_get_interface(ddp* self, const GUID* riid, void** object) {
    HRESULT hr = E_NOINTERFACE;
    EnterCriticalSection(&self->lock);

    const s32 item_count = intfc_get_count(self->interfaces);
    for (s32 i = 0; i < item_count; i++) {
        iddp* instance = NULL;
        if (SUCCEEDED(hr = intfc_get_item(self->interfaces, i, &instance))) {
            if (IsEqualGUID(riid, &instance->id)) {
                *object = instance;
                goto exit;
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddp_query_interface(ddp* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = E_NOINTERFACE;
    EnterCriticalSection(&self->lock);

    iddp* instance = NULL;
    if (SUCCEEDED(hr = intfc_query_item(self->interfaces, riid, &instance))) {
        iddp_add_ref(instance);
        *object = instance;
        goto exit;
    }

    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IDirectDrawPalette, riid)) {
        if (SUCCEEDED(hr = iddp_create(self->manager, riid, &instance))) {
            instance->instance = self;
            if (SUCCEEDED(hr = ddp_add_ref(self, instance))) {
                *object = instance;
                goto exit;
            }

            iddp_release(instance);
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddp_add_ref(ddp* self, iddp* object) {
    return intfc_add_item(self->interfaces, &object->id, object);
}

HRESULT ddp_remove_ref(ddp* self, iddp* object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            ddp_release(self, RELEASE_NOTIFY);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddp_get_caps(ddp* self, u32* caps) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (caps == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    *caps = self->caps;

    return DD_OK;
}

HRESULT ddp_get_entries(ddp* self, u32 flags, u32 base, u32 count, PALETTEENTRY* entries) {
    if (self == NULL) {
        return DDERR_INVALIDPARAMS;
    }
    
    // TODO better checks on count for 1, 2, 4, and 8-bit palettes
    if (flags != DDPFLAGS_NONE
        || base >= PALETTE_MAX_ENTRY_COUNT
        || count > PALETTE_MAX_ENTRY_COUNT
        || (base + count > PALETTE_MAX_ENTRY_COUNT)) {
        return DDERR_INVALIDPARAMS;
    }

    if (entries == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO indexed palettes
    // TODO checks for base and count for non 8-bit palettes

    if (self->caps & DDPCAPS_8BITENTRIES) {
        return DDERR_UNSUPPORTED; // TODO
    }

    EnterCriticalSection(&self->lock);
    CopyMemory(entries, &self->entries[base], count * sizeof(PALETTEENTRY));
    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddp_initialize(ddp* self, dd* object, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    self->caps = flags;
    self->instance = object;

    return DD_OK;
}

HRESULT ddp_set_entries(ddp* self, u32 flags, u32 start, u32 count, PALETTEENTRY* entries) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (flags != DDPFLAGS_NONE
        || start >= PALETTE_MAX_ENTRY_COUNT
        || count > PALETTE_MAX_ENTRY_COUNT
        || (start + count > PALETTE_MAX_ENTRY_COUNT)) {
        return DDERR_INVALIDPARAMS;
    }

    if (entries == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO indexed palettes
    // TODO checks for start and count for non 8-bit palettes

    if (self->caps & DDPCAPS_8BITENTRIES) {
        return DDERR_UNSUPPORTED; // TODO
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);
    CopyMemory(&self->entries[start], entries, count * sizeof(PALETTEENTRY));

    if (!(self->caps & DDPCAPS_ALLOW256)) {
        // TODO handle non 8-bit
        // TODO handle alpha caps
        self->entries[0].peRed = 0;
        self->entries[0].peGreen = 0;
        self->entries[0].peBlue = 0;
        self->entries[0].peFlags = 0;

        self->entries[255].peRed = 255;
        self->entries[255].peGreen = 255;
        self->entries[255].peBlue = 255;
        self->entries[255].peFlags = 0;
    }

    if (SUCCEEDED(hr = palette_entry_to_rgb_quad(&self->entries[start], count, &self->quads[start]))) {
        const s32 item_count = arr_get_count(self->surfaces);
        for (s32 i = 0; i < item_count; i++) {
            dds* instance = NULL;
            if (SUCCEEDED(arr_get_item(self->surfaces, i, &instance))) {
                hr = dds_set_palette_entries(instance, start, count, self->quads);
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddp_register_surface(ddp* self, dds* surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO update caps with DDPCAPS_PRIMARYSURFACE, DDPCAPS_PRIMARYSURFACELEFT

    hr = arr_add_item(self->surfaces, surface);

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddp_unregister_surface(ddp* self, dds* surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO update caps with DDPCAPS_PRIMARYSURFACE, DDPCAPS_PRIMARYSURFACELEFT

    const s32 item_count = arr_get_count(self->surfaces);
    for (s32 i = 0; i < item_count; i++) {
        dds* instance = NULL;
        if (SUCCEEDED(arr_get_item(self->surfaces, i, &instance))) {
            if (instance == surface) {
                hr = arr_remove_item(self->surfaces, i);
                goto exit;
            }
        }
    }

    hr = DDERR_NOTFOUND;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}
