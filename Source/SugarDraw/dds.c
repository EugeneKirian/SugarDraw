#include "dd.h"
#include "ddc.h"
#include "ddcc.h"
#include "ddg.h"
#include "ddp.h"
#include "dds.h"
#include "ddsfc.h"
#include "utilities.h"

static HRESULT dds_can_flip(dds* self);
static HRESULT dds_restore_surface(dds* self);

HRESULT dds_create(sugar* manager, u32 flags, dds** object) {
    if (manager == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (flags != DDS_NONE && (flags & ~DDS_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    dds* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWSURFACE, sizeof(dds), &instance))) {
        instance->flags = flags;
        instance->manager = manager;
        if (SUCCEEDED(hr = ddsfc_create(manager->allocator, instance, &instance->chain))) {
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

            ddsfc_release(instance->chain);
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void dds_release(dds* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        dds_wait_for_vertical_blank(self, NULL, TRUE);

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

        if (self->chain != NULL) {
            ddsfc_release(self->chain);
        }

        if (self->color != NULL) {
            ddcc_release(self->color, RELEASE_NONE);
        }

        if (self->gamma != NULL) {
            ddgc_release(self->gamma, RELEASE_NONE);
        }

        if (self->surface != NULL) {
            ddsd_remove_ref(self->surface);
        }

        if (self->attachments != NULL) {
            const u32 item_count = connector_get_count(self->attachments);
            for (u32 i = 0; i < item_count; i++) {
                MAKETYPE(iddsconn, connector);
                if (SUCCEEDED(connector_get_item(self->attachments, i, &connector))) {
                    idds* intfc = NULL;
                    if (SUCCEEDED(dds_get_interface(connector.instance, &connector.id, &intfc))) {
                        idds_remove_ref(intfc);
                    }
                }
            }

            // TODO properly release implicit surfaces

            connector_release(self->attachments);
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
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return intfc_query_item(self->interfaces, riid, object);
}

HRESULT dds_query_interface(dds* self, const GUID* riid, void** object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (IsEqualGUID(&IID_IDirectDrawColorControl, riid)) {
        EXITCODE(dds_query_color_control(self, riid, object));
    }
    
    if (IsEqualGUID(&IID_IDirectDrawGammaControl, riid)) {
        EXITCODE(dds_query_gamma_control(self, riid, object));
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
            goto exit;
        }
    }

    hr = E_NOINTERFACE;

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

    // TODO properly release implicit surfaces

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

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance != surface->instance->instance) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (self->instance != surface->instance->instance) {
        return DDERR_DEVICEDOESNTOWNSURFACE;
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

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS; // Can rect be NULL?
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO proper implementation

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dds_blt(dds* self, RECT* dst, dds* surface, RECT* src, u32 flags, DDBLTFX* effects) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (flags != DDBLT_NONE && (flags & ~DDBLT_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    if ((flags & DDBLT_WAIT) && (flags & DDBLT_DONOTWAIT)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) {
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    if (flags & DDBLT_REQUIRES_FX_STRUCT) {
        if (effects == NULL) {
            return DDERR_INVALIDPARAMS;
        }

        if (effects->dwSize != sizeof(DDBLTFX)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (!(self->desc.ddpfPixelFormat.dwFlags & DDPF_RGB)) {
        return DDERR_UNSUPPORTEDFORMAT; // TODO
    }

    if (!(flags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL))) {
        if (surface == NULL) {
            return DDERR_INVALIDPARAMS;
        }

        if (surface->flags & DDS_LOST) {
            return DDERR_SURFACELOST;
        }

        if (self->instance != surface->instance) {
            return DDERR_DEVICEDOESNTOWNSURFACE;
        }

        if (surface->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) {
            return DDERR_ISOPTIMIZEDSURFACE;
        }

        if (flags & DDBLT_KEYSRC) {
            if (!(surface->desc.dwFlags & DDSD_CKSRCBLT)) {
                return DDERR_INVALIDPARAMS;
            }
        }

        if (!(surface->desc.ddpfPixelFormat.dwFlags & DDPF_RGB)) {
            return DDERR_UNSUPPORTEDFORMAT; // TODO
        }
    }

    if (flags & DDBLT_KEYDEST) {
        if (!(self->desc.dwFlags & DDSD_CKDESTBLT)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (flags & DDBLT_ROTATIONANGLE) {
        if (flags & (DDBLT_COLORFILL | DDBLT_DEPTHFILL)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (flags & DDBLT_DEPTHFILL) {
        if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    MAKETYPE(RECT, destination);
    if (SUCCEEDED(hr = dds_get_rect(self, dst, &destination))) {
        MAKETYPE(RECT, bounds);
        if (SUCCEEDED(hr = ddsd_get_rect(self->surface, &bounds))) {
            if (!IntersectRect(&bounds, &bounds, &destination)) {
                EXITCODE(DDERR_INVALIDRECT);
            }
        }

        MAKETYPE(RECT, source);
        if (surface != NULL) {
            if (SUCCEEDED(hr = dds_get_rect(surface, src, &source))) {
                MAKETYPE(RECT, bounds);
                if (SUCCEEDED(hr = ddsd_get_rect(surface->surface, &bounds))) {
                    if (!IsInsideRect(&bounds, &source)) {
                        EXITCODE(DDERR_INVALIDRECT);
                    }
                }
            }
        }

        if (SUCCEEDED(hr = dds_wait_for_vertical_blank(self, surface, flags & DDBLT_WAIT))) {
            RGNDATA* region = NULL;
            if (self->clipper.instance != NULL) {
                ddc_get_region(self->clipper.instance, &region);
            }

            if (SUCCEEDED(hr = ddsd_blt(self->surface, &destination,
                surface == NULL ? NULL : surface->surface,
                surface == NULL ? NULL : &source, region, flags, effects))) {
                hr = dds_signal_update(self);
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_blt_batch(dds* self, DDBLTBATCH* batch, u32 count, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validations

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    return DDERR_UNSUPPORTED;
}

HRESULT dds_blt_fast(dds* self, u32 x, u32 y, dds* surface, RECT* rect, u32 transfer) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if ((self->flags & DDS_LOST) || (surface->flags & DDS_LOST)) {
        return DDERR_SURFACELOST;
    }

    if (transfer != DDBLTFAST_NONE && (transfer & ~DDBLTFAST_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    if ((transfer & DDBLTFAST_WAIT) && (transfer & DDBLTFAST_DONOTWAIT)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->clipper.instance != NULL) {
        return DDERR_BLTFASTCANTCLIP;
    }

    if ((self->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        || (surface->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)) {
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    //if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
    //    || !(surface->desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)) {
    //    return DDERR_UNSUPPORTED; // TODO verify
    //}

    if (transfer & DDBLTFAST_SRCCOLORKEY) {
        if (!(surface->desc.dwFlags & DDSD_CKSRCBLT)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (transfer & DDBLTFAST_DESTCOLORKEY) {
        if (!(self->desc.dwFlags & DDSD_CKDESTBLT)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (self->instance != surface->instance) {
        return DDERR_DEVICEDOESNTOWNSURFACE;
    }

    if (!(self->desc.ddpfPixelFormat.dwFlags && DDPF_RGB)
        || !(surface->desc.ddpfPixelFormat.dwFlags && DDPF_RGB)) {
        return DDERR_UNSUPPORTEDFORMAT; // TODO
    }

    // TODO DDERR_OVERLAPPINGRECTS
    // TODO DDERR_COLORKEYNOTSET

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    MAKETYPE(RECT, src);
    if (SUCCEEDED(hr = dds_get_rect(surface, rect, &src))) {
        if (SUCCEEDED(hr = ddsd_inside_rect(surface->surface, &src))) {
            RECT target;
            target.left = x;
            target.top = y;
            target.right = target.left + src.right - src.left;
            target.bottom = target.top + src.bottom - src.top;

            if (SUCCEEDED(hr = ddsd_inside_rect(self->surface, &target))) {
                if (SUCCEEDED(hr = dds_wait_for_vertical_blank(self, surface, transfer & DDBLTFAST_WAIT))) {
                    if (SUCCEEDED(hr = ddsd_blt_fast(self->surface, &target, surface->surface, &src, transfer))) {
                        hr = dds_signal_update(self);
                    }
                }
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

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO proper implementation
    // TODO Address of the IDirectDrawSurface7 interface for the DirectDrawSurface object to be detached.
    // If this parameter is NULL, all attached surfaces are detached.

    // See Creating Complex Surfaces and Flipping Chains for details on detachment of implicit surfaces

    // TODO decrement refs

    return DDERR_UNSUPPORTED;
}

HRESULT dds_enum_attached_surfaces(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dds_enum_overlay_z_orders(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    return DDERR_UNSUPPORTED; // TODO
}

HRESULT dds_flip(dds* self, dds* override, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    // TODO validate flags DDSCAPS_VALID

    if ((flags & (DDFLIP_EVEN | DDFLIP_ODD))
        && !(self->desc.ddsCaps.dwCaps & DDSCAPS_OVERLAY)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER)
        || !(self->desc.ddsCaps.dwCaps & DDSCAPS_FLIP)) {
        return DDERR_NOTFLIPPABLE;
    }

    // TODO mip-map

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = dds_can_flip(self))) {
        if (SUCCEEDED(hr = dds_wait_for_vertical_blank(self, NULL, flags & DDFLIP_WAIT))) {
            if (SUCCEEDED(hr = ddsfc_flip(self->chain, override))) {
                hr = dds_signal_update(self);
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_get_attached_surface(dds* self, DDSCAPS2* caps, dds** surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (caps == NULL || surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (caps->dwCaps == DDSCAPS_NONE && caps->dwCaps2 == DDSCAPS2_NONE
        && caps->dwCaps3 == DDSCAPS3_NONE && caps->dwCaps4 == DDSCAPS4_NONE) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    dds* match = NULL;
    const u32 item_count = connector_get_count(self->attachments);
    for (u32 i = 0; i < item_count; i++) {
        MAKETYPE(iddsconn, connector);
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
                    // TODO correct error code. Multiple matches are not allowed?
                    EXITCODE(DDERR_INVALIDPARAMS)
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    HRESULT hr = DD_OK;

    if (flags & DDGBS_CANBLT) {
        u32 count = 0;
        if (SUCCEEDED(hr = ddsd_get_lock_count(self->surface, &count))) {
            if (count == 0) {
                if (self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OVERLAY)) {
                    return ddg_is_ready(self->graphics, FALSE);
                }

                return hr;
            }

            return DDERR_SURFACEBUSY;
        }
    }
    else if (flags & DDGBS_ISBLTDONE) {
        if (self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OVERLAY)) {
            return ddg_is_ready(self->graphics, FALSE);
        }
    }

    return hr;
}

HRESULT dds_get_caps(dds* self, DDSCAPS2* caps) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (caps == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    CopyMemory(caps, &self->desc.ddsCaps, sizeof(DDSCAPS2));

    return DD_OK;
}

HRESULT dds_get_clipper(dds* self, iddcconn* clipper) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (clipper == NULL) {
        return DDERR_INVALIDPARAMS;
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) {
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) {
        return DDERR_NOCOLORKEY;
    }

    if (flags & (DDCKEY_DESTOVERLAY | DDCKEY_SRCOVERLAY)) {
        if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_OVERLAY)) {
            return DDERR_NOTAOVERLAYSURFACE;
        }
    }

    if (flags & DDCKEY_DESTBLT) {
        if (self->desc.dwFlags & DDSD_CKDESTBLT) {
            CopyMemory(key, &self->desc.ddckCKDestBlt, sizeof(DDCOLORKEY));
            return DD_OK;
        }

        return DDERR_NOCOLORKEY;
    }

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

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (hdc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (SUCCEEDED(hr = ddg_is_ready(self->graphics, FALSE))) {
        HDC dc;
        if (SUCCEEDED(hr = ddsd_get_dc(self->surface, &dc))) {
            if (self->desc.ddsCaps.dwCaps & DDSCAPS_PALETTE) {
                // It is not explicitly stated in the documentation,
                // however, palettized surfaces that have no explicitly attached palette
                // must use the the primary surface palette, if it exists.

                ddp* palette = self->palette.instance == NULL
                    ? (self->instance->primary == NULL
                        ? NULL : self->instance->primary->palette.instance)
                    : self->palette.instance;

                // TODO indexed palettes
                if (palette == NULL) {
                    ddsd_release_dc(self->surface, dc);
                    EXITCODE(DDERR_CANTCREATEDC);
                }

                SetDIBColorTable(dc, 0, palette->count, palette->quads);
                dds_set_palette_entries(self, palette->count, palette->quads, palette->lookup);
            }

            *hdc = dc;
        }
    }

exit:
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (palette == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (format == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (format->dwSize != sizeof(DDPIXELFORMAT)) {
        return DDERR_INVALIDPARAMS;
    }

    CopyMemory(format, &self->desc.ddpfPixelFormat, sizeof(DDPIXELFORMAT));

    return DD_OK;
}

HRESULT dds_get_surface_desc(dds* self, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
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

HRESULT dds_initialize(dds* self, const GUID* riid, dd* object, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL || desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    // TODO: use validation rules from dd_create_surface (refactoring needed).
    if (FAILED(ddpixelformat_validate(&desc->ddpfPixelFormat))) {
        return DDERR_INVALIDPIXELFORMAT;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    self->instance = object;
    self->graphics = object->graphics;
    CopyMemory(&self->desc, desc, sizeof(DDSURFACEDESC2));
    CopyMemory(&self->mode, &self->instance->cooperation.mode, sizeof(DEVMODEA));

    // TODO: use validation rules from dd_create_surface (refactoring needed).

    // TODO DDERR_DDSCAPSCOMPLEXREQUIRED

    const u32 track = self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT);

    ddsd* surface = NULL;
    if (SUCCEEDED(hr = ddsd_create(self->manager->allocator, object->blitter, &surface))) {
        if (SUCCEEDED(hr = ddsd_initialize(surface, &self->desc))) {
            self->surface = surface;

            // TODO Refactoring: All of this should be inside initialize!!
            {
                // TODO support 1, 2, 4 bit surfaces for non-primary.
                // Use paint.net to create test images with 1, 2, 4 bit color depth and verify
                // the behavior of the original DirectDraw implementation.

                if (self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT)) {
                    u32 flags = DDPCAPS_NONE;

                    switch (self->desc.ddpfPixelFormat.dwRGBBitCount) {
                    case 1:
                    case 2:
                    case 4: {
                        // TODO clean-up...
                        EXITCODE(DDERR_UNSUPPORTEDFORMAT);
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
                        if (SUCCEEDED(hr = ddsd_get_palette(self->surface, count, quads))) {
                            if (SUCCEEDED(hr = rgb_quad_to_palette_entry(quads, count, entries))) {
                                iddp* instance = NULL;
                                if (SUCCEEDED(hr = dd_create_palette(self->instance, flags, entries, &instance))) {
                                    self->palette.instance = instance->instance;
                                    CopyMemory(&self->palette.id, &instance->id, sizeof(GUID));
                                    hr = ddp_register_surface(self->palette.instance, self);
                                }
                            }
                        }

                        // TODO handle failure of palette creation
                    }break;
                    }

                    hr = ddsd_set_region_tracking(self->surface, track);
                }

                //// TODO make this a setting. Should propagate to back buffers?

                //// Under normal circumstances DirectDraw uses the window DC data as initial
                //// (visual) state of the primary surface, so that there is no black rectangle
                //// visible to the user before first flip/blit/write into the surface,
                //// unless the window was cleared black prior to DirectDraw calls.
                //MAKETYPE(RECT, rect);
                //// TODO is this needed in case of tracking rimary surface + back buffers blit/dc/lock regions?
                //if (SUCCEEDED(hr = ddsd_get_rect(self->surface, &rect))) {
                //    HDC sdc = NULL;
                //    if (SUCCEEDED(hr = ddsd_get_dc(self->surface, &sdc))) {
                //        HWND hwnd = self->instance->cooperation.hwnd;
                //        FillRect(sdc, &rect, (HBRUSH)GetClassLongPtrA(hwnd, GCLP_HBRBACKGROUND));
                //        hr = ddsd_release_dc(self->surface, sdc);
                //    }
                //}
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

        if (self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT)) {
            back.ddsCaps.dwCaps &= ~(DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT);
        }

        for (u32 i = 0; i < self->desc.dwBackBufferCount; i++) {
            dds* instance = NULL;
            // TODO. Should this be done using dd_create_surface?
            // Should the back buffers to be enumeratable as surfaces directly via dd_enumerate_surfaces?
            if (SUCCEEDED(hr = dds_create(self->manager, DDS_IMPLICIT, &instance))) {
                if (SUCCEEDED(hr = dds_initialize(instance, riid, self->instance, &back))) {
                    idds* intfc = NULL;
                    if (SUCCEEDED(hr = dds_query_interface(instance, riid, &intfc))) {
                        iddsconn connector;
                        connector.instance = instance;
                        CopyMemory(&connector.id, riid, sizeof(GUID));
                        if (SUCCEEDED(hr = connector_add_item(self->attachments, &connector))) {
                            if (SUCCEEDED(hr = ddsfc_add_surface(self->chain, instance))) {
                                if (SUCCEEDED(hr = ddsd_set_region_tracking(instance->surface, track))) {
                                    continue;
                                }
                            }
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

    return (self->flags & DDS_LOST) ? DDERR_SURFACELOST : DD_OK;
}

HRESULT dds_lock(dds* self, RECT* rect, DDSURFACEDESC2* desc, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO ModeX

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = ddg_is_ready(self->graphics, flags & DDLOCK_WAIT))) {
        RECT lock;
        if (SUCCEEDED(hr = dds_get_rect(self, rect, &lock))) {
            if (!IsValidRect(&lock)) {
                return DDERR_INVALIDRECT;
            }

            EnterCriticalSection(&self->lock);

            hr = ddsd_lock(self->surface, &lock, desc, flags);

            LeaveCriticalSection(&self->lock);
        }
    }

    return hr;
}

HRESULT dds_release_dc(dds* self, HDC hdc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (hdc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_NODC;
    }

    if (self->desc.dwFlags & DDSD_LPSURFACE) {
        return DDERR_UNSUPPORTED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddsd_release_dc(self->surface, hdc))) {
        hr = dds_signal_update(self);
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

    if (!(self->flags & DDS_LOST)) {
        return DD_OK;
    }

    if (self->flags & DDS_IMPLICIT) {
        return DDERR_IMPLICITLYCREATED;
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) {
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // DirectDraw only allows surfaces to be restored when the current display mode
    // matches the display mode at the time of surface creation (initialization).
    // In practice this means that the surface width, height, and depth of the surfaces
    // before they are lost and after they are restored are the same, therefore
    // it is only needed to mark them as found, or not lost again.

    MAKEDEVMODEA(mode);
    if (SUCCEEDED(hr = sugar_get_display_mode(self->manager, &mode))) {
        if (!devmodea_equal(&self->mode, &mode)) {
            EXITCODE(DDERR_WRONGMODE);
        }

        hr = dds_restore_surface(self);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_set_clipper(dds* self, iddcconn* clipper) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (clipper == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) {
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) {
        return DDERR_INVALIDSURFACETYPE;
    }

    if (flags & (DDCKEY_DESTOVERLAY | DDCKEY_SRCOVERLAY)) {
        if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_OVERLAY)) {
            return DDERR_NOTAOVERLAYSURFACE;
        }
    }

    // TODO mip maps

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

HRESULT dds_set_overlay_position(dds* self, s32 x, s32 y) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_OVERLAY)) {
        return DDERR_NOTAOVERLAYSURFACE;
    }

    // TODO See Positioning Overlay Surfaces
    // TODO DDERR_INVALIDPOSITION

    return DDERR_UNSUPPORTED;
}

HRESULT dds_set_palette(dds* self, iddpconn* palette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (palette == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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
            ddsd_remove_palette(self->surface);

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
        ddsd_remove_palette(self->surface);

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

        ddsd_set_palette(self->surface, instance->count, instance->quads, instance->lookup);
    }

    ddsd_change_uniqueness_value(self->surface);

    // TODO: how to handle backbuffers? propagate the palette? Need tests...
    // What if backbuffer has it's own palete... Is it posible?

    if (self->desc.ddsCaps.dwCaps & DDSCAPS_COMPLEX) {
        const u32 item_count = connector_get_count(self->attachments);
        for (u32 i = 0; i < item_count; i++) {
            MAKETYPE(iddsconn, connector);
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    RECT lock;
    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = dds_get_rect(self, rect, &lock))) {
        if (!IsValidRect(&lock)) {
            return DDERR_INVALIDRECT;
        }

        EnterCriticalSection(&self->lock);

        if (SUCCEEDED(hr = ddsd_unlock(self->surface, &lock))) {
            hr = dds_signal_update(self);
        }

        LeaveCriticalSection(&self->lock);
    }

    return hr;
}

HRESULT dds_update_overlay(dds* self, iddsconn* connector,
    RECT* src, iddsconn* surface, RECT* dst, u32 flags, DDOVERLAYFX* effects) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (connector == NULL || surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (self->instance != surface->instance->instance) {
        return DDERR_DEVICEDOESNTOWNSURFACE;
    }

    // TODO check DDOVER_HIDE and DDOVER_SHOW vs DDSCAPS_VISIBLE ?

    // TODO See Positioning Overlay Surfaces
    // TODO DDERR_INVALIDPOSITION

    HRESULT hr = DD_OK;
    if (flags & DDOVER_HIDE) {
        self->desc.ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
        if (self->overlay.target.instance == self->instance->primary) {
            hr = ddg_signal_update(self->graphics);
        }

        return hr;
    }

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
                    dds_unregister_overlay(instance, connector);

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

                dds_unregister_overlay(instance, connector);
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
                dds_register_overlay(target, connector);
            }

        exit:
                self->overlay.flags = flags;
                CopyMemory(&self->overlay.src, &source, sizeof(RECT));
                CopyMemory(&self->overlay.dst, &destination, sizeof(RECT));

                if (flags & DDOVER_DDFX) {
                    CopyMemory(&self->overlay.effects, effects, sizeof(DDOVERLAYFX));
                }

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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (!(self->desc.ddsCaps.dwCaps & DDSCAPS_OVERLAY)) {
        return DDERR_NOTAOVERLAYSURFACE;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_update_overlay_z_order(dds* self, u32 flags, iddsconn* surface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (surface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO Overlay Z-Orders

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_dd_interface(dds* self, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO validate caps

    return ddsd_page_unlock(self->surface);
}

HRESULT dds_set_surface_desc(dds* self, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO Updating Surface Characteristics in documentation

    return ddsd_set_surface_desc(self->surface, desc);
}

HRESULT dds_set_private_data(dds* self, const GUID* tag, void* data, u32 size, u32 flags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (tag == NULL || data == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO validate size
    // TODO validate flags

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_private_data(dds* self, const GUID* tag, void* buffer, u32* size) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (tag == NULL || size == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_free_private_data(dds* self, const GUID* tag) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (tag == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_uniqueness_value(dds* self, u32* value) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (value == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_priority(dds* self, u32* priority) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (priority == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
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

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_lod(dds* self, u32* lod) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (lod == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    // TODO

    return DDERR_UNSUPPORTED;
}

HRESULT dds_get_rect(dds* self, RECT* rect, RECT* result) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (result == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    result->left = rect == NULL ? 0 : rect->left;
    result->top = rect == NULL ? 0 : rect->top;
    result->right = rect == NULL ? (s32)self->desc.dwWidth : rect->right;
    result->bottom = rect == NULL ? (s32)self->desc.dwHeight : rect->bottom;

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

HRESULT dds_query_gamma_control(dds* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->gamma != NULL) {
        return ddgc_query_interface(self->gamma, riid, object);
    }

    ddgc* gamma = NULL;
    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = ddgc_create(self->manager, &gamma))) {
        if (SUCCEEDED(hr = ddgc_initialize(gamma, self))) {
            if (SUCCEEDED(hr = ddgc_query_interface(gamma, riid, object))) {
                // TODO what is ref count here? Should be 2?
                self->gamma = gamma;
                return hr;
            }
        }

        ddgc_release(gamma, RELEASE_NONE);
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

HRESULT dds_remove_gamma_control(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    self->gamma = NULL;

    return DD_OK;
}

HRESULT dds_remove_palette(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ZeroMemory(&self->palette, sizeof(iddpconn));

    return ddsd_remove_palette(self->surface);
}

HRESULT dds_set_lost(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->flags & DDS_LOST) {
        return DD_OK;
    }

    EnterCriticalSection(&self->lock);

    self->flags |= DDS_LOST;

    HRESULT hr = DD_OK;
    const u32 item_count = connector_get_count(self->attachments);
    for (u32 i = 0; i < item_count; i++) {
        MAKETYPE(iddsconn, connector);
        if (SUCCEEDED(hr = connector_get_item(self->attachments, i, &connector))) {
            if (SUCCEEDED(hr = dds_set_lost(connector.instance))) {
                continue;
            }
        }

        goto exit;
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT dds_set_palette_entries(dds* self, u32 count, RGBQUAD* quads, plt* lookup) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (quads == NULL || lookup == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = ddsd_set_palette(self->surface, count, quads, lookup))) {
        if (self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT)) {
            hr = dds_signal_update(self);
        }
    }

    return hr;
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

    return connector_remove_item(self->overlays, overlay);
}

HRESULT dds_signal_update(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if ((self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT))
        || (self->desc.ddsCaps.dwCaps & (DDSCAPS_OVERLAY | DDSCAPS_VISIBLE))) {
        return ddg_signal_update(self->graphics);
    }

    return DD_OK;
}

HRESULT dds_wait_for_vertical_blank(dds* self, dds* surface, bool wait) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    if (self->graphics == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    bool waitable = (self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT))
        || (self->desc.ddsCaps.dwCaps & (DDSCAPS_OVERLAY | DDSCAPS_VISIBLE));

    if (!waitable && surface != NULL) {
        waitable = (surface->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT))
            || (surface->desc.ddsCaps.dwCaps & (DDSCAPS_OVERLAY | DDSCAPS_VISIBLE));
    }

    if (waitable) {
        return ddg_is_ready(self->graphics, wait);
    }

    return DD_OK;
}

HRESULT dds_can_flip(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->flags & DDS_LOST) {
        return DDERR_SURFACELOST;
    }

    const u32 item_count = connector_get_count(self->attachments);
    for (u32 i = 0; i < item_count; i++) {
        MAKETYPE(iddsconn, connector);
        if (SUCCEEDED(connector_get_item(self->attachments, i, &connector))) {
            if (connector.instance->flags & DDS_LOST) {
                return DDERR_SURFACELOST;
            }
        }
    }

    return DD_OK;
}

HRESULT dds_restore_surface(dds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    dd* exclusive = NULL;
    if (SUCCEEDED(hr = sugar_get_exclusive(self->manager, &exclusive))) {
        if (exclusive == NULL) {
            if (self->instance->cooperation.flags & DDSCL_EXCLUSIVE) {
                return DDERR_WRONGMODE;
            }
        }
        else if (self->instance != exclusive) {
            return DDERR_WRONGMODE;
        }

        // TODO DDERR_NOEXCLUSIVEMODE
        // TODO DDERR_INCOMPATIBLEPRIMARY

        if (self->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT)) {
            if (self->desc.ddsCaps.dwCaps & DDSCAPS_FLIP) {
                if (self->instance != exclusive) {
                    return DDERR_SURFACELOST;
                }
            }
        }

        const u32 item_count = connector_get_count(self->attachments);
        for (u32 i = 0; i < item_count; i++) {
            MAKETYPE(iddsconn, connector);
            if (SUCCEEDED(hr = connector_get_item(self->attachments, i, &connector))) {
                if (connector.instance->flags & DDS_IMPLICIT) {
                    if (SUCCEEDED(hr = dds_restore_surface(connector.instance))) {
                        continue;
                    }
                }
            }

            goto exit;
        }

        if (SUCCEEDED(hr = ddsd_restore_surface(self->surface))) {
            self->flags &= ~DDS_LOST;
        }
    }

exit:
    return hr;
}
