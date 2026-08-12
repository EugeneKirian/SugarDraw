#include "dd.h"
#include "ddc.h"
#include "ddg.h"
#include "ddp.h"
#include "dds.h"
#include "idd.h"
#include "utilities.h"

static HRESULT dd_can_change_display_mode(dd* self);

static HRESULT dd_acquire_exclusive_mode(dd* self);
static HRESULT dd_unacquire_exclusive_mode(dd* self);

HRESULT dd_create(sugar* manager, const GUID* rclsid, driver* driver, dd** object) {
    if (manager == NULL || rclsid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsEqualGUID(&CLSID_DirectDraw, rclsid)
        && !IsEqualGUID(&CLSID_DirectDraw7, rclsid)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    HRESULT hr = DD_OK;
    dd* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAW, sizeof(dd), &instance))) {
        instance->manager = manager;
        instance->driver = driver;
        CopyMemory(&instance->id, rclsid, sizeof(GUID));
        instance->cooperation.mode.dmSize = sizeof(DEVMODEA);
        if (SUCCEEDED(hr = blitter_create(manager->allocator, &instance->blitter))) {
            if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAW, &instance->interfaces))) {
                if (SUCCEEDED(hr = arr_create(manager->allocator, MEM_TAG_DIRECTDRAW, &instance->clippers))) {
                    if (SUCCEEDED(hr = arr_create(manager->allocator, MEM_TAG_DIRECTDRAW, &instance->palettes))) {
                        if (SUCCEEDED(hr = arr_create(manager->allocator, MEM_TAG_DIRECTDRAW, &instance->surfaces))) {
                            InitializeCriticalSection(&instance->lock);
                            *object = instance;
                            return hr;
                        }

                        arr_release(instance->palettes);
                    }

                    arr_release(instance->clippers);
                }

                intfc_release(instance->interfaces);
            }

            blitter_release(instance->blitter);
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void dd_release(dd* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->graphics != NULL) {
            ddg_release(self->graphics);
        }

        if (self->interfaces != NULL) {
            const u32 item_count = intfc_get_count(self->interfaces);
            for (u32 i = 0; i < item_count; i++) {
                idd* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    idd_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        if (self->surfaces != NULL) {
            const u32 item_count = arr_get_count(self->surfaces);
            for (u32 i = 0; i < item_count; i++) {
                dds* instance = NULL;
                if (SUCCEEDED(arr_get_item(self->surfaces, i, &instance))) {
                    dds_release(instance, RELEASE_NONE);
                }
            }

            arr_release(self->surfaces);
        }

        if (self->palettes != NULL) {
            const u32 item_count = arr_get_count(self->palettes);
            for (u32 i = 0; i < item_count; i++) {
                ddp* instance = NULL;
                if (SUCCEEDED(arr_get_item(self->palettes, i, &instance))) {
                    ddp_release(instance, RELEASE_NONE);
                }
            }

            arr_release(self->palettes);
        }

        if (self->clippers != NULL) {
            const u32 item_count = arr_get_count(self->clippers);
            for (u32 i = 0; i < item_count; i++) {
                ddc* instance = NULL;
                if (SUCCEEDED(arr_get_item(self->clippers, i, &instance))) {
                    ddc_release(instance, RELEASE_NONE);
                }
            }

            arr_release(self->clippers);
        }

        if (self->blitter != NULL) {
            blitter_release(self->blitter);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            sugar_remove_dd(self->manager, self);
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT dd_get_interface(dd* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return intfc_query_item(self->interfaces, riid, object);
}

HRESULT dd_query_interface(dd* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (IsEqualGUID(&IID_IDDVideoPortContainer, riid)) {
        return E_NOINTERFACE;
    }

    HRESULT hr = DD_OK;
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
        || IsEqualGUID(&IID_IDirectDraw3, riid)
        || IsEqualGUID(&IID_IDirectDraw4, riid)
        || IsEqualGUID(&IID_IDirectDraw7, riid)) {
        if (SUCCEEDED(hr = idd_create(self->manager, riid, &instance))) {
            instance->instance = self;
            if (SUCCEEDED(hr = dd_add_ref(self, instance))) {
                *object = instance;
                goto exit;
            }

            idd_release(instance);
            goto exit;
        }
    }

    hr = E_NOINTERFACE;

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

    // TODO Parent and Child Object Lifetimes page in the documentation

    // Note: Earlier versions of the DirectDraw interface (IDirectDraw2 and IDirectDraw, to be exact) behave differently than the most recent interface.
    // When using these early interfaces, DirectDraw automatically releases all child objects when the parent itself is released.
    // As a result, if you use these older interfaces, the order in which you release objects is critical.
    // In this case, you should release the children of a DirectDraw object before releasing the DirectDraw object itself
    // (or not release them at all, counting on the parent to do cleanup for you).
    // Because the DirectDraw object releases the child objects, if you release the parent before the children,
    // you are very likely to incur a memory fault for attempting to dereference a pointer that was invalidated when the parent object released its children. 

    // Some older applications relied on the automatic release of child objects and neglected to properly release some objects when no longer needed.At the time,
    // this practice didn't cause any negative side effects, however doing so when using the IDirectDraw7 interface might result in memory leaks. 

    // TODO Need tests for this, and likely need to track internal object use tracking...

    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            dd_release(self, RELEASE_NOTIFY);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_compact(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->cooperation.flags == DDSCL_NONE) {
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    if (!(self->cooperation.flags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN))) {
        return DDERR_NOEXCLUSIVEMODE;
    }

    return DD_OK;
}

HRESULT dd_create_clipper(dd* self, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    ddc* instance = NULL;
    if (SUCCEEDED(hr = ddc_create(self->manager, &CLSID_DirectDrawClipper, &instance))) {
        if (SUCCEEDED(hr = ddc_initialize(instance, self))) {
            idd* intfc = NULL;
            if (SUCCEEDED(hr = ddc_query_interface(instance, &IID_IDirectDrawClipper, &intfc))) {
                *object = intfc;
                goto exit;
            }
        }

        ddc_release(instance, RELEASE_NONE);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_create_palette(dd* self, u32 flags, PALETTEENTRY* entries, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if ((flags != DDPCAPS_NONE) && (flags & ~DDPCAPS_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    if (entries == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->cooperation.flags == DDSCL_NONE) {
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    if (!(flags & (DDPCAPS_1BIT | DDPCAPS_2BIT | DDPCAPS_4BIT | DDPCAPS_8BIT))) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO more validations

    // TODO DDERR_NOEXCLUSIVEMODE

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

HRESULT dd_create_surface(dd* self, const GUID* riid, DDSURFACEDESC2* desc, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (riid == NULL || desc == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO
    // Unlike the CreateSurface method exposed by the IDirectDraw3 and earlier interfaces, you cannot use IDirectDraw7::CreateSurface
    // to implicitly create a flipping chain of render target surfaces with an attached depth-buffer.
    // The DDSURFACEDESC2 structure that the IDirectDraw7::CreateSurface method accepts doesn't contain a field to specify a depth-buffer bit depth.
    // As a result, applications must create a depth-buffer surface explicitly, then attach it to the back-buffer render target surface. For more information, see Depth Buffers.

    // TODO incomplete validations...
    // TODO move most of validations into separate function, so that it can be reused by ::Initialize

    if (self->cooperation.flags == DDSCL_NONE) {
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
        if ((desc->dwFlags & DDSD_BACKBUFFERCOUNT) && desc->dwBackBufferCount == 0) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (desc->dwFlags & DDSD_BACKBUFFERCOUNT) {
        if (desc->dwBackBufferCount == 0) {
            return DDERR_INVALIDPARAMS;
        }

        if (!(desc->ddsCaps.dwCaps & (DDSCAPS_FLIP | DDSCAPS_COMPLEX))) {
            return DDERR_INVALIDCAPS;
        }
    }

    HRESULT hr = DD_OK;
    if ((desc->dwFlags & DDSD_SIZEFORMAT) != DDSD_SIZEFORMAT) {
        MAKEDDSURFACEDESC2(disp);
        if (SUCCEEDED(hr = ddsurfacedesc2_from_devmodea(&disp, &self->cooperation.mode))) {
            if (!(desc->dwFlags & DDSD_WIDTH)) {
                desc->dwFlags |= DDSD_WIDTH;
                desc->dwWidth = disp.dwWidth;
            }

            if (!(desc->dwFlags & DDSD_HEIGHT)) {
                desc->dwFlags |= DDSD_HEIGHT;
                desc->dwHeight = disp.dwHeight;
            }

            if (!(desc->dwFlags & DDSD_PIXELFORMAT)) {
                desc->dwFlags |= DDSD_PIXELFORMAT;
                CopyMemory(&desc->ddpfPixelFormat, &disp.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
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

        dd* exclusive = NULL;
        if (SUCCEEDED(hr = sugar_get_exclusive(self->manager, &exclusive))) {
            if (exclusive != NULL && self != exclusive) {
                return DDERR_NOEXCLUSIVEMODE;
            }
        }

        if (desc->dwFlags & (DDSD_PITCH | DDSD_LPSURFACE | DDSD_LINEARSIZE | DDSD_FVF)) {
            return DDERR_INVALIDCAPS;
        }

        if (desc->ddsCaps.dwCaps & (DDSCAPS_FLIP | DDSCAPS_COMPLEX)
            && !(self->cooperation.flags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN))) {
            return DDERR_NOEXCLUSIVEMODE;
        }

        desc->ddsCaps.dwCaps |= DDSCAPS_VISIBLE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
        if (desc->ddsCaps.dwCaps & DDSCAPS_FLIP) {
            desc->ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
        }
    }

    if (desc->dwFlags & DDSD_LPSURFACE) {
        desc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;// | DDSCAPS_VIDEOMEMORY; // TODO verify...
    }

    // TODO: For primary surface - get the window background color.
    // DD keeps the window color (or whole image??) as the initial state of the primary surface...

    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddpixelformat_validate(&desc->ddpfPixelFormat))) {
        dds* instance = NULL;
        if (SUCCEEDED(hr = dds_create(self->manager, DDS_NONE, &instance))) {
            if (SUCCEEDED(hr = dds_initialize(instance, riid, self, desc))) {
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
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_duplicate_surface(dd* self, dds* surface, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (surface == NULL || riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self != surface->instance) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO A primary surface, 3D surface, or implicitly
    // created surface cannot be duplicated.

    // the surface memory is shared between the surfaces...

    // TODO DDERR_CANTDUPLICATE

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dd_enum_display_modes(dd* self, u32 flags, DDSURFACEDESC2* desc, ENUMDISPLAYMODESCALLBACK callback, ddedmc* context) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (callback == NULL || context == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO DDEDM_STANDARDVGAMODES 
    // TODO values from DDSURFACEDESC2

    u32 count = 0;
    const DEVMODEA* modes = NULL;
    if (SUCCEEDED(hr = sugar_get_display_modes(self->manager, &count, &modes))) {
        for (u32 i = 0; i < count; i++) {
            MAKEDDSURFACEDESC2(current);
            if (SUCCEEDED(hr = ddsurfacedesc2_from_devmodea(&current, &modes[i]))) {
                if ((flags & DDEDM_REFRESHRATES) && (modes[i].dmFields & DM_DISPLAYFREQUENCY)) {
                    current.dwFlags |= DDSD_REFRESHRATE;
                    current.dwRefreshRate = modes[i].dmDisplayFrequency;
                }

                if (callback(context, &current) == DDENUMRET_OK) {
                    continue;
                }
            }

            goto exit;
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_enum_surfaces(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO
    // If the DDENUMSURFACES_CANBECREATED flag is set, this method attempts
    // to temporarily create a surface that meets the search criterion.
    // When using the DDENUMSURFACES_DOESEXIST flag, an enumerated surface's reference count
    // is incremented—if you are not going to use the surface, be sure to use IDirectDrawSurface7::Release
    // to release it after each enumeration. If you will be using the surface, release it when it is no longer needed.

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dd_flip_to_gdi_surface(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dd_get_caps(dd* self, DDCAPS_DX7* caps) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (caps == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (caps->dwSize != sizeof(DDCAPS_DX7)) {
        return DDERR_INVALIDPARAMS;
    }

    caps->dwCaps = DDCAPS_ALL;
    caps->dwCaps2 = DDCAPS2_ALL;
    caps->dwCKeyCaps = DDCKEYCAPS_ALL;
    caps->dwFXCaps = DDFXCAPS_ALL;
    caps->dwFXAlphaCaps = DDFXALPHACAPS_ALL;
    caps->dwPalCaps = DDPCAPS_ALL;
    caps->dwSVCaps = DDSVCAPS_NONE;
    caps->dwAlphaBltConstBitDepths = DDBD_2 | DDBD_4 | DDBD_8;
    caps->dwAlphaBltPixelBitDepths = DDBD_1 | DDBD_2 | DDBD_4 | DDBD_8;
    caps->dwAlphaBltSurfaceBitDepths = DDBD_1 | DDBD_2 | DDBD_4 | DDBD_8;
    caps->dwAlphaOverlayConstBitDepths = DDBD_2 | DDBD_4 | DDBD_8;
    caps->dwAlphaOverlayPixelBitDepths = DDBD_1 | DDBD_2 | DDBD_4 | DDBD_8;
    caps->dwAlphaOverlaySurfaceBitDepths = DDBD_1 | DDBD_2 | DDBD_4 | DDBD_8;
    caps->dwZBufferBitDepths = DDBD_8 | DDBD_16 | DDBD_24 | DDBD_32;
    caps->dwVidMemTotal = MAX_TOTAL_VIDEO_MEMORY;
    caps->dwVidMemFree = MAX_TOTAL_VIDEO_MEMORY; // TODO Calculate
    caps->dwMaxVisibleOverlays = MAX_VISIBLE_OVERLAY_COUNT;
    caps->dwCurrVisibleOverlays = 0; // TODO Calculate

    // TODO

    // Devices that do not impose limits on stretching or shrinking
    // an overlay destination rectangle often report a minimum and maximum stretch factor of 0.
    caps->dwMinOverlayStretch = 0;
    caps->dwMaxOverlayStretch = 0;

    // TODO proper values

    return DD_OK;
}

HRESULT dd_get_display_mode(dd* self, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    hr = ddsurfacedesc2_from_devmodea(desc, &self->cooperation.mode);

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_get_fourcc_codes(dd* self, u32* count, u32* codes) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (count == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dd_get_gdi_surface(dd* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dd_get_monitor_frequency(dd* self, u32* frequency) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (frequency == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    *frequency = self->cooperation.mode.dmDisplayFrequency;

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_get_scan_line(dd* self, u32* line) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (line == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    u32 status = DDGSTATUS_NONE;
    if (SUCCEEDED(hr = ddg_get_status(self->graphics, &status))) {
        if (status & DDGSTATUS_UPDATING) {
            *line = self->cooperation.mode.dmPelsHeight;
            goto exit;
        }
    }

    *line = 0;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_get_vertical_blank_status(dd* self, bool* status) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (status == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    u32 value = DDGSTATUS_NONE;
    if (SUCCEEDED(hr = ddg_get_status(self->graphics, &value))) {
        *status = !(value & DDGSTATUS_UPDATING);
        goto exit;
    }

    *status = FALSE;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_initialize(dd* self, const GUID* device) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = sugar_get_display_mode(self->manager, &self->cooperation.mode))) {
        ddg* instance = NULL;
        if (SUCCEEDED(hr = ddg_create(self->manager, self->blitter, self->driver, &instance))) {
            if (SUCCEEDED(hr = ddg_initialize(instance, self))) {
                self->graphics = instance;
                goto exit;
            }

            ddg_release(instance);
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_restore_display_mode(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->cooperation.flags == DDSCL_NONE) {
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO DDERR_LOCKEDSURFACES 

    if (SUCCEEDED(hr = dd_can_change_display_mode(self))) {
        if (SUCCEEDED(hr = sugar_restore_display_mode(self->manager))) {
            if (SUCCEEDED(hr = sugar_get_display_mode(self->manager, &self->cooperation.mode))) {
                if (!(self->cooperation.flags & DDSCL_NOWINDOWCHANGES)) {
                    SetWindowPos(self->cooperation.hwnd, HWND_TOPMOST,
                        self->cooperation.rect.left, self->cooperation.rect.top,
                        self->cooperation.rect.right - self->cooperation.rect.left,
                        self->cooperation.rect.bottom - self->cooperation.rect.top, SWP_NOACTIVATE | SWP_NOZORDER);
                }

                hr = ddg_recreate_surface(self->graphics);
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_set_cooperative_level(dd* self, HWND hwnd, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (flags & ~DDSCL_VALID) {
        return DDERR_INVALIDPARAMS;
    }

    if ((flags & DDSCL_FPUSETUP) && (flags & DDSCL_FPUPRESERVE)) {
        return DDERR_INVALIDPARAMS;
    }

    if (!(flags & (DDSCL_EXCLUSIVE | DDSCL_NORMAL)) && !(flags & DDSCL_SETFOCUSWINDOW)) {
        return DDERR_INVALIDPARAMS;
    }

    if ((flags & DDSCL_EXCLUSIVE) && !(flags & DDSCL_FULLSCREEN)) {
        return DDERR_INVALIDPARAMS;
    }

    if ((flags & DDSCL_ALLOWMODEX) && !(flags & DDSCL_FULLSCREEN)) {
        return DDERR_INVALIDPARAMS;
    }

    if ((flags & DDSCL_EXCLUSIVE) && !(flags & DDSCL_CREATEDEVICEWINDOW)) {
        if (hwnd == NULL) {
            return DDERR_INVALIDPARAMS;
        }

        if (!IsWindow(hwnd)) {
            return DDERR_INVALIDPARAMS;
        }

        if (GetWindowLongA(hwnd, GWL_STYLE) & WS_CHILD) {
            return DDERR_HWNDSUBCLASSED;
        }
    }

    if (flags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)) {
        if (!IsWindow(hwnd)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    // TODO DDSCL_ALLOWMODEX
    // TODO DDSCL_DONTHOOKHWND
    // TODO DDSCL_SETFOCUSWINDOW
    // TODO DDSCL_CREATEDEVICEWINDOW
    // TODO DDSCL_SETDEVICEWINDOW

    // TODO DDERR_HWNDALREADYSET

    if (self->cooperation.hwnd == hwnd && self->cooperation.flags == flags) {
        return DD_OK;
    }

    // TODO window hooking, is it needed for full screen?
    
    // TODO window hooking - ERASE primary surface on resize/move:
    // using FillRect(sdc, &rect, (HBRUSH)GetClassLongPtrA(hwnd, GCLP_HBRBACKGROUND)); or window dc?
    // ddoverlap.exe font.exe

    // Restore display mode when user changes the mode from exclusive to normal.
    const bool reset_display_mode = (flags & DDSCL_NORMAL)
        && (self->cooperation.flags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN));

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (flags & DDSCL_NORMAL) {
        if (FAILED(hr = dd_unacquire_exclusive_mode(self))) {
            goto exit;
        }
    }

    if (flags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)) {
        if (FAILED(hr = dd_acquire_exclusive_mode(self))) {
            goto exit;
        }
    }

    self->cooperation.hwnd = hwnd;
    self->cooperation.flags = flags;

    if (self->cooperation.flags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)) {
        if (!(self->cooperation.flags & DDSCL_NOWINDOWCHANGES)) {
            SetWindowPos(self->cooperation.hwnd, HWND_TOPMOST, 0, 0,
                self->cooperation.mode.dmPelsWidth, self->cooperation.mode.dmPelsHeight, SWP_SHOWWINDOW | SWP_NOZORDER);
        }
    }

    if (reset_display_mode) {
        // See Restoring Display Modes in the documentation.
        // This behavior was first offered in the IDirectDraw2 interface,
        // and is offered by all newer versions of the interface.
        if (SUCCEEDED(hr = dd_can_change_display_mode(self))) {
            if (SUCCEEDED(hr = sugar_restore_display_mode(self->manager))) {
                if (SUCCEEDED(hr = sugar_get_display_mode(self->manager, &self->cooperation.mode))) {
                    if (!(self->cooperation.flags & DDSCL_NOWINDOWCHANGES)) {
                        SetWindowPos(self->cooperation.hwnd, HWND_TOPMOST,
                            self->cooperation.rect.left, self->cooperation.rect.top,
                            self->cooperation.rect.right - self->cooperation.rect.left,
                            self->cooperation.rect.bottom - self->cooperation.rect.top, SWP_NOACTIVATE | SWP_NOZORDER);
                    }

                    hr = ddg_recreate_surface(self->graphics);
                }
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_set_display_mode(dd* self, u32 width, u32 height, u32 bpp, u32 rate, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if ((flags != DDSDM_NONE) && (flags & ~DDSDM_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO ModeX

    // TODO: Per documentation - check for locked surfaces, or still drawing...

    // TODO: Read documentation about IDirectDraw7::SetCooperativeLevel interaction with ::SetDisplayMode and ::RestoreDisplayMode

    // TODO multi-monitor suport

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = dd_can_change_display_mode(self))) {
        GetWindowRect(self->cooperation.hwnd, &self->cooperation.rect);
        if (SUCCEEDED(hr = sugar_set_display_mode(self->manager, width, height, bpp, rate))) {
            if (SUCCEEDED(hr = sugar_get_display_mode(self->manager, &self->cooperation.mode))) {
                if (!(self->cooperation.flags & DDSCL_NOWINDOWCHANGES)) {
                    SetWindowPos(self->cooperation.hwnd, HWND_TOPMOST, 0, 0, width, height, SWP_SHOWWINDOW | SWP_NOZORDER);
                }

                hr = ddg_recreate_surface(self->graphics);
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_wait_for_vertical_blank(dd* self, u32 flags, HANDLE event) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (flags == DDWAITVB_NONE || (flags & ~DDWAITVB_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    if (event != NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return (flags & DDWAITVB_BLOCKBEGIN)
        ? ddg_is_ready(self->graphics, TRUE)
        : ddg_is_updating(self->graphics, TRUE);
}

HRESULT dd_get_available_vid_mem(dd* self, DDSCAPS2* caps, u32* total, u32* free) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (caps == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO proper implementation

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dd_get_surface_from_dc(dd* self, HDC hdc, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (hdc == NULL || riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO proper implementation

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dd_restore_all_surfaces(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    const u32 item_count = arr_get_count(self->surfaces);
    for (u32 i = 0; i < item_count; i++) {
        dds* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->surfaces, i, &instance))) {
            if (SUCCEEDED(hr = dds_restore(instance))) {
                continue;
            }
        }

        goto exit;
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_test_cooperative_level(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->cooperation.flags == DDCSCL_NONE) {
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    dd* exclusive = NULL;
    if (SUCCEEDED(hr = sugar_get_exclusive(self->manager, &exclusive))) {
        if (self->cooperation.flags & (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)) {
            EXITCODE(self == exclusive ? DD_OK : DDERR_NOEXCLUSIVEMODE);
        }

        if (self->cooperation.flags & DDSCL_NORMAL) {
            if (exclusive != NULL) {
                EXITCODE(DDERR_EXCLUSIVEMODEALREADYSET);
            }

            MAKEDEVMODEA(mode);
            if (SUCCEEDED(hr = sugar_get_display_mode(self->manager, &mode))) {
                hr = devmodea_equal(&self->cooperation.mode, &mode)
                    ? DD_OK : DDERR_WRONGMODE;
            }
        }
    }

    exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_get_device_identifier(dd* self, DDDEVICEIDENTIFIER2* identifier) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO version from resource

    strcpy_s(identifier->szDriver, MAX_DDDEVICEID_STRING, SUGARDRAW_DEVICE_FILE);
    strcpy_s(identifier->szDescription, MAX_DDDEVICEID_STRING, SUGARDRAW_DEVICE_NAME);

    CopyMemory(&identifier->guidDeviceIdentifier, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    identifier->dwWHQLLevel = 0x7EA0701; // 2026-07-01

    return DD_OK;
}

HRESULT dd_start_mode_test(dd* self, SIZE* modes, u32 count, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validation

    // TODO proper implementation

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dd_evaluate_mode(dd* self, u32 flags, u32* timeout) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validation

    // TODO proper implementation

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dd_set_driver(dd* self, driver* driver) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (driver == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddg_set_driver(self->graphics, driver))) {
        self->driver = driver;
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_attach_clipper(dd* self, ddc* clipper) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (clipper == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    hr = arr_add_item(self->clippers, clipper);

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_remove_clipper(dd* self, ddc* clipper) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (clipper == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    hr = arr_remove_item(self->clippers, clipper);

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_remove_palette(dd* self, ddp* palette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (palette == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    hr = arr_remove_item(self->palettes, palette);

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_remove_surface(dd* self, dds* surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = arr_remove_item(self->surfaces, surface))) {
        if (self->primary == surface) {
            self->primary = NULL;
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_lose_all_surfaces(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    const u32 item_count = arr_get_count(self->surfaces);
    for (u32 i = 0; i < item_count; i++) {
        dds* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->surfaces, i, &instance))) {
            if (SUCCEEDED(hr = dds_set_lost(instance))) {
                continue;
            }
        }

        goto exit;
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dd_can_change_display_mode(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    dd* exclusive = NULL;
    if (SUCCEEDED(hr = sugar_get_exclusive(self->manager, &exclusive))) {
        if (exclusive != NULL && self != exclusive) {
            return DDERR_NOEXCLUSIVEMODE;
        }
    }

    return hr;
}

HRESULT dd_acquire_exclusive_mode(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    dd* exclusive = NULL;
    if (SUCCEEDED(hr = sugar_get_exclusive(self->manager, &exclusive))) {
        if (self == exclusive) {
            return DD_OK;
        }

        return exclusive == NULL
            ? sugar_set_exclusive(self->manager, self)
            : DDERR_EXCLUSIVEMODEALREADYSET;
    }

    return hr;
}

HRESULT dd_unacquire_exclusive_mode(dd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    dd* exclusive = NULL;
    if (SUCCEEDED(hr = sugar_get_exclusive(self->manager, &exclusive))) {
        return exclusive == NULL || self == exclusive
            ? sugar_set_exclusive(self->manager, NULL)
            : DDERR_NOEXCLUSIVEMODE;
    }

    return hr;
}
