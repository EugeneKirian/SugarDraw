#include "ddsd.h"
#include "lock.h"
#include "region.h"
#include "utilities.h"

static BOOL SaveHBITMAPToFile(HBITMAP hBitmap, const char* label); // TODO
typedef struct header header;
//static BOOL SaveBITMAPToFile(header* h, void* data, const char* label);

#pragma pack(push, 1)
typedef struct header {
    BITMAPINFOHEADER    info;
    RGBQUAD             palette[PALETTE_MAX_ENTRY_COUNT];
} header;
#pragma pack(pop)

typedef struct bitmap {
    HDC                 dc;
    header              header;
    HANDLE              mapping;
    HBITMAP             bitmap;
    s32                 checkpoint;
} bitmap;

typedef struct palette {
    plt*                lookup;
    RGBQUAD*            quads;
} palette;

typedef struct ddsd {
    allocator*          allocator;
    blitter*            blitter;
    s32                 refs;
    u32                 flags;
    DDSURFACEDESC2*     desc;
    CRITICAL_SECTION    lock;
    lock*               locks;
    bitmap              bitmap;
    palette             palette;
    u8*                 pixels;
    s32                 pages;
    region*             region;
    u32                 uniqueness;
} ddsd;

static HRESULT ddsd_start_record_dc(ddsd* self);
static HRESULT ddsd_end_record_dc(ddsd* self);

