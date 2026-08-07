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
    blitter*            blitter;
    s32                 refs;
    bool                exposed;
    DDSURFACEDESC2*     desc;
    CRITICAL_SECTION    lock;
    lock*               locks;
    bitmap              bitmap;
    u8*                 pixels;
    s32                 pages;
    u32                 uniqueness;
} ddsd;

HRESULT ddsd_create(allocator* allocator, blitter* blitter, ddsd** object) {
    if (allocator == NULL || blitter == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    ddsd* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_DIRECTDRAWSURFACEDATA, sizeof(ddsd), &instance))) {
        instance->allocator = allocator;
        instance->blitter = blitter;
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

        if (!(self->desc->dwFlags & DDSD_LPSURFACE)) {
            // TODO hdc, hbitmap, maping

            if (self->pixels != NULL) {
                allocator_free(self->allocator, self->pixels);
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

    if (self->pixels != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO all kinds of verifications

    self->desc = desc;
    self->uniqueness++;

    if (self->desc->dwFlags & DDSD_LPSURFACE) {
        self->pixels = desc->lpSurface;
    }
    else {
        // TODO go here only for DC-supporting formats
        // TODO handle cases for YUV, etc formats which are not DC-supported?

        // TODO: From where? exclusive mode hwnd, or cliper?
        self->bitmap.dc = CreateCompatibleDC(NULL);
        if (self->bitmap.dc == NULL) {
            EXITCODE(DDERR_OUTOFMEMORY);
        }

        // TODO make stride aligned to a reasonable value for SIMD processing

        const u32 bpp = self->desc->ddpfPixelFormat.dwRGBBitCount;
        const u32 stride = ((self->desc->dwWidth * bpp + 63) & ~63) >> 3; // TODO
        const u32 size = stride * self->desc->dwHeight;
        const u32 aligned_width = stride / (bpp / 8); // TODO

        BITMAPINFOHEADER* header = &self->bitmap.header.info;
        header->biSize = sizeof(BITMAPINFOHEADER);
        header->biWidth = (LONG)aligned_width;
        header->biHeight = -(LONG)self->desc->dwHeight;
        header->biPlanes = 1;
        header->biBitCount = bpp;
        header->biCompression = (bpp == 15 || bpp == 16) ? BI_BITFIELDS : BI_RGB;

        switch (bpp) {
        case 1:
        case 2:
        case 4: {
            EXITCODE(DDERR_UNSUPPORTED);
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
                = self->desc->ddpfPixelFormat.dwRBitMask;
            ((DWORD*)self->bitmap.header.palette)[1]
                = self->desc->ddpfPixelFormat.dwGBitMask;
            ((DWORD*)self->bitmap.header.palette)[2]
                = self->desc->ddpfPixelFormat.dwBBitMask;
        }break;
        }

        header->biSizeImage = ((aligned_width * bpp + 63) & ~63) / 8 * self->desc->dwHeight; // TODO

        self->bitmap.mapping = CreateFileMappingA(INVALID_HANDLE_VALUE,
            NULL, PAGE_READWRITE | SEC_COMMIT, 0, size, NULL);
        if (self->bitmap.mapping == NULL) {
            // TODO clean-up...
            EXITCODE(DDERR_OUTOFMEMORY);
        }

        self->desc->lPitch = stride;
        self->bitmap.bitmap = CreateDIBSection(self->bitmap.dc,
            (BITMAPINFO*)&self->bitmap.header.info, DIB_RGB_COLORS,
            &self->pixels, self->bitmap.mapping, 0);
        if (self->bitmap.bitmap == NULL) {
            // TODO clean-up...
            EXITCODE(DDERR_OUTOFMEMORY);
        }

        SelectObject(self->bitmap.dc, self->bitmap.bitmap);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_blt(ddsd* self, RECT* dst, ddsd* surface, RECT* src, RGNDATA* region, u32 flags, DDBLTFX* effects) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (dst == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (flags != DDBLT_NONE && (flags & ~DDBLT_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    if (!(flags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL))) {
        if (surface == NULL || src == NULL) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (flags & DDBLT_REQUIRES_FX_STRUCT) {
        if (effects == NULL) {
            return DDERR_INVALIDPARAMS;
        }

        if (effects->dwSize != sizeof(DDBLTFX)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (self->desc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXEDTO8) {
        return DDERR_UNSUPPORTEDFORMAT;
    }

    if (surface != NULL) {
        if (surface->desc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXEDTO8) {
            return DDERR_UNSUPPORTEDFORMAT;
        }
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    MAKETYPE(RECT, rect);
    if (SUCCEEDED(hr = ddsd_get_rect(self, &rect))) {
        // Clip the destination rectangle lock to the overlap area,
        // So that the area outside of the surface isn't affected,
        // and preserve the original rectangle for proper blitting later.
        if (IntersectRect(&rect, dst, &rect)) {
            if (SUCCEEDED(hr = ddsd_lock_rect(self, &rect))) {
                if (surface != NULL) {
                    if (FAILED(hr = ddsd_lock_rect(surface, src))) {
                        ddsd_unlock_rect(self, &rect);
                        EXITCODE(DDERR_LOCKEDSURFACES);
                    }
                }

                MAKETYPE(blt, submission);
                if (flags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL)) {
                    submission.flags |= BLITTER_FILL;
                }

                if (flags & DDBLT_DDFX) {
                    submission.flags |= BLITTER_EFFECTS;
                }

                if (flags & DDBLT_ROTATIONANGLE) {
                    submission.flags |= BLITTER_ROTATION_ANGLE;
                }

                if (surface != NULL) {
                    if (flags & DDBLT_KEYSRC) {
                        submission.flags |= BLITTER_SRC_COLOR_KEY;
                        CopyMemory(&submission.colors.source, &surface->desc->ddckCKSrcBlt, sizeof(DDCOLORKEY));
                    }

                    if (flags & DDBLT_KEYSRCOVERRIDE) {
                        submission.flags |= BLITTER_SRC_COLOR_KEY;
                        CopyMemory(&submission.colors.source, &effects->ddckSrcColorkey, sizeof(DDCOLORKEY));
                    }
                }

                if (flags & DDBLT_KEYDEST) {
                    submission.flags |= BLITTER_DEST_COLOR_KEY;
                    CopyMemory(&submission.colors.destination, &self->desc->ddckCKDestBlt, sizeof(DDCOLORKEY));
                }

                if (flags & DDBLT_KEYDESTOVERRIDE) {
                    submission.flags |= BLITTER_DEST_COLOR_KEY;
                    CopyMemory(&submission.colors.destination, &effects->ddckDestColorkey, sizeof(DDCOLORKEY));
                }

                submission.images.destination.pixels = self->pixels;
                submission.images.destination.width = self->desc->dwWidth;
                submission.images.destination.height = self->desc->dwHeight;
                submission.images.destination.stride = self->desc->lPitch;
                CopyMemory(&submission.images.destination.format,
                    &self->desc->ddpfPixelFormat, sizeof(DDPIXELFORMAT));

                if ((submission.images.destination.format.dwFlags & DDPF_PALETTEINDEXED1)
                    || (submission.images.destination.format.dwFlags & DDPF_PALETTEINDEXED2)
                    || (submission.images.destination.format.dwFlags & DDPF_PALETTEINDEXED4)
                    || (submission.images.destination.format.dwFlags & DDPF_PALETTEINDEXED8)) {
                    // TODO count based on palette caps
                    submission.images.destination.palette.count =
                        1 << submission.images.destination.format.dwRGBBitCount;
                    submission.images.destination.palette.palette = self->bitmap.header.palette;
                }

                if (surface != NULL) {
                    submission.images.source.pixels = surface->pixels;
                    submission.images.source.width = surface->desc->dwWidth;
                    submission.images.source.height = surface->desc->dwHeight;
                    submission.images.source.stride = surface->desc->lPitch;
                    CopyMemory(&submission.images.source.format,
                        &surface->desc->ddpfPixelFormat, sizeof(DDPIXELFORMAT));

                    if ((submission.images.source.format.dwFlags & DDPF_PALETTEINDEXED1)
                        || (submission.images.source.format.dwFlags & DDPF_PALETTEINDEXED2)
                        || (submission.images.source.format.dwFlags & DDPF_PALETTEINDEXED4)
                        || (submission.images.source.format.dwFlags & DDPF_PALETTEINDEXED8)) {
                        // TODO count based on palette caps
                        submission.images.source.palette.count =
                            1 << submission.images.source.format.dwRGBBitCount;
                        submission.images.source.palette.palette = surface->bitmap.header.palette;
                    }
                }

                CopyMemory(&submission.rects.destination, dst, sizeof(RECT));

                if (src != NULL) {
                    CopyMemory(&submission.rects.source, src, sizeof(RECT));
                }

                submission.region = region;

                if (SUCCEEDED(hr = blitter_submit(self->blitter, &submission))) {
                    self->uniqueness++;
                }

                if (surface != NULL) {
                    ddsd_unlock_rect(surface, src);
                }

                ddsd_unlock_rect(self, &rect);
            }
        }
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

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO better way to acquire multiple locks, and unwind easily...
    if (SUCCEEDED(hr = ddsd_lock_rect(self, dst))) {
        if (FAILED(hr = ddsd_lock_rect(surface, src))) {
            ddsd_unlock_rect(self, dst);
            EXITCODE(DDERR_LOCKEDSURFACES);
        }

        MAKETYPE(blt, submission);
        if (transfer & DDBLTFAST_SRCCOLORKEY) {
            submission.flags |= BLITTER_SRC_COLOR_KEY;
            CopyMemory(&submission.colors.source, &surface->desc->ddckCKSrcBlt, sizeof(DDCOLORKEY));
        }

        if (transfer & DDBLTFAST_DESTCOLORKEY) {
            submission.flags |= BLITTER_DEST_COLOR_KEY;
            CopyMemory(&submission.colors.destination, &self->desc->ddckCKDestBlt, sizeof(DDCOLORKEY));
        }

        submission.images.destination.pixels = self->pixels;
        submission.images.destination.width = self->desc->dwWidth;
        submission.images.destination.height = self->desc->dwHeight;
        submission.images.destination.stride = self->desc->lPitch;
        CopyMemory(&submission.images.destination.format,
            &self->desc->ddpfPixelFormat, sizeof(DDPIXELFORMAT));

        if (submission.images.destination.format.dwFlags & DDPF_PALETTEINDEXEDTO8) {
            return DDERR_UNSUPPORTEDFORMAT;
        }

        if ((submission.images.destination.format.dwFlags & DDPF_PALETTEINDEXED1)
            || (submission.images.destination.format.dwFlags & DDPF_PALETTEINDEXED2)
            || (submission.images.destination.format.dwFlags & DDPF_PALETTEINDEXED4)
            || (submission.images.destination.format.dwFlags & DDPF_PALETTEINDEXED8)) {
            // TODO get palette count from the palette caps
            submission.images.destination.palette.count =
                1 << submission.images.destination.format.dwRGBBitCount;
            submission.images.destination.palette.palette = self->bitmap.header.palette;
        }

        submission.images.source.pixels = surface->pixels;
        submission.images.source.width = surface->desc->dwWidth;
        submission.images.source.height = surface->desc->dwHeight;
        submission.images.source.stride = surface->desc->lPitch;
        CopyMemory(&submission.images.source.format,
            &surface->desc->ddpfPixelFormat, sizeof(DDPIXELFORMAT));

        if (submission.images.source.format.dwFlags & DDPF_PALETTEINDEXEDTO8) {
            return DDERR_UNSUPPORTEDFORMAT;
        }

        if ((submission.images.source.format.dwFlags & DDPF_PALETTEINDEXED1)
            || (submission.images.source.format.dwFlags & DDPF_PALETTEINDEXED2)
            || (submission.images.source.format.dwFlags & DDPF_PALETTEINDEXED4)
            || (submission.images.source.format.dwFlags & DDPF_PALETTEINDEXED8)) {
            // TODO get palette count from the palette caps
            submission.images.source.palette.count =
                1 << submission.images.source.format.dwRGBBitCount;
            submission.images.source.palette.palette = surface->bitmap.header.palette;
        }

        CopyMemory(&submission.rects.destination, dst, sizeof(RECT));
        CopyMemory(&submission.rects.source, src, sizeof(RECT));

        if (SUCCEEDED(hr = blitter_submit(self->blitter, &submission))) {
            self->uniqueness++;
        }

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

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (base >= PALETTE_MAX_ENTRY_COUNT
        || count > PALETTE_MAX_ENTRY_COUNT
        || (base + count) > PALETTE_MAX_ENTRY_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    if (quads == NULL) {
        return DDERR_INVALIDPARAMS;
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

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (start >= PALETTE_MAX_ENTRY_COUNT
        || count > PALETTE_MAX_ENTRY_COUNT
        || (start + count) > PALETTE_MAX_ENTRY_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    if (quads == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);

    // TODO do I have all parameters here?
    // TODO proper implementation.

    if (self->bitmap.dc != NULL) {
        // TODO what about 1,2,4,8 bits of palette caps?
        SetDIBColorTable(self->bitmap.dc, start, count, quads);
    }

    CopyMemory(&self->bitmap.header.palette[start], quads, count * sizeof(RGBQUAD));

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddsd_get_dc(ddsd* self, HDC* hdc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (hdc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->desc->dwFlags & DDSD_LPSURFACE) {
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

            if (self->desc->ddsCaps.dwCaps & DDSCAPS_OWNDC) {
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

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (hdc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->desc->dwFlags & DDSD_LPSURFACE) {
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

            if (self->desc->ddsCaps.dwCaps & DDSCAPS_OWNDC) {
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

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (rect == NULL || desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO validate intersection DDERR_INVALIDRECT

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddsd_lock_rect(self, rect))) {
        // TODO 1,2,4-bit support
        const u32 bits = self->desc->ddpfPixelFormat.dwRGBBitCount;
        const u32 bytes = (bits == 15 ? 16 : bits) / 8;
        // TODO proper offset calculation
        CopyMemory(desc, self->desc, sizeof(DDSURFACEDESC2));
        desc->lpSurface = self->pixels + rect->left * bytes + rect->top * self->desc->lPitch;
        desc->lPitch = self->desc->lPitch;
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_unlock(ddsd* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
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

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    InterlockedIncrement(&self->pages);

    return DD_OK;
}

HRESULT ddsd_page_unlock(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
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

HRESULT ddsd_set_surface_desc(ddsd* self, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT ddsd_get_uniqueness_value(ddsd* self, u32* value) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    *value = self->uniqueness;

    return DD_OK;
}

HRESULT ddsd_change_uniqueness_value(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
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

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    rect->left = 0;
    rect->top = 0;
    rect->right = self->desc->dwWidth;
    rect->bottom = self->desc->dwHeight;

    return DD_OK;
}

HRESULT ddsd_inside_rect(ddsd* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsValidRect(rect)) {
        return DDERR_INVALIDRECT;
    }

    MAKETYPE(RECT, bounds);
    bounds.right = (s32)self->desc->dwWidth;
    bounds.bottom = (s32)self->desc->dwHeight;

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
