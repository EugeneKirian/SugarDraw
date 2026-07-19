#include "dd.h"
#include "ddc.h"
#include "ddcc.h"
#include "ddg.h"
#include "ddp.h"
#include "dds.h"
#include "utilities.h"

HRESULT dds_create(sugar* manager, dds** object) {
    if (manager == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    dds* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWSURFACE, sizeof(dds), &instance))) {
        instance->manager = manager;
        if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAWSURFACE, &instance->interfaces))) {
            if (SUCCEEDED(hr = connector_create(manager->allocator, MEM_TAG_DIRECTDRAWSURFACE, &instance->attachments))) {
                if (SUCCEEDED(hr = connector_create(manager->allocator, MEM_TAG_DIRECTDRAWSURFACE, &instance->overlays))) {
                    InitializeCriticalSection(&instance->lock);
                    *object = instance;
                    return hr;
                }

                connector_release(instance->attachments);
            }

            intfc_release(instance->interfaces);
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void dds_release(dds* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const s32 item_count = intfc_get_count(self->interfaces);
            for (s32 i = 0; i < item_count; i++) {
                idds* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    idds_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        if (self->color != NULL) {
            ddcc_release(self->color, RELEASE_NONE);
        }

        if (self->surface != NULL) {
            ddsd_remove_ref(self->surface);
        }

        if (self->attachments != NULL) {
            // TODO attachments
        }

        if (self->overlay.target.instance != NULL) {
            // TODO 
        }

        if (self->overlays != NULL) {
            // TODO overlays
        }

        if (self->clipper.instance != NULL) {
            iddc* intfc = NULL;
            if (SUCCEEDED(ddc_get_interface(self->clipper.instance, &self->clipper.id, &intfc))) {
                iddc_remove_ref(intfc);
            }
        }

        if (self->palette.instance != NULL) {
            ddp* instance = self->palette.instance;
            ddp_unregister_surface(instance, self);

            iddp* intfc = NULL;
            if (SUCCEEDED(ddp_get_interface(instance, &self->palette.id, &intfc))) {
                iddp_remove_ref(intfc);
            }
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            if (self->instance != NULL) {
                dd_remove_surface(self->instance, self);
            }
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT dds_get_interface(dds* self, const GUID* riid, void** object) {
    HRESULT hr = E_NOINTERFACE;
    EnterCriticalSection(&self->lock);

    const s32 item_count = intfc_get_count(self->interfaces);
    for (s32 i = 0; i < item_count; i++) {
        idds* instance = NULL;
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

HRESULT dds_query_interface(dds* self, const GUID* riid, void** object) {
    HRESULT hr = E_NOINTERFACE;
    EnterCriticalSection(&self->lock);

    if (IsEqualGUID(&IID_IDirectDrawGammaControl, riid)) {
        hr = dds_query_color_control(self, riid, object);
        goto exit;
    }

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

HRESULT dds_add_attached_surface(dds* self, iddsconn* surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance != surface->instance->instance) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO proper implementation

    // TODO increment ref count on the interface

    // TODO DDERR_SURFACEALREADYATTACHED
    // TODO DDERR_SURFACEALREADYDEPENDENT

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dds_add_overlay_dirty_rect(dds* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS; // Can rect be NULL?
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }
    
    // TODO proper implementation

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dds_blt(dds* self, RECT* dst, dds* surface, RECT* src, u32 flags, DDBLTFX* effects) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO ModeX

    // TODO proper implementation

    return DD_OK; // TODO
}

HRESULT dds_blt_batch(dds* self, DDBLTBATCH* batch, u32 count, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO proper implementation

    return DDERR_UNSUPPORTED;
}

HRESULT dds_blt_fast(dds* self, u32 x, u32 y, dds* surface, RECT* rect, u32 transfer) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->instance != surface->instance) {
        return DDERR_INVALIDPARAMS;
    }

    if (surface->clipper.instance != NULL) {
        return DDERR_BLTFASTCANTCLIP;
    }

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
        || !(surface->desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)) {
        return DDERR_UNSUPPORTED; // TODO verify
    }

    if (transfer != DDBLTFAST_NONE && (transfer & ~DDBLTFAST_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO adjust rrectangles or bail if no/incomplete overlap?

    // TODO proper implementation, at least a good refactor...
    // TODO: for now handle the most simplest cases...

    // TODO overlay: DDSD_CKDESTOVERLAY and DDSD_CKSRCOVERLAY 

    if (transfer & DDBLTFAST_SRCCOLORKEY) {
        if (!(surface->desc.dwFlags & DDSD_CKSRCBLT)) {
            return DDERR_INVALIDCAPS; // TODO error code
        }
    }

    if (transfer & DDBLTFAST_DESTCOLORKEY) {
        if (!(self->desc.dwFlags & DDSD_CKDESTBLT)) {
            return DDERR_INVALIDCAPS; // TODO error code
        }
    }

    // TODO can transfer be both DDBLTFAST_SRCCOLORKEY and DDBLTFAST_DESTCOLORKEY?

    if (self->desc.ddpfPixelFormat.dwRGBBitCount != surface->desc.ddpfPixelFormat.dwRGBBitCount) {
        return DDERR_UNSUPPORTED; // TODO
    }

    // TODO adjust dimensions for partial or no overlaps?
    // do it gracefully, or fails with DDERR_INVALIDRECT ?
    // below is clipping. Need tests!

    // Source rectangle
    s32 src_x = rect == NULL ? 0 : rect->left;
    s32 src_y = rect == NULL ? 0 : rect->top;
    s32 src_w = rect == NULL ? (s32)surface->desc.dwWidth : rect->right;
    s32 src_h = rect == NULL ? (s32)surface->desc.dwHeight : rect->bottom;

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

    // Destination rectangle
    s32 dst_x = x;
    s32 dst_y = y;
    s32 dst_w = x + src_w - src_x;
    s32 dst_h = y + src_h - src_y;

    // Check if the destination starting point (x,y) is within the surface boundaries.
    if (dst_w < dst_x || dst_h < dst_y) {
        return DDERR_INVALIDPARAMS;
    }

    RECT dst;
    dst.left = dst_x;
    dst.top = dst_y;
    dst.right = dst_w;
    dst.bottom = dst_h;

    RECT src;
    src.left = src_x;
    src.top = src_y;
    src.right = src_w;
    src.bottom = src_h;

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddg_can_update(self->graphics, transfer & DDBLTFAST_WAIT))) {
        if (SUCCEEDED(hr = ddsd_blt_fast(self->surface, &dst, surface->surface, &src, transfer))) {
            if (self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
                hr = ddg_signal_update(self->graphics);
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_delete_attached_surface(dds* self, iddsconn* surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO proper implementation
    // TODO Address of the IDirectDrawSurface7 interface for the DirectDrawSurface object to be detached.
    // If this parameter is NULL, all attached surfaces are detached.

    // TODO decrement refs

    return DDERR_UNSUPPORTED;
}

HRESULT dds_enum_attached_surfaces(dds* self) {
    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dds_enum_overlay_z_orders(dds* self) {
    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dds_flip(dds* self, dds* override, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validate flags

    // TODO can override be self?

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_FLIP)) {
        return DDERR_NOTFLIPPABLE;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO check locks DDERR_SURFACEBUSY 

    dds* target = NULL;
    if (override != NULL) {
        // Validate that the override surface is one of the back buffers.
        const s32 item_count = connector_get_count(self->attachments);
        for (s32 i = 0; i < item_count; i++) {
            iddsconn connector;
            ZeroMemory(&connector, sizeof(iddsconn));
            if (SUCCEEDED(connector_get_item(self->attachments, i, &connector))) {
                if (connector.instance == override) {
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
        const s32 item_count = connector_get_count(self->attachments);
        for (s32 i = 0; i < item_count; i++) {
            iddsconn connector;
            ZeroMemory(&connector, sizeof(iddsconn));
            if (SUCCEEDED(connector_get_item(self->attachments, i, &connector))) {
                if (connector.instance->desc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) {
                    target = connector.instance;
                    break;
                }
            }
        }

        // TODO: what is the correct behavior? Need tests!
    }

    if (target == NULL) {
        hr = DDERR_NOTFLIPPABLE; // TODO error code
        goto exit;
    }

    if (SUCCEEDED(hr = ddg_can_update(self->graphics, flags & DDFLIP_WAIT))) {
        target->surface = InterlockedExchangePointer(&self->surface, target->surface);

        if ((self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
            || (self->desc.ddsCaps.dwCaps & (DDSCAPS_OVERLAY | DDSCAPS_VISIBLE))) {
            hr = ddg_signal_update(self->graphics);
        }
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
    const s32 item_count = connector_get_count(self->attachments);
    for (s32 i = 0; i < item_count; i++) {
        iddsconn connector;
        ZeroMemory(&connector, sizeof(iddsconn));
        if (SUCCEEDED(hr = connector_get_item(self->attachments, i, &connector))) {
            const DDSURFACEDESC2* desc = &connector.instance->desc;

            // TODO proper search
            bool criteria = TRUE;
            if (caps->dwCaps != DDSCAPS_NONE) {
                criteria = criteria && (desc->ddsCaps.dwCaps & caps->dwCaps);
            }

            if (caps->dwCaps2 != DDSCAPS2_NONE) {
                criteria = criteria && (desc->ddsCaps.dwCaps2 & caps->dwCaps2);
            }

            if (caps->dwCaps3 != DDSCAPS3_NONE) {
                criteria = criteria && (desc->ddsCaps.dwCaps3 & caps->dwCaps3);
            }

            if (caps->dwCaps4 != DDSCAPS4_NONE) {
                criteria = criteria && (desc->ddsCaps.dwCaps4 & caps->dwCaps4);
            }

            if (criteria) {
                if (match == NULL) {
                    match = connector.instance;
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
        hr = DDERR_SURFACENOTATTACHED; // TODO proper error
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_get_blt_status(dds* self, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (flags != DDGBS_CANBLT && flags != DDGBS_ISBLTDONE) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    HRESULT hr = DD_OK;

    if (flags & DDGBS_CANBLT) {
        u32 count = 0;
        if (SUCCEEDED(hr = ddsd_get_lock_count(self->surface, &count))) {
            if (count == 0) {
                if (self->desc.ddsCaps.dwCaps & DDPCAPS_PRIMARYSURFACE) {
                    return ddg_can_update(self->graphics, FALSE);
                }

                return DD_OK;
            }

            return DDERR_SURFACEBUSY;
        }
    }
    else if (flags & DDGBS_ISBLTDONE) {
        if (self->desc.ddsCaps.dwCaps & DDPCAPS_PRIMARYSURFACE) {
            return ddg_can_update(self->graphics, FALSE);
        }
    }

    return hr;
}

HRESULT dds_get_caps(dds* self, DDSCAPS2* caps) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (caps == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    CopyMemory(caps, &self->desc.ddsCaps, sizeof(DDSCAPS2));

    return DD_OK;
}

HRESULT dds_get_clipper(dds* self, iddcconn* clipper) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (clipper == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->clipper.instance == NULL) {
        return DDERR_NOCLIPPERATTACHED;
    }

    CopyMemory(clipper, &self->clipper, sizeof(iddcconn));

    return DD_OK;
}

HRESULT dds_get_color_key(dds* self, u32 flags, DDCOLORKEY* key) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (flags & DDCKEY_DESTBLT) {
        if (self->desc.dwFlags & DDSD_CKDESTBLT) {
            CopyMemory(key, &self->desc.ddckCKDestBlt, sizeof(DDCOLORKEY));
            return DD_OK;
        }

        return DDERR_NOCOLORKEY;
    }

    // TODO check overlay flags with non-overlay surface

    if (flags & DDSD_CKDESTOVERLAY) {
        if (self->desc.dwFlags & DDCKEY_DESTOVERLAY) {
            CopyMemory(key, &self->desc.ddckCKDestOverlay, sizeof(DDCOLORKEY));
            return DD_OK;
        }

        return DDERR_NOCOLORKEY;
    }

    if (flags & DDCKEY_SRCBLT) {
        if (self->desc.dwFlags & DDSD_CKSRCBLT) {
            CopyMemory(key, &self->desc.ddckCKSrcBlt, sizeof(DDCOLORKEY));
            return DD_OK;
        }

        return DDERR_NOCOLORKEY;
    }

    if (flags & DDCKEY_SRCOVERLAY) {
        if (self->desc.dwFlags & DDSD_CKSRCOVERLAY) {
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

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) {
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    // TODO ModeX

    // TODO validate surface bit count (1,2,4,8) and normal argb color bit masks for (15,16,24,32)
    // TODO DDLOCK_READONLY

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddg_can_update(self->graphics, FALSE))) {
        HDC dc;
        if (SUCCEEDED(hr = ddsd_get_dc(self->surface, &dc))) {
            if (self->desc.ddsCaps.dwCaps & DDSCAPS_PALETTE) {
                // It is not explicitly stated in the documentation,
                // however, palettized surfaces that have no explicitly attached palette
                // must use the palette of primary surface, if it exists.

                // TODO support indexed palettes
                ddp* palette = self->palette.instance != NULL
                    ? self->palette.instance
                    : (self->instance->primary == NULL ? NULL : self->instance->primary->palette.instance);

                if (palette != NULL) {
                    // TODO what about 1,2,4,8 bits of palette caps?
                    const u32 count = 1 << self->desc.ddpfPixelFormat.dwRGBBitCount;
                    SetDIBColorTable(dc, 0, count, palette->quads);
                }
            }

            *hdc = dc;
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_get_flip_status(dds* self, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO proper implementation - check for locks

    return DD_OK;
}

HRESULT dds_get_overlay_position(dds* self, s32* x, s32* y) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO
    // DDERR_GENERIC
    // DDERR_INVALIDOBJECT
    // DDERR_INVALIDPARAMS
    // DDERR_INVALIDPOSITION
    // DDERR_NOOVERLAYDEST
    // DDERR_NOTAOVERLAYSURFACE
    // DDERR_OVERLAYNOTVISIBLE
    // DDERR_SURFACELOST
    // 
    // TODO proper implementation - check for locks

    return DD_OK;
}

HRESULT dds_get_palette(dds* self, iddpconn* palette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (palette == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->palette.instance == NULL) {
        return DDERR_NOPALETTEATTACHED;
    }

    EnterCriticalSection(&self->lock);

    CopyMemory(palette, &self->palette, sizeof(iddpconn));

    // TODO DDERR_NOEXCLUSIVEMODE

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT dds_get_pixel_format(dds* self, DDPIXELFORMAT* format) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (format == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (format->dwSize != sizeof(DDPIXELFORMAT)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    CopyMemory(format, &self->desc.ddpfPixelFormat, sizeof(DDPIXELFORMAT));

    return DD_OK;
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

    if (desc->dwFlags & DDSD_PIXELFORMAT) {
        CopyMemory(&desc->ddpfPixelFormat, &self->desc.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
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
    self->graphics = object->graphics;
    CopyMemory(&self->desc, desc, sizeof(DDSURFACEDESC2));

    // TODO: use validation rules from dds_create (refactoring needed).

    // TODO DDERR_DDSCAPSCOMPLEXREQUIRED

    ddsd* surface = NULL;
    if (SUCCEEDED(hr = ddsd_create(self->manager->allocator, &surface))) {
        if (SUCCEEDED(hr = ddsd_initialize(surface, &self->desc))) {
            self->surface = surface;

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
                    RGBQUAD quads[PALETTE_MAX_ENTRY_COUNT];
                    PALETTEENTRY entries[PALETTE_MAX_ENTRY_COUNT];
                    const u32 count = 1 << self->desc.ddpfPixelFormat.dwRGBBitCount;
                    if (SUCCEEDED(hr = ddsd_get_palette(self->surface, 0, count, quads))) {
                        if (SUCCEEDED(hr = rgb_quad_to_palette_entry(quads, count, entries))) {
                            iddp* instance = NULL;
                            if (SUCCEEDED(hr = dd_create_palette(self->instance, flags, entries, &instance))) {
                                self->palette.instance = instance->instance;
                                CopyMemory(&self->palette.id, &instance->id, sizeof(GUID));
                                ddp_register_surface(self->palette.instance, self);
                            }
                        }
                    }

                    // TODO handle failure of palette creation
                }break;
                }
            }
        }
    }

    // TODO better code...
    if (FAILED(hr)) {
        if (surface != NULL) {
            ddsd_release(surface);
        }

        goto exit;
    }

    if (self->desc.dwFlags & DDSD_BACKBUFFERCOUNT) {
        DDSURFACEDESC2 back;
        CopyMemory(&back, &self->desc, sizeof(DDSURFACEDESC2));

        // TODO properly set values.
        back.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
        back.dwBackBufferCount = 0;

        // TODO set implicit ?

        for (u32 i = 0; i < self->desc.dwBackBufferCount; i++) {
            dds* instance = NULL;
            // TODO. Should this be done using dd_create_surface?
            // Should the back buffers to be enumeratable as surfaces directly via dd_enumerate_surfaces?
            if (SUCCEEDED(hr = dds_create(self->manager, &instance))) {
                if (SUCCEEDED(hr = dds_initialize(instance, self->instance, &back))) {
                    idds* intfc = NULL;
                    // TODO pick proper interface. Tests needed.
                    if (SUCCEEDED(hr = dds_query_interface(instance, &IID_IDirectDrawSurface, &intfc))) {
                        iddsconn connector;
                        connector.instance = instance;
                        CopyMemory(&connector.id, &IID_IDirectDrawSurface, sizeof(GUID));
                        if (SUCCEEDED(hr = connector_add_item(self->attachments, &connector))) {
                            continue;
                        }
                    }
                }

                dds_release(instance, RELEASE_NONE);
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

HRESULT dds_is_lost(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    return DD_OK;
}

HRESULT dds_lock(dds* self, RECT* rect, DDSURFACEDESC2* desc, u32 flags) {
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

    // TODO ModeX

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = ddg_can_update(self->graphics, flags & DDLOCK_WAIT))) {
        RECT lock;
        if (SUCCEEDED(hr = dds_get_rect(self, rect, &lock))) {
            if (!IsValidRect(&lock)) {
                return DDERR_INVALIDRECT;
            }

            EnterCriticalSection(&self->lock);

            hr = ddsd_lock(self->surface, &lock, desc);

            LeaveCriticalSection(&self->lock);
        }
    }

    return hr;
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

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddsd_release_dc(self->surface, hdc))) {
        if (self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
            hr = ddg_signal_update(self->graphics);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_restore(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    return DD_OK;
}

HRESULT dds_set_clipper(dds* self, iddcconn* clipper) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (clipper == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) {
        return DDERR_INVALIDSURFACETYPE;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);
    if (clipper->instance == NULL) {
        if (self->clipper.instance != NULL) {
            iddc* intfc = NULL;
            if (SUCCEEDED(ddc_get_interface(self->clipper.instance, &self->clipper.id, &intfc))) {
                iddc_remove_ref(intfc);
            }
        }
    }
    else if (self->clipper.instance != NULL) {
        ddc* instance = self->clipper.instance;
        if (instance == clipper->instance) {
            if (!IsEqualGUID(&self->clipper.id, &clipper->id)) {
                CopyMemory(&self->clipper.id, &clipper->id, sizeof(GUID));
            }

            goto exit;
        }

        iddc* intfc = NULL;
        if (SUCCEEDED(ddc_get_interface(instance, &self->clipper.id, &intfc))) {
            iddc_remove_ref(intfc);
        }
    }

    CopyMemory(&self->clipper, clipper, sizeof(iddcconn));

    if (self->clipper.instance != NULL) {
        iddc* intfc = NULL;
        ddc* instance = self->clipper.instance;
        ddc_query_interface(instance, &self->clipper.id, &intfc);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
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

    // TODO check overlay flags with non-overlay surface

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
            self->desc.dwFlags &= DDSD_CKSRCOVERLAY;
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

HRESULT dds_set_overlay_position(dds* self, s32 x, s32 y) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_set_palette(dds* self, iddpconn* palette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (palette == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_PALETTE)) {
        return DDERR_NOTPALETTIZED; // TODO verify error code
    }

    if (palette->instance != NULL) {
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
    }

    // TODO:
    // The normal cooperative level indicates that your DirectDraw application
    // will operate as a windowed application. At this cooperative level
    // you won't be able to change the primary surface's palette or perform page flipping.


    // TODO Other checks...
    // TODO DDERR_NOT8BITCOLOR
    // TODO DDERR_NOTONMIPMAPSUBLEVEL

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);
    if (palette->instance == NULL) {
        if (self->palette.instance != NULL) {
            ddp* instance = self->palette.instance;
            ddp_unregister_surface(instance, self);

            iddp* intfc = NULL;
            if (SUCCEEDED(ddp_get_interface(instance, &self->palette.id, &intfc))) {
                iddp_remove_ref(intfc);
            }
        }
    }
    else if (self->palette.instance != NULL) {
        ddp* instance = self->palette.instance;
        if (instance == palette->instance) {
            if (!IsEqualGUID(&self->palette.id, &palette->id)) {
                CopyMemory(&self->palette.id, &palette->id, sizeof(GUID));
            }

            goto exit;
        }

        ddp_unregister_surface(instance, self);
        iddp* intfc = NULL;
        if (SUCCEEDED(ddp_get_interface(instance, &self->palette.id, &intfc))) {
            iddp_remove_ref(intfc);
        }
    }

    CopyMemory(&self->palette, palette, sizeof(iddpconn));

    if (self->palette.instance != NULL) {
        iddp* intfc = NULL;
        ddp* instance = self->palette.instance;
        ddp_query_interface(instance, &self->palette.id, &intfc);
        ddp_register_surface(instance, self);

        // TODO validate palette capabillities
        // TODO handle flags

        ddsd_set_palette(self->surface,
            0, 1 << self->desc.ddpfPixelFormat.dwRGBBitCount, self->palette.instance->quads);
    }

    ddsd_change_uniqueness_value(self->surface);

    // TODO: how to handle backbuffers? propagate the palette? Need tests...
    // What if backbuffer has it's own palete... Is it posible?

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
        const s32 item_count = connector_get_count(self->attachments);
        for (s32 i = 0; i < item_count; i++) {
            iddsconn connector;
            ZeroMemory(&connector, sizeof(iddsconn));
            if (SUCCEEDED(connector_get_item(self->attachments, i, &connector))) {
                if (connector.instance->desc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) {
                    dds_set_palette(connector.instance, palette);
                }
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_unlock(dds* self, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    RECT lock;
    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = dds_get_rect(self, rect, &lock))) {
        if (!IsValidRect(&lock)) {
            return DDERR_INVALIDRECT;
        }

        EnterCriticalSection(&self->lock);

        if (SUCCEEDED(hr = ddsd_unlock(self->surface, &lock))) {
            if (self->desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
                hr = ddg_signal_update(self->graphics);
            }
        }

        LeaveCriticalSection(&self->lock);
    }

    return hr;
}

HRESULT dds_update_overlay(dds* self, const GUID* riid,
    RECT* src, iddsconn* surface, RECT* dst, u32 flags, DDOVERLAYFX* effects) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_OVERLAY)) {
        return DDERR_NOTAOVERLAYSURFACE;
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) {
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    if (surface->instance != NULL) {
        if (surface->instance->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) {
            return DDERR_ISOPTIMIZEDSURFACE;
        }
    }

    // TODO check DDOVER_HIDE and DDOVER_SHOW vs DDSCAPS_VISIBLE ?

    HRESULT hr = DD_OK;
    if (flags & DDOVER_HIDE) {
        self->desc.ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
        if (self->instance->primary == surface->instance) {
            hr = ddg_signal_update(self->graphics);
        }

        return hr;
    }

    iddsconn connector;
    connector.instance = self;
    CopyMemory(&connector.id, riid, sizeof(GUID));

    RECT source, destination;
    if (SUCCEEDED(hr = dds_get_rect(self, src, &source))) {
        if (SUCCEEDED(hr = dds_get_rect(surface->instance, dst, &destination))) {
            if (!IsValidRect(&source) || !IsValidRect(&destination)) {
                return DDERR_INVALIDRECT;
            }

            // TODO adjust dimensions for partial or no overlaps?
            // do it gracefully, or fails with DDERR_INVALIDRECT ?

            HRESULT hr = DD_OK;
            EnterCriticalSection(&self->lock);
            if (surface->instance == NULL) {
                if (self->overlay.target.instance != NULL) {
                    dds* instance = self->overlay.target.instance;
                    dds_unregister_overlay(instance, &connector);

                    idds* intfc = NULL;
                    if (SUCCEEDED(dds_get_interface(instance, &self->overlay.target.id, &intfc))) {
                        idds_remove_ref(intfc);
                    }
                }

            }
            else if (self->overlay.target.instance != NULL) {
                dds* instance = self->overlay.target.instance;
                if (instance == surface->instance) {
                    if (!IsEqualGUID(&self->overlay.target.id, &surface->id)) {
                        CopyMemory(&self->overlay.target.id, &surface->id, sizeof(GUID));
                    }

                    goto exit;
                }

                dds_unregister_overlay(instance, &connector);
                idds* intfc = NULL;
                if (SUCCEEDED(dds_get_interface(instance, &self->overlay.target.id, &intfc))) {
                    idds_remove_ref(intfc);
                }
            }

            self->overlay.flags = flags;
            CopyMemory(&self->overlay.src, &source, sizeof(RECT));
            CopyMemory(&self->overlay.dst, &destination, sizeof(RECT));
            CopyMemory(&self->overlay.target, surface, sizeof(iddsconn));

            if (flags & DDOVER_DDFX) {
                CopyMemory(&self->overlay.effects, effects, sizeof(DDOVERLAYFX));
            }

            if (self->overlay.target.instance != NULL) {
                idds* intfc = NULL;
                dds* target = self->overlay.target.instance;
                dds_query_interface(target, &self->overlay.target.id, &intfc);
                dds_register_overlay(target, &connector);
            }

        exit:
            if (flags & DDOVER_SHOW) {
                self->desc.ddsCaps.dwCaps |= DDSCAPS_VISIBLE;
                if (self->overlay.target.instance == self->instance->primary) {
                    hr = ddg_signal_update(self->graphics);
                }
            }

            LeaveCriticalSection(&self->lock);
        }
    }

    return hr;
}

HRESULT dds_update_overlay_display(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_update_overlay_z_order(dds* self, u32 flags, iddsconn* surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_dd_interface(dds* self, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    return dd_query_interface(self->instance, &IID_IUnknown, object);
}

HRESULT dds_page_lock(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validate caps

    return ddsd_page_lock(self->surface);
}

HRESULT dds_page_unlock(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validate caps

    return ddsd_page_unlock(self->surface);
}

HRESULT dds_set_surface_desc(dds* self, DDSURFACEDESC2* desc) {
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

    // TODO

    return ddsd_set_surface_desc(self->surface, desc);
}

HRESULT dds_set_private_data(dds* self, const GUID* tag, void* data, u32 size, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (tag == NULL || data == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO validate size
    // TODO validate flags

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_private_data(dds* self, const GUID* tag, void* buffer, u32* size) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (tag == NULL || size == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_free_private_data(dds* self, const GUID* tag) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (tag == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_uniqueness_value(dds* self, u32* value) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (value == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    return ddsd_get_uniqueness_value(self->surface, value);
}

HRESULT dds_change_uniqueness_value(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    return ddsd_change_uniqueness_value(self->surface);
}

HRESULT dds_set_priority(dds* self, u32 priority) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_priority(dds* self, u32* priority) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (priority == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_set_lod(dds* self, u32 lod) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_lod(dds* self, u32* lod) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lod == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_rect(dds* self, RECT* rect, RECT* result) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (result == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    result->left = rect == NULL ? 0 : rect->left;
    result->top = rect == NULL ? 0 : rect->top;
    result->right = rect == NULL ? self->desc.dwWidth : rect->right;
    result->bottom = rect == NULL ? self->desc.dwHeight : rect->bottom;

    return DD_OK;
}

HRESULT dds_query_color_control(dds* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->color != NULL) {
        return ddcc_query_interface(self->color, riid, object);
    }

    ddcc* color = NULL;
    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = ddcc_create(self->manager, &color))) {
        if (SUCCEEDED(hr = ddcc_initialize(color, self))) {
            if (SUCCEEDED(hr = ddcc_query_interface(color, riid, object))) {
                // TODO what is ref count here? Should be 2?
                self->color = color;
                return hr;
            }
        }

        ddcc_release(color, RELEASE_NONE);
    }

    return hr;
}

HRESULT dds_remove_color_control(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    self->color = NULL;

    return DD_OK;
}

HRESULT dds_remove_palette(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ZeroMemory(&self->palette, sizeof(iddpconn));

    return DD_OK;
}

HRESULT dds_register_overlay(dds* self, iddsconn* overlay) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (overlay == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return connector_add_item(self->overlays, overlay);
}

HRESULT dds_unregister_overlay(dds* self, iddsconn* overlay) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (overlay == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    const s32 item_count = connector_get_count(self->overlays);
    for (s32 i = 0; i < item_count; i++) {
        iddsconn connector;
        ZeroMemory(&connector, sizeof(iddsconn));
        if (SUCCEEDED(connector_get_item(self->overlays, i, &connector))) {
            if (connector.instance == overlay->instance
                && IsEqualGUID(&connector.id, &overlay->id)) {
                connector_remove_item(self->overlays, i);
                return DD_OK;
            }
        }
    }

    return DDERR_NOTFOUND;
}
