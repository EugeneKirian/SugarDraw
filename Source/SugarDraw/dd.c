#include "idd.h"
#include "dd.h"
#include "dds.h"
#include "ddp.h"

HRESULT dd_create(sugar* manager, const GUID* rclsid, dd** object) {
    if (manager == NULL || rclsid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsEqualGUID(&CLSID_DirectDraw, rclsid)
        && !IsEqualGUID(&CLSID_DirectDraw7, rclsid)) {
        return E_NOINTERFACE;
    }

    HRESULT hr = DD_OK;
    dd* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAW, sizeof(dd), &instance))) {
        instance->manager = manager;
        CopyMemory(&instance->id, rclsid, sizeof(GUID));
        if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAW, &instance->interfaces))) {
            if (SUCCEEDED(hr = arr_create(manager->allocator, MEM_TAG_DIRECTDRAW, &instance->surfaces))) {
                if (SUCCEEDED(hr = arr_create(manager->allocator, MEM_TAG_DIRECTDRAW, &instance->palettes))) {
                    InitializeCriticalSection(&instance->lock);
                    *object = instance;
                    return hr;
                }

                arr_release(instance->surfaces);
            }

            intfc_release(instance->interfaces);
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void dd_release(dd* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const s32 count = intfc_get_count(self->interfaces);
            for (s32 i = 0; i < count; i++) {
                idd* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    idd_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        if (self->surfaces != NULL) {
            const s32 item_count = arr_get_count(self->surfaces);
            for (s32 i = 0; i < item_count; i++) {
                dds* instance = NULL;
                if (SUCCEEDED(arr_get_item(self->surfaces, i, &instance))) {
                    dds_release(instance, RELEASE_NONE);
                }
            }

            arr_release(self->surfaces);
        }

        if (self->palettes != NULL) {
            const s32 item_count = arr_get_count(self->palettes);
            for (s32 i = 0; i < item_count; i++) {
                ddp* instance = NULL;
                if (SUCCEEDED(arr_get_item(self->palettes, i, &instance))) {
                    ddp_release(instance, RELEASE_NONE);
                }
            }

            arr_release(self->palettes);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            sugar_remove_direct_draw(self->manager, self);
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT dd_get_interface(dd* self, const GUID* riid, void** object) {
    HRESULT hr = E_NOINTERFACE;
    EnterCriticalSection(&self->lock);

    const s32 item_count = intfc_get_count(self->interfaces);
    for (s32 i = 0; i < item_count; i++) {
        idd* instance = NULL;
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

HRESULT dd_query_interface(dd* self, const GUID* riid, void** object) {
    HRESULT hr = E_NOINTERFACE;
    EnterCriticalSection(&self->lock);

    idd* instance = NULL;
    if (SUCCEEDED(hr = intfc_query_item(self->interfaces, riid, &instance))) {
        idd_add_ref(instance);
        *object = instance;
        goto exit;
    }

    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IDirectDraw, riid)
        || IsEqualGUID(&IID_IDirectDraw2, riid)
        || IsEqualGUID(&IID_IDirectDraw4, riid)
        || IsEqualGUID(&IID_IDirectDraw7, riid)) {
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
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            dd_release(self, RELEASE_NOTIFY);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_create_surface(dd* self, const GUID* riid, DDSURFACEDESC2* desc, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || desc == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO incomplete validations...
    // TODO move most of validations into separate function, so that it can be reused by ::Initialize

    if (self->cooperative_level.flags == DDSCL_NONE) {
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    desc->dwFlags |= DDSD_CAPS;
    if ((desc->dwFlags != DDSD_NONE) && (desc->dwFlags & ~DDSD_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->ddsCaps.dwCaps & ~DDSCAPS_VALID) {
        return DDERR_INVALIDCAPS;
    }

    if ((desc->ddsCaps.dwCaps & DDSCAPS_MODEX)
        && (desc->ddsCaps.dwCaps & DDSCAPS_STANDARDVGAMODE)) {
        return DDERR_INVALIDCAPS;
    }

    if ((desc->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        && (desc->ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)) {
        return DDERR_INVALIDCAPS;
    }

    if (desc->ddsCaps.dwCaps & (DDSCAPS_FLIP | DDSCAPS_COMPLEX)) {
        if (!(desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            && !(desc->ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER))) {
            return DDERR_INVALIDCAPS;
        }

        if (desc->dwFlags & DDSD_BACKBUFFERCOUNT && desc->dwBackBufferCount == 0) {
            return DDERR_INVALIDPARAMS;
        }
    }

    HRESULT hr = DD_OK;
    if ((desc->dwFlags & (DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT))
        != (DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT)) {
        DEVMODEA mode;
        ZeroMemory(&mode, sizeof(DEVMODEA));
        if (FAILED(hr = sugar_get_display_mode(self->manager, &mode))) {
            return hr;
        }

        if (!(desc->dwFlags & DDSD_WIDTH)) {
            desc->dwFlags |= DDSD_WIDTH;
            desc->dwWidth = mode.dmPelsWidth;
        }

        if (!(desc->dwFlags & DDSD_HEIGHT)) {
            desc->dwFlags |= DDSD_HEIGHT;
            desc->dwHeight = mode.dmPelsHeight;
        }

        if (!(desc->dwFlags & DDSD_PIXELFORMAT)) {
            desc->dwFlags |= DDSD_PIXELFORMAT;

            // TODO proper implementation..
            // Surface pixel format can be implicit (i.e. same as primary surface)
            // TODO do we need to set it for non-primary surfaces explicitly?
            // TODO dds_get_pixel_format ...

            ZeroMemory(&desc->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
            desc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

            desc->ddpfPixelFormat.dwFlags = DDPF_RGB;
            desc->ddpfPixelFormat.dwRGBBitCount = mode.dmBitsPerPel;

            switch (mode.dmBitsPerPel) {
            case 1: {
                desc->ddsCaps.dwCaps |= DDSCAPS_PALETTE;
                desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED1;
                return DDERR_UNSUPPORTEDFORMAT; // TODO
            }break;
            case 2: {
                desc->ddsCaps.dwCaps |= DDSCAPS_PALETTE;
                desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED2;
                return DDERR_UNSUPPORTEDFORMAT; // TODO
            }break;
            case 4: {
                desc->ddsCaps.dwCaps |= DDSCAPS_PALETTE;
                desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED4;
                return DDERR_UNSUPPORTEDFORMAT; // TODO
            }break;
            case 8: {
                desc->ddsCaps.dwCaps |= DDSCAPS_PALETTE;
                desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
            }break;
            case 15: {
                desc->ddpfPixelFormat.dwRBitMask = 0x7C00;
                desc->ddpfPixelFormat.dwGBitMask = 0x03E0;
                desc->ddpfPixelFormat.dwBBitMask = 0x001F;
            }break;
            case 16: {
                desc->ddpfPixelFormat.dwRBitMask = 0xF800;
                desc->ddpfPixelFormat.dwGBitMask = 0x07E0;
                desc->ddpfPixelFormat.dwBBitMask = 0x001F;
            }break;
            case 24: {
                desc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
                desc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
                desc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
            }break;
            case 32: {
                desc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
                desc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
                desc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
                desc->ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
            }break;
            }
        }
    }

    // TODO proper caps setting...
    if (desc->ddpfPixelFormat.dwRGBBitCount <= 8) {
        desc->ddsCaps.dwCaps |= DDSCAPS_PALETTE;

        switch (desc->ddpfPixelFormat.dwRGBBitCount) {
        case 1: {
            desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED1;
        }break;
        case 2: {
            desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED2;
        }break;
        case 4: {
            desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED4;
        }break;
        case 8: {
            desc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
        }break;
        }
    }

    if (desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
        if (self->primary != NULL) {
            return DDERR_PRIMARYSURFACEALREADYEXISTS;
        }

        if (desc->dwFlags & (DDSD_PITCH | DDSD_LPSURFACE | DDSD_LINEARSIZE | DDSD_FVF)) {
            return DDERR_INVALIDCAPS;
        }

        if (desc->ddsCaps.dwCaps & (DDSCAPS_FLIP | DDSCAPS_COMPLEX)
            && !(self->cooperative_level.flags & (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE))) {
            return DDERR_NOEXCLUSIVEMODE;
        }

        desc->ddsCaps.dwCaps |= DDSCAPS_VISIBLE;
        if (desc->ddsCaps.dwCaps & DDSCAPS_FLIP) {
            desc->ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
        }
    }

    if (!(desc->dwFlags & DDSD_LPSURFACE)) {
        desc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;// | DDSCAPS_VIDEOMEMORY; // TODO verify...
    }

    // TODO validate pixel format...

    EnterCriticalSection(&self->lock);

    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_create(self->manager, &instance))) {
        if (SUCCEEDED(hr = dds_initialize(instance, self, desc))) {
            idds* intfc = NULL;
            if (SUCCEEDED(hr = dds_query_interface(instance, riid, &intfc))) {
                if (SUCCEEDED(hr = arr_add_item(self->surfaces, instance))) {
                    if (desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
                        self->primary = instance;
                    }

                    *object = intfc;
                    goto exit;
                }
            }
        }

        dds_release(instance, RELEASE_NONE);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_remove_surface(dd* self, dds* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    const s32 item_count = arr_get_count(self->surfaces);
    for (s32 i = 0; i < item_count; i++) {
        dds* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->surfaces, i, &instance))) {
            if (instance == object) {
                hr = arr_remove_item(self->surfaces, i);
                if (self->primary == object) {
                    self->primary = NULL;
                }

                goto exit;
            }
        }
    }

    hr = DDERR_NOTFOUND;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_create_palette(dd* self, u32 flags, PALETTEENTRY* entries, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if ((flags != DDPCAPS_NONE) && (flags & ~DDPCAPS_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    if (entries == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!self->initialized) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->cooperative_level.flags == DDSCL_NONE) {
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    if (!(flags & (DDPCAPS_1BIT | DDPCAPS_2BIT | DDPCAPS_4BIT | DDPCAPS_8BIT))) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO more validations

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    ddp* instance = NULL;
    if (SUCCEEDED(hr = ddp_create(self->manager, &instance))) {
        if (SUCCEEDED(hr = ddp_initialize(instance, self, flags))) {
            iddp* intfc = NULL;
            if (SUCCEEDED(hr = ddp_query_interface(instance, &IID_IDirectDrawPalette, &intfc))) {
                if (SUCCEEDED(hr = arr_add_item(self->palettes, instance))) {
                    u32 start = 0, count = 0;
                    if (flags & DDPCAPS_1BIT) {
                        count = 1 << 1;
                    }
                    else if (flags & DDPCAPS_2BIT) {
                        count = 1 << 2;
                    }
                    else if (flags & DDPCAPS_4BIT) {
                        count = 1 << 4;
                    }
                    else if (flags & DDPCAPS_8BIT) {
                        count = PALETTE_MAX_ENTRY_COUNT;
                    }

                    // TODO DDPCAPS_ALLOW256
                    // TODO DDPCAPS_8BITENTRIES 

                    if (SUCCEEDED(hr = ddp_set_entries(instance, DDPCAPS_NONE, start, count, entries))) {
                        *object = intfc;
                        goto exit;
                    }
                }
            }
        }

        ddp_release(instance, RELEASE_NONE);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_remove_palette(dd* self, ddp* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    const s32 item_count = arr_get_count(self->palettes);
    for (s32 i = 0; i < item_count; i++) {
        ddp* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->palettes, i, &instance))) {
            if (instance == object) {
                hr = arr_remove_item(self->palettes, i);
                goto exit;
            }
        }
    }

    hr = DDERR_NOTFOUND;

exit:
    LeaveCriticalSection(&self->lock);

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

HRESULT dd_restore_display_mode(dd* self) {
    // TODO
    return DDERR_UNSUPPORTED;
}

HRESULT dd_set_cooperative_level(dd* self, HWND hwnd, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (!self->initialized) {
        return DDERR_NOTINITIALIZED;
    }

    if (flags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)) {
        if (!IsWindow(hwnd)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    // TODO Incomplete.
    // TODO: all kind of validations and checks.
    // Also, the manager has to know what object, if any has exclusive mode.
    // Only one object can have exclusive mode at a time. TODO Tests...

    // TODO. It also seems that the mode cannot be changed if at least one surface or palette
    // was created by DirectDraw instance and still exists. TODO Tests...

    EnterCriticalSection(&self->lock);

    self->cooperative_level.hwnd = hwnd;
    self->cooperative_level.flags = flags;

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT dd_set_display_mode(dd* self, u32 width, u32 height, u32 bpp, u32 rate, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (!(self->cooperative_level.flags & DDSCL_EXCLUSIVE)) {
        return DDERR_NOEXCLUSIVEMODE;
    }

    // TODO: Per documentation - check for locked surfaces, or still drawing...

    // TODO: Read documentation about IDirectDraw7::SetCooperativeLevel interaction with ::SetDisplayMode and ::RestoreDisplayMode

    EnterCriticalSection(&self->lock);

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = sugar_set_display_mode(self->manager, width, height, bpp, rate))) {
        // TODO proper implementation
        if (self->cooperative_level.flags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)) {
            SetWindowPos(self->cooperative_level.hwnd, HWND_TOPMOST, 0, 0, width, height, SWP_SHOWWINDOW);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}