static HRESULT ddsd_add_region_rect(ddsd* self, RECT* rect);

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
            if (SUCCEEDED(hr = region_create(allocator, MEM_TAG_DIRECTDRAWSURFACEDATA, &instance->region))) {
                InitializeCriticalSection(&instance->lock);
                instance->refs = 1;
                *object = instance;
                return hr;
            }

            lock_release(instance->locks);
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

        if (self->region != NULL) {
            region_release(self->region);
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

        const u32 bits = self->desc->ddpfPixelFormat.dwRGBBitCount;
        const u32 bpp = bits == 15 ? 16 : bits;
        const u32 stride = ((self->desc->dwWidth * bpp + 31) & ~31) / 8;
        const u32 size = stride * self->desc->dwHeight;

        BITMAPINFOHEADER* header = &self->bitmap.header.info;
        header->biSize = sizeof(BITMAPINFOHEADER);
        header->biWidth = (LONG)self->desc->dwWidth;
        header->biHeight = -(LONG)self->desc->dwHeight;
        header->biPlanes = 1;
        header->biBitCount = bpp;
        header->biCompression = (bits == 15 || bits == 16) ? BI_BITFIELDS : BI_RGB;
        header->biSizeImage = size;

        switch (bits) {
        case 1:
        case 2:
        case 4: {
            EXITCODE(DDERR_UNSUPPORTED);
        }break;
        case 8: {
            header->biClrUsed = header->biClrImportant = PALETTE_MAX_ENTRY_COUNT;
            self->palette.quads = self->bitmap.header.palette;

            bool is_set = FALSE;
            if (GetDeviceCaps(self->bitmap.dc, RASTERCAPS) & RC_PALETTE) {
                PALETTEENTRY entries[PALETTE_MAX_ENTRY_COUNT];
                is_set = GetSystemPaletteEntries(self->bitmap.dc,
                    0, PALETTE_MAX_ENTRY_COUNT, entries) == PALETTE_MAX_ENTRY_COUNT;

                if (is_set) {
                    palette_entry_to_rgb_quad(entries, PALETTE_MAX_ENTRY_COUNT, self->palette.quads);
                }
            }

            if (!is_set) {
                // Set default palette to gray scale.
                for (u32 i = 0; i < PALETTE_MAX_ENTRY_COUNT; i++) {
                    self->palette.quads[i].rgbRed = (u8)i;
                    self->palette.quads[i].rgbGreen = (u8)i;
                    self->palette.quads[i].rgbBlue = (u8)i;
                }
            }
        }break;
        case 15:
        case 16: {
            ((DWORD*)self->bitmap.header.palette)[0]
                = self->desc->ddpfPixelFormat.dwRBitMask;
            ((DWORD*)self->bitmap.header.palette)[1]
                = self->desc->ddpfPixelFormat.dwGBitMask;
            ((DWORD*)self->bitmap.header.palette)[2]
                = self->desc->ddpfPixelFormat.dwBBitMask;
        }break;
        }

        self->bitmap.mapping = CreateFileMappingA(INVALID_HANDLE_VALUE,
            NULL, PAGE_READWRITE | SEC_COMMIT, 0, header->biSizeImage, NULL);
        if (self->bitmap.mapping == NULL) {
            // TODO clean-up...
            EXITCODE(DDERR_OUTOFMEMORY);
        }

        self->desc->lPitch = stride;

        const BITMAPINFO* bmi = (BITMAPINFO*)header;
        self->bitmap.bitmap = CreateDIBSection(self->bitmap.dc, bmi,
            DIB_RGB_COLORS, &self->pixels, self->bitmap.mapping, 0);
        if (self->bitmap.bitmap == NULL) {
            // TODO clean-up...
            EXITCODE(DDERR_OUTOFMEMORY);
        }

        SelectObject(self->bitmap.dc, self->bitmap.bitmap);

        // TODO
        //memset(self->pixels, 0xFF, header->biSizeImage);
        //SaveHBITMAPToFile(self->bitmap.bitmap, "init");
        // END TODO
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

    //SaveBITMAPToFile(&self->bitmap.header, self->pixels, "blt target");
    //SaveHBITMAPToFile(self->bitmap.bitmap, "blt target");
    if (surface != NULL) {
        //SaveBITMAPToFile(&surface->bitmap.header, surface->pixels, "blt source");
        //SaveHBITMAPToFile(surface->bitmap.bitmap, "blt source");
    }

    MAKETYPE(RECT, rect);
    if (SUCCEEDED(hr = ddsd_get_rect(self, &rect))) {
        // Clip the destination rectangle lock to the overlap area,
        // So that the area outside of the surface isn't affected,
        // and preserve the original rectangle for proper blitting later.
        if (IntersectRect(&rect, dst, &rect)) {
            if (SUCCEEDED(hr = ddsd_lock_rect(self, &rect, DDLOCK_NONE))) {
                if (surface != NULL) {
                    if (FAILED(hr = ddsd_lock_rect(surface, src, DDLOCK_READONLY))) {
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
                    submission.images.destination.palette.palette = self->palette.quads;
                    submission.images.destination.palette.lookup = self->palette.lookup;
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
                        submission.images.source.palette.palette = surface->palette.quads;
                        submission.images.source.palette.lookup = surface->palette.lookup;
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
    if (SUCCEEDED(hr = ddsd_lock_rect(self, dst, DDLOCK_NONE))) {
        if (FAILED(hr = ddsd_lock_rect(surface, src, DDLOCK_READONLY))) {
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
            submission.images.destination.palette.palette = self->palette.quads;
            submission.images.destination.palette.lookup = self->palette.lookup;
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
            submission.images.source.palette.palette = surface->palette.quads;
            submission.images.source.palette.lookup = surface->palette.lookup;
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

HRESULT ddsd_get_palette(ddsd* self, u32 count, RGBQUAD* quads) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (quads == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);

    CopyMemory(quads, self->bitmap.header.palette, count * sizeof(RGBQUAD));

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddsd_set_palette(ddsd* self, u32 count, RGBQUAD* quads, plt* lookup) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (quads == NULL || lookup == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);

    HRESULT hr = DD_OK;
    MAKETYPE(RECT, rect);
    if (SUCCEEDED(hr = ddsd_get_rect(self, &rect))) {
        if (SUCCEEDED(hr = ddsd_add_region_rect(self, &rect))) {
            self->palette.lookup = lookup;
            CopyMemory(self->bitmap.header.palette, quads, count * sizeof(RGBQUAD));

            if (self->bitmap.dc != NULL) {
                SetDIBColorTable(self->bitmap.dc, 0, count, quads);
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_remove_palette(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->pixels == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    HRESULT hr = DD_OK;
    MAKETYPE(RECT, rect);
    if (SUCCEEDED(hr = ddsd_get_rect(self, &rect))) {
        if (SUCCEEDED(hr = ddsd_add_region_rect(self, &rect))) {
            self->palette.lookup = NULL;
        }
    }

    return hr;
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

    if (self->flags & DDSD_DCEXPOSED) {
        return DDERR_DCALREADYCREATED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    RECT lock;
    if (SUCCEEDED(hr = ddsd_get_rect(self, &lock))) {
        if (SUCCEEDED(hr = ddsd_lock_rect(self, &lock, DDLOCK_READONLY))) {
            if (SUCCEEDED(hr = ddsd_start_record_dc(self))) {
                self->flags |= DDSD_DCEXPOSED;

                if (self->desc->ddsCaps.dwCaps & DDSCAPS_OWNDC) {
                    self->bitmap.checkpoint = SaveDC(self->bitmap.dc);
                }

                *hdc = self->bitmap.dc;
                goto exit;
            }

            ddsd_unlock_rect(self, &lock);
        }
    }

exit:
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

    if (!(self->flags & DDSD_DCEXPOSED)) {
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
            if (SUCCEEDED(hr = ddsd_end_record_dc(self))) {
                self->flags &= ~DDSD_DCEXPOSED;
                self->uniqueness++;

                if (self->desc->ddsCaps.dwCaps & DDSCAPS_OWNDC) {
                    RestoreDC(self->bitmap.dc, self->bitmap.checkpoint);
                }
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddsd_lock(ddsd* self, RECT* rect, DDSURFACEDESC2* desc, u32 flags) {
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

    if (SUCCEEDED(hr = ddsd_lock_rect(self, rect, flags))) {
        // TODO 1,2,4-bit support
        const u32 bits = self->desc->ddpfPixelFormat.dwRGBBitCount;
        const u32 bytes = bits == 15 ? 2 : (bits / 8);
        CopyMemory(desc, self->desc, sizeof(DDSURFACEDESC2));

        desc->dwFlags |= DDSD_PITCH | DDSD_LPSURFACE;
        desc->lpSurface = self->pixels
            + rect->left * bytes + rect->top * self->desc->lPitch;
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
    SaveHBITMAPToFile(self->bitmap.bitmap, "unlock");
    //SaveBITMAPToFile(&self->bitmap.header, self->pixels, "unlock");
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

HRESULT ddsd_lock_rect(ddsd* self, RECT* rect, u32 flags) {
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
        if (SUCCEEDED(hr = lock_acquire(self->locks, rect))) {
            if (!(flags & DDLOCK_READONLY)) {
                hr = ddsd_add_region_rect(self, rect);
            }
        }
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
        if (SUCCEEDED(hr = region_clear(self->region))) {
            self->flags &= ~DDSD_DCEXPOSED;
        }
    }

    return hr;
}

HRESULT ddsd_clear_region(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    return region_clear(self->region);
}

HRESULT ddsd_get_region(ddsd* self, RGNDATA** region) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (region == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    *region = self->region->region;

    return DD_OK;
}

HRESULT ddsd_set_region_tracking(ddsd* self, bool track) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    self->flags = track
        ? (self->flags | DDSD_TRACKCHANGES)
        : (self->flags & ~(DDSD_TRACKCHANGES));

    return DD_OK;
}

HRESULT ddsd_start_record_dc(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->flags & DDSD_TRACKCHANGES) {
        SetBoundsRect(self->bitmap.dc, NULL, DCB_ENABLE | DCB_RESET);
    }

    return DD_OK;
}

HRESULT ddsd_end_record_dc(ddsd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    if (self->flags & DDSD_TRACKCHANGES) {
        MAKETYPE(RECT, rect);
        if (GetBoundsRect(self->bitmap.dc, &rect, DCB_RESET) & DCB_SET) {
            hr = region_add_rect(self->region, &rect);
        }
    }

    return hr;
}

HRESULT ddsd_add_region_rect(ddsd* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    if (self->flags & DDSD_TRACKCHANGES) {
        hr = region_add_rect(self->region, rect);
    }

    return hr;
}

#include <stdio.h>
static int id = 0;
static char szFilePath[MAX_PATH];

static BOOL SaveBITMAPToFile(header* h, void* data, const char* label) {
    sprintf(szFilePath, "C:\\GIT\\Images\\%d-%p-%p-%s.bmp", id++, h, data, label);

    DWORD dwBitsSize = 0;
    if (h->info.biBitCount <= 8) {
        // For palettized bitmaps, bits are stored as indices
        dwBitsSize = ((h->info.biWidth * (DWORD)(-1 * h->info.biHeight) + 31) / 32) * (DWORD)(-1 * h->info.biHeight);
    }
    else if (h->info.biBitCount == 16 || h->info.biBitCount == 15) {
        // 16-bit: 2 bytes per pixel, aligned to 4 bytes per row
        DWORD bytesPerRow = ((h->info.biWidth * 16 + 31) / 32) * 4;
        dwBitsSize = bytesPerRow * (DWORD)(-1 * h->info.biHeight);
    }
    else {
        // 24/32-bit: get actual DIB bits size
        DWORD bytesPerRow = ((h->info.biWidth * h->info.biBitCount + 31) / 32) * 4;
        dwBitsSize = bytesPerRow * (DWORD)(-1 * h->info.biHeight);
    }

    int nColors = 0;
    DWORD compression = BI_RGB;
    DWORD redMask = 0, greenMask = 0, blueMask = 0;

    if (h->info.biBitCount <= 8) {
        nColors = (h->info.biBitCount == 1) ? 2 :
            (h->info.biBitCount == 4) ? 16 : 256;
        compression = BI_RGB;
    }
    else if (h->info.biBitCount == 16 || h->info.biBitCount == 15) {
        // For 15/16-bit, use BI_BITFIELDS
        compression = BI_BITFIELDS;
        nColors = 3; // Need 3 RGBQUAD entries for the masks
    }
    else {
        compression = BI_RGB;
        nColors = 0;
    }

    DWORD dwDIBSize = sizeof(BITMAPINFOHEADER);
    if (nColors > 0) {
        dwDIBSize += nColors * sizeof(RGBQUAD);
    }


    DWORD bytes = 0;
    BITMAPFILEHEADER bf = { 0 };
    bf.bfType = 0x4D42; // "BM"
    bf.bfSize = sizeof(BITMAPFILEHEADER) + dwDIBSize + dwBitsSize;
    bf.bfOffBits = sizeof(BITMAPFILEHEADER) + dwDIBSize;

    HANDLE hFile = CreateFileA(szFilePath, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &bytes, NULL);
    WriteFile(hFile, h, dwDIBSize, &bytes, NULL);
    WriteFile(hFile, data, dwBitsSize, &bytes, NULL);
    CloseHandle(hFile);
    return TRUE;
}

static BOOL SaveHBITMAPToFile(HBITMAP hBitmap, const char* label) {
   // return TRUE;
    if (!hBitmap) {
        return FALSE;
    }

    sprintf(szFilePath, "C:\\GIT\\Images\\%d-%s.bmp", id++, label);

    HDC hDC = GetDC(NULL);
    if (!hDC) {
        return FALSE;
    }

    // Get bitmap information
    BITMAP bm;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bm)) {
        ReleaseDC(NULL, hDC);
        return FALSE;
    }

    // Calculate number of color table entries
    int nColors = 0;
    DWORD compression = BI_RGB;
    DWORD redMask = 0, greenMask = 0, blueMask = 0;

    if (bm.bmBitsPixel <= 8) {
        nColors = (bm.bmBitsPixel == 1) ? 2 :
            (bm.bmBitsPixel == 4) ? 16 : 256;
        compression = BI_RGB;
    }
    else if (bm.bmBitsPixel == 16 || bm.bmBitsPixel == 15) {
        // For 15/16-bit, use BI_BITFIELDS
        compression = BI_BITFIELDS;
        nColors = 3; // Need 3 RGBQUAD entries for the masks
    }
    else {
        compression = BI_RGB;
        nColors = 0;
    }

    // Get bitmap info header
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = (bm.bmBitsPixel == 15) ? 16 : bm.bmBitsPixel; // 15-bit stored as 16-bit
    bi.biCompression = compression;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = nColors;
    bi.biClrImportant = nColors;

    // Calculate DIB size (including color table or bitfields)
    DWORD dwDIBSize = sizeof(BITMAPINFOHEADER);
    if (nColors > 0) {
        dwDIBSize += nColors * sizeof(RGBQUAD);
    }

    // Calculate bits size (including padding)
    DWORD dwBitsSize = 0;
    if (bm.bmBitsPixel <= 8) {
        // For palettized bitmaps, bits are stored as indices
        dwBitsSize = ((bm.bmWidth * bm.bmBitsPixel + 31) / 32) * 4 * bm.bmHeight;
    }
    else if (bm.bmBitsPixel == 16 || bm.bmBitsPixel == 15) {
        // 16-bit: 2 bytes per pixel, aligned to 4 bytes per row
        DWORD bytesPerRow = ((bm.bmWidth * 16 + 31) / 32) * 4;
        dwBitsSize = bytesPerRow * bm.bmHeight;
    }
    else {
        // 24/32-bit: get actual DIB bits size
        DWORD bytesPerRow = ((bm.bmWidth * bm.bmBitsPixel + 31) / 32) * 4;
        dwBitsSize = bytesPerRow * bm.bmHeight;
    }

    // Allocate memory for DIB info
    LPBITMAPINFO lpBitmapInfo = (LPBITMAPINFO)malloc(dwDIBSize);
    if (!lpBitmapInfo) {
        ReleaseDC(NULL, hDC);
        return FALSE;
    }

    // Copy header
    memcpy(lpBitmapInfo, &bi, sizeof(BITMAPINFOHEADER));

    // Allocate memory for bitmap bits
    LPBYTE lpBits = (LPBYTE)malloc(dwBitsSize);
    if (!lpBits) {
        free(lpBitmapInfo);
        ReleaseDC(NULL, hDC);
        return FALSE;
    }

    // Get DIB bits using GetDIBits
    HDC hMemDC = CreateCompatibleDC(hDC);
    if (!hMemDC) {
        free(lpBits);
        free(lpBitmapInfo);
        ReleaseDC(NULL, hDC);
        return FALSE;
    }

    // For 16-bit with bitfields, we need to use a special structure
    if (compression == BI_BITFIELDS) {
        // GetDIBits with BI_BITFIELDS requires the masks to be set
        int result = GetDIBits(hMemDC, hBitmap, 0, bm.bmHeight,
            lpBits, lpBitmapInfo, DIB_RGB_COLORS);
        if (!result) {
            free(lpBits);
            free(lpBitmapInfo);
            DeleteDC(hMemDC);
            ReleaseDC(NULL, hDC);
            return FALSE;
        }
    }
    else {
        // For RGB bitmaps, just get the bits
        int result = GetDIBits(hMemDC, hBitmap, 0, bm.bmHeight,
            lpBits, lpBitmapInfo, DIB_RGB_COLORS);
        if (!result) {
            free(lpBits);
            free(lpBitmapInfo);
            DeleteDC(hMemDC);
            ReleaseDC(NULL, hDC);
            return FALSE;
        }
    }

    // Create file
    HANDLE hFile = CreateFileA(szFilePath, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        free(lpBits);
        free(lpBitmapInfo);
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hDC);
        return FALSE;
    }

    // Prepare file header
    BITMAPFILEHEADER bf = { 0 };
    bf.bfType = 0x4D42; // "BM"
    bf.bfSize = sizeof(BITMAPFILEHEADER) + dwDIBSize + dwBitsSize;
    bf.bfOffBits = sizeof(BITMAPFILEHEADER) + dwDIBSize;

    // Write file header
    DWORD dwWritten = 0;
    if (!WriteFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &dwWritten, NULL) ||
        dwWritten != sizeof(BITMAPFILEHEADER)) {
        CloseHandle(hFile);
        free(lpBits);
        free(lpBitmapInfo);
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hDC);
        return FALSE;
    }

    // Write info header + color table/bitfields
    if (!WriteFile(hFile, lpBitmapInfo, dwDIBSize, &dwWritten, NULL) ||
        dwWritten != dwDIBSize) {
        CloseHandle(hFile);
        free(lpBits);
        free(lpBitmapInfo);
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hDC);
        return FALSE;
    }

    // Write bitmap bits
    if (!WriteFile(hFile, lpBits, dwBitsSize, &dwWritten, NULL) ||
        dwWritten != dwBitsSize) {
        CloseHandle(hFile);
        free(lpBits);
        free(lpBitmapInfo);
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hDC);
        return FALSE;
    }

    // Cleanup
    CloseHandle(hFile);
    free(lpBits);
    free(lpBitmapInfo);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hDC);

    return TRUE;
}