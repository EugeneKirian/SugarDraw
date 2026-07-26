#include "blt.h"
#include "ddsd.h"
#include "lock.h"
#include "utilities.h"

typedef struct header {
    BITMAPINFOHEADER    info;
    RGBQUAD             palette[PALETTE_MAX_ENTRY_COUNT];
} header;

typedef struct bitmap {
    HDC                 dc;
    header              header;
    HANDLE              mapping;
    HBITMAP             bitmap;
    s32                 checkpoint;
} bitmap;

typedef struct ddsd {
    allocator*          allocator;
    s32                 refs;
    bool                exposed;
    DDSURFACEDESC2      desc;
    CRITICAL_SECTION    lock;
    lock*               locks;
    bitmap              bitmap;
    u8*                 data;
    s32                 pages;
    u32                 uniqueness;
} ddsd;

HRESULT ddsd_create(allocator* allocator, ddsd** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    ddsd* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_DIRECTDRAWSURFACEDATA, sizeof(ddsd), &instance))) {
        instance->allocator = allocator;
        if (SUCCEEDED(hr = lock_create(allocator, MEM_TAG_DIRECTDRAWSURFACEDATA, &instance->locks))) {
            InitializeCriticalSection(&instance->lock);
            instance->refs = 1;
            
            *object = instance;

            return hr;
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void ddsd_release(ddsd* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (!(self->desc.dwFlags & DDSD_LPSURFACE)) {
            // TODO hdc, hbitmap, maping

            if (self->data != NULL) {
                allocator_free(self->allocator, self->data);
            }
        }

        if (self->locks != NULL) {
            lock_release(self->locks);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self);
    }
}

ULONG ddsd_add_ref(ddsd* self) {
    if (self == NULL) {
        return 0;
    }

    return InterlockedIncrement(&self->refs);
}

ULONG ddsd_remove_ref(ddsd* self) {
    if (self == NULL || self->refs == 0) {
        return 0;
    }

    s32 result = InterlockedDecrement(&self->refs);

    if ((result = max(result, 0)) == 0) {
        self->refs = 0;
        ddsd_release(self);
    }

    return result;
}

HRESULT ddsd_initialize(ddsd* self, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->data != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO all kinds of verifications

    self->uniqueness++;
    CopyMemory(&self->desc, desc, sizeof(DDSURFACEDESC2));

    if (self->desc.dwFlags & DDSD_LPSURFACE) {
        self->data = desc->lpSurface;
    }
    else {
        // TODO go here only for DC-supporting formats
        // TODO handle cases for YUV, etc formats which are not DC-supported?

        // TODO: From where? exclusive mode hwnd, or cliper?
        self->bitmap.dc = CreateCompatibleDC(NULL);
        if (self->bitmap.dc == NULL) {
            hr = DDERR_OUTOFMEMORY;
            goto exit;
        }

        // TODO make stride aligned to a reasonable value for SIMD processing

        const u32 bpp = self->desc.ddpfPixelFormat.dwRGBBitCount;
        const u32 stride = ((self->desc.dwWidth * bpp + 63) & ~63) >> 3; // TODO
        const u32 size = stride * self->desc.dwHeight;
        const u32 aligned_width = stride / (bpp / 8); // TODO

        BITMAPINFOHEADER* header = &self->bitmap.header.info;
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
            goto exit;
        }break;
        case 8: {
            header->biClrUsed = PALETTE_MAX_ENTRY_COUNT;
            RGBQUAD* palette = self->bitmap.header.palette;

            bool is_set = FALSE;
            if (GetDeviceCaps(self->bitmap.dc, RASTERCAPS) & RC_PALETTE) {
                PALETTEENTRY entries[PALETTE_MAX_ENTRY_COUNT];
                is_set = GetSystemPaletteEntries(self->bitmap.dc,
                    0, PALETTE_MAX_ENTRY_COUNT, entries) == PALETTE_MAX_ENTRY_COUNT;

                if (is_set) {
                    palette_entry_to_rgb_quad(entries, PALETTE_MAX_ENTRY_COUNT, palette);
                }
            }

            if (!is_set) {
                // Default palette to gray scale.
                for (s32 i = 0; i < PALETTE_MAX_ENTRY_COUNT; i++) {
                    palette[i].rgbRed = i;
                    palette[i].rgbGreen = i;
                    palette[i].rgbBlue = i;
                }
            }
        }break;
        case 15:
        case 16: {
            header->biClrUsed = 3;
            ((DWORD*)self->bitmap.header.palette)[0]
                = self->desc.ddpfPixelFormat.dwRBitMask;
            ((DWORD*)self->bitmap.header.palette)[1]
                = self->desc.ddpfPixelFormat.dwGBitMask;
            ((DWORD*)self->bitmap.header.palette)[2]
                = self->desc.ddpfPixelFormat.dwBBitMask;
        }break;
               // TODO
               //case 24:
               //case 32: {
               //    ((DWORD*)self->bitmap.header.palette)[0]
               //        = self->desc.ddpfPixelFormat.dwRBitMask;
               //    ((DWORD*)self->bitmap.header.palette)[1]
               //        = self->desc.ddpfPixelFormat.dwGBitMask;
               //    ((DWORD*)self->bitmap.header.palette)[2]
               //        = self->desc.ddpfPixelFormat.dwBBitMask;
               //}break;
        }

        header->biSizeImage = ((aligned_width * bpp + 63) & ~63) / 8 * self->desc.dwHeight; // TODO

        self->bitmap.mapping = CreateFileMappingA(INVALID_HANDLE_VALUE,
            NULL, PAGE_READWRITE | SEC_COMMIT, 0, size, NULL);
        if (self->bitmap.mapping == NULL) {
            // TODO clean-up...
            hr = DDERR_OUTOFMEMORY;
            goto exit;
        }

        self->desc.lPitch = stride;
        self->bitmap.bitmap = CreateDIBSection(self->bitmap.dc,
            (BITMAPINFO*)&self->bitmap.header.info, DIB_RGB_COLORS,
            &self->data, self->bitmap.mapping, 0);
        if (self->bitmap.bitmap == NULL) {
            // TODO clean-up...
            hr = DDERR_OUTOFMEMORY;
            goto exit;
        }

        SelectObject(self->bitmap.dc, self->bitmap.bitmap);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_blt_fast(ddsd* self, RECT* dst, ddsd* surface, RECT* src, u32 transfer) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (dst == NULL || surface == NULL || src == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (transfer != DDBLTFAST_NONE && (transfer & ~DDBLTFAST_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO DDERR_OVERLAPPINGRECTS overlapping rects?
    // TODO DDERR_COLORKEYNOTSET

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO better way to acquire multiple locks, and unwind easily...
    if (SUCCEEDED(hr = ddsd_lock_rect(self, dst))) {
        if (FAILED(hr = ddsd_lock_rect(surface, src))) {
            ddsd_unlock_rect(self, dst);
            hr = DDERR_LOCKEDSURFACES;
            goto exit;
        }

        if (transfer & DDBLTFAST_SRCCOLORKEY) {
            blt_src_color_key(self->data, (u32)dst->left, (u32)dst->top, (u32)dst->right, (u32)dst->bottom,
                self->desc.ddpfPixelFormat.dwRGBBitCount, self->desc.lPitch,
                surface->data, (u32)src->left, (u32)src->top, (u32)src->right, (u32)src->bottom,
                surface->desc.ddpfPixelFormat.dwRGBBitCount, surface->desc.lPitch,
                surface->desc.ddckCKSrcBlt.dwColorSpaceLowValue,
                surface->desc.ddckCKSrcBlt.dwColorSpaceHighValue);
        }
        else if (transfer & DDBLTFAST_DESTCOLORKEY) {
            blt_dst_color_key(self->data, (u32)dst->left, (u32)dst->top, (u32)dst->right, (u32)dst->bottom,
                self->desc.ddpfPixelFormat.dwRGBBitCount, self->desc.lPitch,
                surface->data, (u32)src->left, (u32)src->top, (u32)src->right, (u32)src->bottom,
                surface->desc.ddpfPixelFormat.dwRGBBitCount, surface->desc.lPitch,
                self->desc.ddckCKDestBlt.dwColorSpaceLowValue,
                self->desc.ddckCKDestBlt.dwColorSpaceHighValue);
        }
        else {
            blt_blit(self->data, (u32)dst->left, (u32)dst->top, (u32)dst->right, (u32)dst->bottom,
                &self->desc.ddpfPixelFormat, self->desc.lPitch,
                self->bitmap.header.palette,
                surface->data, (u32)src->left, (u32)src->top, (u32)src->right, (u32)src->bottom,
                &surface->desc.ddpfPixelFormat, surface->desc.lPitch,
                surface->bitmap.header.palette);
        }

        self->uniqueness++;

        ddsd_unlock_rect(surface, src);
        ddsd_unlock_rect(self, dst);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_get_palette(ddsd* self, u32 base, u32 count, RGBQUAD* quads) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (base >= PALETTE_MAX_ENTRY_COUNT
        || count > PALETTE_MAX_ENTRY_COUNT
        || (base + count) > PALETTE_MAX_ENTRY_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    if (quads == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    EnterCriticalSection(&self->lock);

    CopyMemory(quads, &self->bitmap.header.palette[base], count * sizeof(RGBQUAD));

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddsd_set_palette(ddsd* self, u32 start, u32 count, RGBQUAD* quads) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (start >= PALETTE_MAX_ENTRY_COUNT
        || count > PALETTE_MAX_ENTRY_COUNT
        || (start + count) > PALETTE_MAX_ENTRY_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    if (quads == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    EnterCriticalSection(&self->lock);

    // TODO do I have all parameters here?
    // TODO proper implementation.

    if (self->bitmap.dc != NULL) {
        // TODO what about 1,2,4,8 bits of palette caps?
        SetDIBColorTable(self->bitmap.dc, start, count, quads);
    }

    CopyMemory(&self->bitmap.header.palette[start], quads, count * sizeof(RGBQUAD));

    // TODO what to do for custom memory that does not have HDC?

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddsd_get_dc(ddsd* self, HDC* hdc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (hdc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->desc.dwFlags & DDSD_LPSURFACE) {
        return DDERR_UNSUPPORTED;
    }

    if (self->exposed) {
        return DDERR_DCALREADYCREATED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    RECT lock;
    if (SUCCEEDED(hr = ddsd_get_rect(self, &lock))) {
        if (SUCCEEDED(hr = ddsd_lock_rect(self, &lock))) {
            self->exposed = TRUE;

            if (self->desc.ddsCaps.dwCaps & DDSCAPS_OWNDC) {
                self->bitmap.checkpoint = SaveDC(self->bitmap.dc);
            }

            *hdc = self->bitmap.dc;
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_release_dc(ddsd* self, HDC hdc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (hdc == NULL) {
        return DDERR_INVALIDPARAMS;
    }
    
    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->desc.dwFlags & DDSD_LPSURFACE) {
        return DDERR_UNSUPPORTED;
    }

    if (!self->exposed) {
        return DDERR_GENERIC; // TODO
    }

    if (self->bitmap.dc != hdc) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    RECT lock;
    if (SUCCEEDED(hr = ddsd_get_rect(self, &lock))) {
        if (SUCCEEDED(hr = ddsd_unlock_rect(self, &lock))) {
            self->exposed = FALSE;
            self->uniqueness++;

            if (self->desc.ddsCaps.dwCaps & DDSCAPS_OWNDC) {
                hr = RestoreDC(self->bitmap.dc, self->bitmap.checkpoint)
                    ? DD_OK : DDERR_GENERIC;
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_lock(ddsd* self, RECT* rect, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL || desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validate intersection DDERR_INVALIDRECT

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddsd_lock_rect(self, rect))) {
        // TODO 1,2,4-bit support
        const u32 bits = self->desc.ddpfPixelFormat.dwRGBBitCount;
        const u32 bytes = (bits == 15 ? 16 : bits) / 8;
        // TODO proper offset calculation
        // TODO properly fill in desc
        desc->lpSurface = self->data + rect->left * bytes + rect->top * self->desc.lPitch;
        desc->lPitch = self->desc.lPitch;
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_unlock(ddsd* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validate against intersection DDERR_INVALIDRECT

    if (lock_get_count(self->locks) == 0) {
        return DDERR_NOTLOCKED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddsd_unlock_rect(self, rect))) {
        self->uniqueness++;
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_page_lock(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    InterlockedIncrement(&self->pages);

    return DD_OK;
}

HRESULT ddsd_page_unlock(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->pages == 0) {
        return DDERR_NOTPAGELOCKED;
    }

    s32 result = InterlockedDecrement(&self->pages);

    if ((result = max(result, 0)) == 0) {
        self->pages = 0;
    }

    return DD_OK;
}

HRESULT ddsd_get_surface_desc(ddsd* self, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    CopyMemory(desc, &self->desc, sizeof(DDSURFACEDESC2));

    return DD_OK;
}

HRESULT ddsd_set_surface_desc(ddsd* self, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT ddsd_get_uniqueness_value(ddsd* self, u32* value) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    *value = self->uniqueness;

    return DD_OK;
}

HRESULT ddsd_change_uniqueness_value(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    InterlockedIncrement(&self->uniqueness);

    return DD_OK;
}

HRESULT ddsd_get_lock_count(ddsd* self, u32* count) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (count == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    *count = lock_get_count(self->locks);

    return DD_OK;
}

HRESULT ddsd_get_rect(ddsd* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    rect->left = 0;
    rect->top = 0;
    rect->right = self->desc.dwWidth;
    rect->bottom = self->desc.dwHeight;

    return DD_OK;
}

HRESULT ddsd_inside_rect(ddsd* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsValidRect(rect)) {
        return DDERR_INVALIDRECT;
    }

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    RECT bounds;
    ZeroMemory(&bounds, sizeof(RECT));
    bounds.right = (s32)self->desc.dwWidth;
    bounds.bottom = (s32)self->desc.dwHeight;

    return IsInsideRect(&bounds, rect) ? DD_OK : DDERR_INVALIDRECT;
}

HRESULT ddsd_lock_rect(ddsd* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsValidRect(rect)) {
        return DDERR_INVALIDRECT;
    }

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = ddsd_inside_rect(self, rect))) {
        hr = lock_acquire(self->locks, rect);
    }

    return hr;
}

HRESULT ddsd_unlock_rect(ddsd* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (lock_get_count(self->locks) == 0) {
        return DDERR_NOTLOCKED;
    }

    return lock_unacquire(self->locks, rect);
}

HRESULT ddsd_restore_surface(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = lock_clear(self->locks))) {
        self->exposed = FALSE;
    }

    return hr;
}

HRESULT ddsd_set_color_key(ddsd* self, u32 flags, DDCOLORKEY* key) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO This is a code duplication. Refactor this...

    if (self->data == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (key == NULL) {
        if (flags & DDCKEY_DESTBLT) {
            self->desc.dwFlags &= ~DDSD_CKDESTBLT;
        }

        if (flags & DDCKEY_DESTOVERLAY) {
            self->desc.dwFlags &= ~DDSD_CKDESTOVERLAY;
        }

        if (flags & DDCKEY_SRCBLT) {
            self->desc.dwFlags &= ~DDSD_CKSRCBLT;
        }

        if (flags & DDCKEY_SRCOVERLAY) {
            self->desc.dwFlags &= ~DDSD_CKSRCOVERLAY;
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
            self->desc.dwFlags |= DDSD_CKDESTBLT;
            CopyMemory(&self->desc.ddckCKDestBlt, &color, sizeof(DDCOLORKEY));
        }

        if (flags & DDCKEY_DESTOVERLAY) {
            self->desc.dwFlags |= DDSD_CKDESTOVERLAY;
            CopyMemory(&self->desc.ddckCKDestOverlay, &color, sizeof(DDCOLORKEY));
        }

        if (flags & DDCKEY_SRCBLT) {
            self->desc.dwFlags |= DDSD_CKSRCBLT;
            CopyMemory(&self->desc.ddckCKSrcBlt, &color, sizeof(DDCOLORKEY));
        }

        if (flags & DDCKEY_SRCOVERLAY) {
            self->desc.dwFlags |= DDSD_CKSRCOVERLAY;
            CopyMemory(&self->desc.ddckCKSrcOverlay, &color, sizeof(DDCOLORKEY));
        }
    }

    return DD_OK;
}
