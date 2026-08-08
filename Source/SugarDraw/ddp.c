#include "dd.h"
#include "ddg.h"
#include "ddp.h"
#include "dds.h"
#include "iddp.h"
#include "plt.h"
#include "utilities.h"

HRESULT ddp_create(sugar* manager, ddp** object) {
    if (manager == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    ddp* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWPALETTE, sizeof(ddp), &instance))) {
        instance->manager = manager;
        if (SUCCEEDED(hr = plt_create(manager->allocator, MEM_TAG_DIRECTDRAWPALETTE, &instance->lookup))) {
            if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAWPALETTE, &instance->interfaces))) {
                if (SUCCEEDED(hr = arr_create(manager->allocator, MEM_TAG_DIRECTDRAWPALETTE, &instance->surfaces))) {
                    InitializeCriticalSection(&instance->lock);
                    *object = instance;
                    return hr;
                }

                intfc_release(instance->interfaces);
            }

            plt_release(instance->lookup);
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
            const u32 item_count = arr_get_count(self->surfaces);
            for (u32 i = 0; i < item_count; i++) {
                dds* instance = NULL;
                if (SUCCEEDED(arr_get_item(self->surfaces, i, &instance))) {
                    if (SUCCEEDED(dds_wait_for_vertical_blank(instance, TRUE))) {
                        dds_remove_palette(instance);
                    }
                }
            }

            arr_release(self->surfaces);
        }

        if (self->lookup != NULL) {
            plt_release(self->lookup);
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
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return intfc_query_item(self->interfaces, riid, object);
}

HRESULT ddp_query_interface(ddp* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
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
            goto exit;
        }
    }

    hr = E_NOINTERFACE;

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

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (flags != DDPFLAGS_NONE) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->count < base + count) {
        return DDERR_INVALIDPARAMS;
    }

    if (entries == NULL) {
        return DDERR_INVALIDPARAMS;
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

    // TODO validate flags

    // TODO indexed palettes

    if (flags & DDPCAPS_1BIT) {
        self->count = 2;
    }
    else if (flags & DDPCAPS_2BIT) {
        self->count = 4;
    }
    else if (flags & DDPCAPS_4BIT) {
        self->count = 16;
    }
    else if (flags & DDPCAPS_8BIT) {
        self->count = PALETTE_MAX_ENTRY_COUNT;
    }
    else {
        return DDERR_UNSUPPORTED; // TODO
    }

    self->caps = flags | DDPCAPS_VSYNC;
    self->instance = object;
    self->uniqueness++;

    return DD_OK;
}

HRESULT ddp_set_entries(ddp* self, u32 flags, u32 start, u32 count, PALETTEENTRY* entries) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (flags != DDPFLAGS_NONE) {
        return DDERR_INVALIDPARAMS;
    }

    if (entries == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->count < start + count) {
        return DDERR_INVALIDPARAMS;
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

    if (SUCCEEDED(hr = ddg_is_ready(self->instance->graphics, TRUE))) {
        if (SUCCEEDED(hr = palette_entry_to_rgb_quad(&self->entries[start], count, &self->quads[start]))) {
            if (SUCCEEDED(hr = plt_set_entries(self->lookup, self->count, self->quads))) {
                const u32 item_count = arr_get_count(self->surfaces);
                for (u32 i = 0; i < item_count; i++) {
                    dds* instance = NULL;
                    if (SUCCEEDED(arr_get_item(self->surfaces, i, &instance))) {
                        hr = dds_set_palette_entries(instance, self->count, self->quads, self->lookup);
                    }
                }

                if (self->caps & (DDPCAPS_PRIMARYSURFACE | DDPCAPS_PRIMARYSURFACELEFT)) {
                    hr = ddg_signal_update(self->instance->graphics);
                }

                self->uniqueness++;
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

    if (SUCCEEDED(hr = arr_add_item(self->surfaces, surface))) {
        if (surface->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
            self->caps |= DDPCAPS_PRIMARYSURFACE;
        }

        if (surface->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACELEFT) {
            self->caps |= DDPCAPS_PRIMARYSURFACELEFT;
        }
    }

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

    if (SUCCEEDED(hr = arr_remove_item(self->surfaces, surface))) {
        if (surface->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
            self->caps &= ~DDPCAPS_PRIMARYSURFACE;
        }

        if (surface->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACELEFT) {
            self->caps &= ~DDPCAPS_PRIMARYSURFACELEFT;
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}
