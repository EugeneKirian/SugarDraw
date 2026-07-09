#include "dd.h"
#include "ddp.h"
#include "dds.h"
#include "iddp.h"
#include "idds.h"
#include "utilities.h"

HRESULT dds_create(sugar* manager, dds** object) {
    if (manager == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    dds* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWSURFACE, sizeof(dds), &instance))) {
        instance->manager = manager;
        if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWSURFACE, sizeof(ddsinfo), &instance->surface))) {
            if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAWSURFACE, &instance->interfaces))) {
                if (SUCCEEDED(hr = arr_create(manager->allocator, MEM_TAG_DIRECTDRAWSURFACE, &instance->attachments))) {
                    InitializeCriticalSection(&instance->lock);
                    *object = instance;
                    return hr;
                }

                intfc_release(instance->interfaces);
            }

            allocator_free(manager->allocator, instance->surface);
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void dds_release(dds* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const int count = intfc_get_count(self->interfaces);
            for (int i = 0; i < count; i++) {
                idds* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    idds_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        if (self->surface != NULL) {
            // TODO bitmap, mapping, hdc

            if (self->surface->info != NULL) {
                allocator_free(self->manager->allocator, self->surface->info);
            }

            allocator_free(self->manager->allocator, self->surface);
        }

        // TODO attachments

        if (self->palette != NULL) {
            iddp_remove_ref(self->palette);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            dd_remove_surface(self->instance, self);
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT dds_query_interface(dds* self, const GUID* riid, void** object) {
    HRESULT hr = E_NOINTERFACE;
    EnterCriticalSection(&self->lock);

    idds* instance = NULL;
    if (SUCCEEDED(hr = intfc_query_item(self->interfaces, riid, &instance))) {
        idds_add_ref(instance);
        *object = instance;
        goto exit;
    }

    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IDirectDrawSurface, riid)
        || IsEqualGUID(&IID_IDirectDrawSurface2, riid)
        || IsEqualGUID(&IID_IDirectDrawSurface3, riid)
        || IsEqualGUID(&IID_IDirectDrawSurface4, riid)
        || IsEqualGUID(&IID_IDirectDrawSurface7, riid)) {
        if (SUCCEEDED(hr = idds_create(self->manager, riid, &instance))) {
            instance->instance = self;
            if (SUCCEEDED(hr = dds_add_ref(self, instance))) {
                *object = instance;
                goto exit;
            }

            idds_release(instance);
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_add_ref(dds* self, idds* object) {
    return intfc_add_item(self->interfaces, &object->id, object);
}

HRESULT dds_remove_ref(dds* self, idds* object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            dds_release(self, RELEASE_NOTIFY);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_blt_fast(dds* self, u32 x, u32 y, dds* surface, RECT* rect, u32 transfer) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->clipper != NULL) {
        return DDERR_UNSUPPORTED;
    }

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        || !(surface->desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)) {
        return DDERR_UNSUPPORTED; // TODO verify
    }

    if ((transfer != DDBLTFAST_NONE) && (transfer & ~DDBLTFAST_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO proper implementation
    // TODO: for now handle the most simplest cases...

    if (transfer != DDBLTFAST_NOCOLORKEY) {
        return DDERR_UNSUPPORTED; // TODO
    }

    if (self->desc.ddpfPixelFormat.dwRGBBitCount != surface->desc.ddpfPixelFormat.dwRGBBitCount) {
        return DDERR_UNSUPPORTED; // TODO
    }

    s32 dst_x = x;
    s32 dst_y = y;
    s32 dst_w = (s32)self->desc.dwWidth;
    s32 dst_h = (s32)self->desc.dwHeight;

    // Check if the destination starting point (x,y) is within the surface boundaries.
    if (dst_w < dst_x || dst_h < dst_y) {
        return DDERR_INVALIDPARAMS;
    }

    s32 src_x = rect == NULL ? 0 : rect->left;
    s32 src_y = rect == NULL ? 0 : rect->top;
    s32 src_w = rect == NULL ? (s32)surface->desc.dwWidth : rect->right - rect->top;
    s32 src_h = rect == NULL ? (s32)surface->desc.dwHeight : rect->bottom - rect->bottom;

    // Clip the source rectangle to the source surface boundaries.
    if (src_x < 0) {
        src_w += src_x; src_x = 0;
    }

    if (src_y < 0) {
        src_h += src_y; src_y = 0;
    }

    if (src_w < 0 || src_h < 0) {
        return DDERR_INVALIDPARAMS;
    }

    if ((s32)surface->desc.dwWidth < src_w) {
        src_w = surface->desc.dwWidth;
    }

    if ((s32)surface->desc.dwHeight < src_h) {
        src_h = surface->desc.dwHeight;
    }

    // Adjust destination rectangle width and height to source rectangle width and height.
    if (src_w < dst_w) {
        src_w = dst_w;
    }

    if (src_h < dst_h) {
        src_h = dst_h;
    }

    // TODO DDERR_INVALIDRECT

    // TODO lock checks DDERR_SURFACEBUSY and DDERR_LOCKEDSURFACES

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO proper blitting
    // TODO handle case when source and destination surfaces are the same...
    for (int i = 0; i < src_h - src_y; i++) {
        const u8* src = (surface->surface->data
            + (i + src_y) * surface->surface->stride + src_x * surface->desc.ddpfPixelFormat.dwRGBBitCount / 8);
        u8* dst = (self->surface->data
            + (i + dst_y) * self->surface->stride + dst_x * self->desc.ddpfPixelFormat.dwRGBBitCount / 8);
        CopyMemory(dst, src, (dst_w - dst_x) * self->desc.ddpfPixelFormat.dwRGBBitCount / 8);
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
        if (self->desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) {
            // TODO blit is done into the non-flippable front buffer (i.e. windowed mode)

            // TODO proper implementation
            //HWND wnd = self->instance->cooperative_level.hwnd;
            //HDC hdc = GetDC(wnd);
            //RECT rect;
            //GetClientRect(wnd, &rect);
            //BitBlt(hdc, 0, 0, rect.right, rect.bottom, self->surface->hdc, 0, 0, SRCCOPY);
            //ReleaseDC(wnd, hdc);
            //InvalidateRect(wnd, &rect, FALSE);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_flip(dds* self, dds* override, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validate flags

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_FLIP)) {
        return DDERR_NOTFLIPPABLE;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO check locks DDERR_SURFACEBUSY 

    dds* target = NULL;
    if (override != NULL) {
        // Validate that the override surface is one of the back buffers.
        const int item_count = arr_get_count(self->attachments);
        for (int i = 0; i < item_count; i++) {
            dds* instance = NULL;
            if (SUCCEEDED(arr_get_item(self->attachments, i, &instance))) {
                if (instance == override) {
                    target = override;
                    break;
                }
            }
        }

        if (target == NULL) {
            hr = DDERR_INVALIDPARAMS;
            goto exit;
        }
    }
    else {
        // Search for the first available back buffer.
        const int item_count = arr_get_count(self->attachments);
        for (int i = 0; i < item_count; i++) {
            dds* instance = NULL;
            if (SUCCEEDED(arr_get_item(self->attachments, i, &instance))) {
                if (instance->desc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) {
                    target = instance;
                    break;
                }
            }
        }

        // TODO: what is the correct behavior? Need tests!
    }

    if (target == NULL) {
        hr = DDERR_NOTFLIPPABLE;
        goto exit;
    }

    target->surface = InterlockedExchangePointer(&self->surface, target->surface);

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
        // TODO proper implementation
        HWND wnd = self->instance->cooperative_level.hwnd;
        HDC hdc = GetDC(wnd);
        RECT rect;
        GetClientRect(wnd, &rect);
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, self->surface->dc, 0, 0, SRCCOPY);
        ReleaseDC(wnd, hdc);
        InvalidateRect(wnd, &rect, FALSE);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_get_attached_surface(dds* self, DDSCAPS2* caps, dds** surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (caps == NULL || surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (caps->dwCaps == DDSCAPS_NONE && caps->dwCaps2 == DDSCAPS2_NONE
        && caps->dwCaps3 == DDSCAPS3_NONE && caps->dwCaps4 == DDSCAPS4_NONE) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    dds* match = NULL;
    const int item_count = arr_get_count(self->attachments);
    for (int i = 0; i < item_count; i++) {
        dds* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->attachments, i, &instance))) {
            // TODO proper search
            bool criteria = TRUE;
            if (caps->dwCaps != DDSCAPS_NONE) {
                criteria = criteria && (instance->desc.ddsCaps.dwCaps & caps->dwCaps);
            }

            if (caps->dwCaps2 != DDSCAPS2_NONE) {
                criteria = criteria && (instance->desc.ddsCaps.dwCaps2 & caps->dwCaps2);
            }

            if (caps->dwCaps3 != DDSCAPS3_NONE) {
                criteria = criteria && (instance->desc.ddsCaps.dwCaps3 & caps->dwCaps3);
            }

            if (caps->dwCaps4 != DDSCAPS4_NONE) {
                criteria = criteria && (instance->desc.ddsCaps.dwCaps4 & caps->dwCaps4);
            }

            if (criteria) {
                if (match == NULL) {
                    match = instance;
                }
                else {
                    hr = DDERR_INVALIDPARAMS; // TODO correct error code. Multiple matches are not allowed?
                    goto exit;
                }
            }
        }
    }

    if (match != NULL) {
        *surface = match;
    }
    else {
        hr = DDERR_NOTFOUND;
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_get_color_key(dds* self, u32 flags, DDCOLORKEY* key) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (flags & DDCKEY_DESTBLT) {
        if (self->colors & DDCKEY_DESTBLT) {
            CopyMemory(key, &self->desc.ddckCKDestBlt, sizeof(DDCOLORKEY));
            return DD_OK;
        }

        return DDERR_NOCOLORKEY;
    }

    if (flags & DDCKEY_DESTOVERLAY) {
        if (self->colors & DDCKEY_DESTOVERLAY) {
            CopyMemory(key, &self->desc.ddckCKDestOverlay, sizeof(DDCOLORKEY));
            return DD_OK;
        }

        return DDERR_NOCOLORKEY;
    }

    if (flags & DDCKEY_SRCBLT) {
        if (self->colors & DDCKEY_SRCBLT) {
            CopyMemory(key, &self->desc.ddckCKSrcBlt, sizeof(DDCOLORKEY));
            return DD_OK;
        }

        return DDERR_NOCOLORKEY;
    }

    if (flags & DDCKEY_SRCOVERLAY) {
        if (self->colors & DDCKEY_SRCOVERLAY) {
            CopyMemory(key, &self->desc.ddckCKSrcOverlay, sizeof(DDCOLORKEY));
            return DD_OK;
        }

        return DDERR_NOCOLORKEY;
    }

    return DDERR_NOCOLORKEY;
}

HRESULT dds_get_dc(dds* self, HDC* hdc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (hdc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->desc.dwFlags & DDSD_LPSURFACE) {
        return DDERR_UNSUPPORTED;
    }

    if (self->surface->exposed) {
        return DDERR_DCALREADYCREATED;
    }

    // TODO validate surface bit count (1,2,4,8) and normal argb color bit masks for (15,16,24,32)
    // TODO DDLOCK_READONLY

    // TODO lock surface

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    self->surface->exposed = TRUE;

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_PALETTE) {
        // It is not explicitly stated in the documentation,
        // however, palettized surfaces that have no explicitly attached palette
        // must use the palette of primary surface, if it exists.

        // TODO support indexed palettes
        iddp* palette = self->palette != NULL
            ? self->palette
            : (self->instance->primary == NULL ? NULL : self->instance->primary->palette);

        if (palette != NULL) {
            // TODO what about 1,2,4,8 bits of palette caps?
            const u32 count = 1 << self->desc.ddpfPixelFormat.dwRGBBitCount;
            SetDIBColorTable(self->surface->dc, 0, count, palette->instance->quads);
        }
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_OWNDC) {
        self->surface->checkpoint = SaveDC(self->surface->dc);
    }

    *hdc = self->surface->dc;

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_get_palette(dds* self, iddp** palette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (palette == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->palette == NULL) {
        return DDERR_NOPALETTEATTACHED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = iddp_add_ref(self->palette))) {
        *palette = self->palette;
    }

    // TODO DDERR_NOEXCLUSIVEMODE

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_get_surface_desc(dds* self, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (desc->dwFlags & DDSD_WIDTH) {
        desc->dwWidth = self->desc.dwWidth;
    }

    if (desc->dwFlags & DDSD_HEIGHT) {
        desc->dwHeight = self->desc.dwHeight;
    }

    // TODO other flags!

    return DD_OK;
}

HRESULT dds_initialize(dds* self, dd* object, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL || desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    self->instance = object;
    CopyMemory(&self->desc, desc, sizeof(DDSURFACEDESC2));

    // TODO: use validation rules from dds_create (refactoring needed).

    // TODO handle custom buffer, i.e. DDSD_LPSURFACE from DD4/7

    if (self->desc.dwFlags & DDSD_LPSURFACE) {
        // TODO not implemented - custom memory...
        hr = DDERR_UNSUPPORTED;
        goto exit;
    }
    else {
        self->surface->dc = CreateCompatibleDC(NULL); // TODO: From where? exclusive mode hwnd, or cliper?
        if (self->surface->dc == NULL) {
            hr = DDERR_OUTOFMEMORY;
            goto exit;
        }

        const u32 bpp = self->desc.ddpfPixelFormat.dwRGBBitCount;
        const u32 stride = ((self->desc.dwWidth * bpp + 63) & ~63) >> 3; // TODO
        const u32 size = stride * self->desc.dwHeight;
        const u32 aligned_width = stride / (bpp / 8); // TODO

        if (SUCCEEDED(hr = allocator_allocate(self->manager->allocator,
            MEM_TAG_DIRECTDRAWSURFACE, sizeof(ddsbmp), &self->surface->info))) {
            BITMAPINFOHEADER* header = &self->surface->info->header;
            header->biSize = sizeof(BITMAPINFOHEADER);
            header->biWidth = (LONG)aligned_width;
            header->biHeight = -(LONG)self->desc.dwHeight;
            header->biPlanes = 1;
            header->biBitCount = bpp;
            header->biCompression = (bpp == 15 || bpp == 16) ? BI_BITFIELDS : BI_RGB;

            switch (bpp) {
            case 1:
            case 2:
            case 4: {
                hr = DDERR_UNSUPPORTED;
                allocator_free(self->manager->allocator, self->surface->info);
                goto exit;
            }break;
            case 8: {
                header->biClrUsed = PALETTE_MAX_ENTRY_COUNT;
                RGBQUAD* palette = self->surface->info->palette;

                bool is_set = FALSE;
                if (GetDeviceCaps(self->surface->dc, RASTERCAPS) & RC_PALETTE) {
                    PALETTEENTRY entries[PALETTE_MAX_ENTRY_COUNT];
                    is_set = GetSystemPaletteEntries(self->surface->dc,
                        0, PALETTE_MAX_ENTRY_COUNT, entries) == PALETTE_MAX_ENTRY_COUNT;

                    if (is_set) {
                        palette_entry_to_rgb_quad(entries, PALETTE_MAX_ENTRY_COUNT, palette);
                    }
                }
                
                if (!is_set) {
                    // Default palette to gray scale.
                    for (int i = 0; i < PALETTE_MAX_ENTRY_COUNT; i++) {
                        palette[i].rgbRed = i;
                        palette[i].rgbGreen = i;
                        palette[i].rgbBlue = i;
                    }
                }
            }break;
            case 15: {
                header->biClrUsed = 3;
                ((DWORD*)self->surface->info->palette)[0] = 0x7C00;
                ((DWORD*)self->surface->info->palette)[1] = 0x03E0;
                ((DWORD*)self->surface->info->palette)[2] = 0x001F;
            }break;
            case 16: {
                header->biClrUsed = 3;
                ((DWORD*)self->surface->info->palette)[0] = 0xF800;
                ((DWORD*)self->surface->info->palette)[1] = 0x07E0;
                ((DWORD*)self->surface->info->palette)[2] = 0x001F;
            }break;
            case 32:
            case 24: {
                // TODO
            }break;
            }

            header->biSizeImage = ((aligned_width * bpp + 63) & ~63) / 8 * self->desc.dwHeight; // TODO

            self->surface->mapping = CreateFileMappingA(INVALID_HANDLE_VALUE,
                NULL, PAGE_READWRITE | SEC_COMMIT, 0, size, NULL);
            if (self->surface->mapping == NULL) {
                // TODO clean-up...
                hr = DDERR_OUTOFMEMORY;
                goto exit;
            }

            self->surface->stride = stride;
            self->surface->bitmap = CreateDIBSection(self->surface->dc,
                (BITMAPINFO*)self->surface->info, DIB_RGB_COLORS,
                &self->surface->data, self->surface->mapping, 0);
            if (self->surface->bitmap == NULL) {
                // TODO clean-up...
                hr = DDERR_OUTOFMEMORY;
                goto exit;
            }

            SelectObject(self->surface->dc, self->surface->bitmap);
        }
    }

    if (self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT)) {
        u32 flags = DDPCAPS_NONE;

        switch (self->desc.ddpfPixelFormat.dwRGBBitCount) {
        case 1:
        case 2:
        case 4: {
            // TODO clean-up...
            hr = DDERR_UNSUPPORTED;
            goto exit;
        }break;
        case 8: {
            flags = DDPCAPS_8BIT | DDPCAPS_ALLOW256;
            //if (self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
            //    flags |= DDPCAPS_PRIMARYSURFACE;
            //}
            //if (self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACELEFT) {
            //    flags |= DDPCAPS_PRIMARYSURFACELEFT;
            //}

            // TODO handle non-HDC surfaces...
            const u32 count = 1 << self->desc.ddpfPixelFormat.dwRGBBitCount;
            PALETTEENTRY entries[PALETTE_MAX_ENTRY_COUNT];
            if (SUCCEEDED(hr = rgb_quad_to_palette_entry(self->surface->info->palette, count, entries))) {
                if (SUCCEEDED(hr = dd_create_palette(self->instance, flags, entries, &self->palette))) {
                    ddp_register_surface(self->palette->instance, self);
                    dds_update_palette_entries(self);
                }
            }
        }break;
        }
    }

    if (self->desc.dwFlags & DDSD_BACKBUFFERCOUNT) {
        DDSURFACEDESC2 back;
        CopyMemory(&back, &self->desc, sizeof(DDSURFACEDESC2));

        // TODO properly set values.
        back.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
        back.dwBackBufferCount = 0;

        for (u32 i = 0; i < self->desc.dwBackBufferCount; i++) {
            dds* instance = NULL;
            // TODO. Should this be done using dd_create_surface?
            // Should the back buffers to be enumeratable as surfaces directly via dd_enumerate_surfaces?
            if (SUCCEEDED(hr = dds_create(self->manager, &instance))) {
                if (SUCCEEDED(hr = dds_initialize(instance, self->instance, &back))) {
                    idds* intfc = NULL;
                    if (SUCCEEDED(hr = dds_query_interface(instance, &IID_IDirectDrawSurface, &intfc))) {
                        if (SUCCEEDED(hr = arr_add_item(self->attachments, instance))) {
                            continue;
                        }
                    }

                    dds_release(instance, RELEASE_NONE);
                }
            }

            if (FAILED(hr)) {
                // TODO clean-up - unitialize surface (instance, desc, clear memory, hdc, etc)
                goto exit;
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_lock(dds* self, RECT* rect, DDSURFACEDESC2* desc, u32 flags) {
    // TODO lock and unlock when GetDC and ReleaseDC
    return DDERR_UNSUPPORTED;
}

HRESULT dds_release_dc(dds* self, HDC hdc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (hdc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->desc.dwFlags & DDSD_LPSURFACE) {
        return DDERR_UNSUPPORTED;
    }

    if (!self->surface->exposed) {
        return DDERR_INVALIDPARAMS; // TODO proper error
    }

    if (self->surface->dc != hdc) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);

    self->surface->exposed = FALSE;

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_OWNDC) {
        RestoreDC(self->surface->dc, self->surface->checkpoint);
    }

    // TODO unlock surface

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT dds_set_color_key(dds* self, u32 flags, DDCOLORKEY* key) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) {
        return DDERR_INVALIDSURFACETYPE;
    }

    if (key == NULL) {
        if (flags & DDCKEY_DESTBLT) {
            self->colors &= ~DDCKEY_DESTBLT;
        }

        if (flags & DDCKEY_DESTOVERLAY) {
            self->colors &= ~DDCKEY_DESTOVERLAY;
        }

        if (flags & DDCKEY_SRCBLT) {
            self->colors &= ~DDCKEY_SRCBLT;
        }

        if (flags & DDCKEY_SRCOVERLAY) {
            self->colors &= DDCKEY_SRCOVERLAY;
        }
    }
    else {
        DDCOLORKEY color;
        color.dwColorSpaceLowValue = key->dwColorSpaceLowValue;
        color.dwColorSpaceHighValue = key->dwColorSpaceLowValue;

        if (flags & DDCKEY_COLORSPACE) {
            color.dwColorSpaceHighValue = key->dwColorSpaceHighValue;
        }

        if (flags & DDCKEY_DESTBLT) {
            self->colors |= DDCKEY_DESTBLT;
            CopyMemory(&self->desc.ddckCKDestBlt, &color, sizeof(DDCOLORKEY));
        }

        if (flags & DDCKEY_DESTOVERLAY) {
            self->colors |= DDCKEY_DESTOVERLAY;
            CopyMemory(&self->desc.ddckCKDestOverlay, &color, sizeof(DDCOLORKEY));
        }

        if (flags & DDCKEY_SRCBLT) {
            self->colors |= DDCKEY_SRCBLT;
            CopyMemory(&self->desc.ddckCKSrcBlt, &color, sizeof(DDCOLORKEY));
        }

        if (flags & DDCKEY_SRCOVERLAY) {
            self->colors |= DDCKEY_SRCOVERLAY;
            CopyMemory(&self->desc.ddckCKSrcOverlay, &color, sizeof(DDCOLORKEY));
        }
    }

    return DD_OK;
}

HRESULT dds_set_palette(dds* self, iddp* palette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_PALETTE)) {
        return DDERR_NOTPALETTIZED; // TODO verify error code
    }

    if (palette->instance->caps & DDPCAPS_ALPHA) {
        if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_TEXTURE)) {
            return DDERR_INVALIDSURFACETYPE;
        }
    }

    switch (self->desc.ddpfPixelFormat.dwRGBBitCount) {
    case 1:
    case 2:
    case 4: {
        return DDERR_UNSUPPORTED; // TODO
    }break;
    case 8: {
        if (!(palette->instance->caps & DDPCAPS_8BIT)) {
            return DDERR_NOT8BITCOLOR;
        }
    }break;
    }

    // TODO Other checks...
    // TODO DDERR_NOT8BITCOLOR 

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);
    if (palette == NULL) {
        if (self->palette != NULL) {
            ddp_unregister_surface(self->palette->instance, self);
            iddp_remove_ref(self->palette);
        }
    }
    else {
        if (self->palette == palette) {
            goto exit;
        }

        ddp_unregister_surface(self->palette->instance, self);
        iddp_remove_ref(self->palette);
    }

    self->palette = palette;

    if (self->palette != NULL) {
        iddp_add_ref(self->palette);
        ddp_register_surface(self->palette->instance, self);
        dds_update_palette_entries(self);
    }

    // TODO: how to handle backbuffers? propagate the palette? Need tests...
    // What if backbuffer has it's own palete... Is it posible?

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
        const int item_count = arr_get_count(self->attachments);
        for (int i = 0; i < item_count; i++) {
            dds* instance = NULL;
            if (SUCCEEDED(arr_get_item(self->attachments, i, &instance))) {
                if (instance->desc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) {
                    dds_set_palette(instance, palette);
                }
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_remove_palette(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    self->palette = NULL;

    return DD_OK;
}

HRESULT dds_update_palette_entries(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->palette != NULL) {
        if (self->surface->dc != NULL) {
            // TODO what about 1,2,4,8 bits of palette caps?
            const u32 count = 1 << self->desc.ddpfPixelFormat.dwRGBBitCount;
            SetDIBColorTable(self->surface->dc, 0, count, self->palette->instance->quads);

            // TODO update bm info?
        }
    }

    // TODO what to do for custom memory that does not have HDC?

    return DD_OK;
}
