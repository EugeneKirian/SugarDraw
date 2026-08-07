#include "dd.h"
#include "ddc.h"
#include "iddc.h"

#define RGNDATASIZE(region)     (sizeof(RGNDATAHEADER) + (sizeof(RECT) * region->rdh.nCount))

static HRESULT ddc_clip_region(ddc* self, RECT* rect, RGNDATA* region);
static HRESULT ddc_compact_region(ddc* self, RGNDATA* region);
static HRESULT ddc_update_hwnd_region(ddc* self);

HRESULT ddc_create(sugar* manager, const GUID* rclsid, ddc** object) {
    if (manager == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsEqualGUID(&CLSID_DirectDrawClipper, rclsid)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    HRESULT hr = DD_OK;
    ddc* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWCLIPPER, sizeof(ddc), &instance))) {
        instance->manager = manager;
        CopyMemory(&instance->id, rclsid, sizeof(GUID));
        if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAWCLIPPER, &instance->interfaces))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void ddc_release(ddc* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const s32 count = intfc_get_count(self->interfaces);
            for (s32 i = 0; i < count; i++) {
                iddc* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    iddc_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            if (self->instance == NULL) {
                sugar_remove_ddc(self->manager, self);
            }
            else {
                dd_remove_clipper(self->instance, self);
            }
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT ddc_get_interface(ddc* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return intfc_query_item(self->interfaces, riid, object);
}

HRESULT ddc_query_interface(ddc* self, const GUID* riid, void** object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    iddc* instance = NULL;
    if (SUCCEEDED(hr = intfc_query_item(self->interfaces, riid, &instance))) {
        iddc_add_ref(instance);
        *object = instance;
        goto exit;
    }

    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IDirectDrawClipper, riid)) {
        if (SUCCEEDED(hr = iddc_create(self->manager, riid, &instance))) {
            instance->instance = self;
            if (SUCCEEDED(hr = ddc_add_ref(self, instance))) {
                *object = instance;
                goto exit;
            }

            iddc_release(instance);
            goto exit;
        }
    }

    hr = E_NOINTERFACE;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddc_add_ref(ddc* self, iddc* object) {
    return intfc_add_item(self->interfaces, &object->id, object);
}

HRESULT ddc_remove_ref(ddc* self, iddc* object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            ddc_release(self, RELEASE_NOTIFY);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddc_get_clip_list(ddc* self, RECT* rect, RGNDATA* region, u32* size) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect != NULL) {
        if (!IsValidRect(rect)) {
            return DDERR_INVALIDRECT;
        }
    }

    if (size == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO multi-monitor suport

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (self->hwnd == NULL) {
        if (self->region == NULL) {
            EXITCODE(DDERR_NOCLIPLIST);
        }

        const u32 length = RGNDATASIZE(self->region);
        if (region == NULL) {
            *size = length;
            goto exit;
        }

        if (*size < length) {
            EXITCODE(DDERR_REGIONTOOSMALL);
        }

        CopyMemory(region, self->region, RGNDATASIZE(self->region));
        hr = ddc_clip_region(self, rect, region);
    }
    else {
        if (SUCCEEDED(hr = ddc_update_hwnd_region(self))) {
            const u32 length = RGNDATASIZE(self->region);
            if (region == NULL) {
                *size = length;
                goto exit;
            }

            if (*size < length) {
                EXITCODE(DDERR_REGIONTOOSMALL);
            }

            CopyMemory(region, self->region, length);
            hr = ddc_clip_region(self, rect, region);
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddc_get_hwnd(ddc* self, HWND* hwnd) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (!self->initialized) {
        return DDERR_NOTINITIALIZED;
    }

    if (hwnd == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->hwnd == NULL) {
        return DDERR_NOHWND;
    }

    EnterCriticalSection(&self->lock);

    *hwnd = self->hwnd;

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddc_initialize(ddc* self, dd* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->initialized) {
        return DDERR_ALREADYINITIALIZED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // According to the documentation:
    // Creating DirectDrawClipper Objects with CoCreateInstance
    // the object passed in can be NULL, in which case it is a driver-independent object.

    if (object != NULL) {
        // Change the ownership of the clipper.
        if (SUCCEEDED(hr = dd_attach_clipper(object, self))) {
            sugar_remove_ddc(self->manager, self);
            self->instance = object;
        }
    }

    if (SUCCEEDED(hr)) {
        self->initialized = TRUE;
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddc_is_clip_list_changed(ddc* self, bool* changed) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (!self->initialized) {
        return DDERR_NOTINITIALIZED;
    }

    if (changed == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    *changed = TRUE;

    return DD_OK;
}

HRESULT ddc_set_clip_list(ddc* self, RGNDATA* region) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (!self->initialized) {
        return DDERR_NOTINITIALIZED;
    }

    if (self->hwnd != NULL) {
        return DDERR_CLIPPERISUSINGHWND;
    }

    if (region != NULL) {
        if (region->rdh.nCount == 0
            || region->rdh.dwSize != sizeof(RGNDATAHEADER)
            || region->rdh.iType != RDH_RECTANGLES) {
            return DDERR_INVALIDCLIPLIST;
        }
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (region != NULL) {
        if (self->region != NULL) {
            allocator_free(self->manager->allocator, self->region);
            self->region = NULL;
        }

        const u32 size = RGNDATASIZE(region);
        if (SUCCEEDED(hr = allocator_allocate(self->manager->allocator,
            MEM_TAG_DIRECTDRAWCLIPPER, size, &self->region))) {
            CopyMemory(self->region, region, size);
        }
    }
    else {
        if (self->region != NULL) {
            allocator_free(self->manager->allocator, self->region);
        }

        self->region = NULL;
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddc_set_hwnd(ddc* self, HWND hwnd) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (!self->initialized) {
        return DDERR_NOTINITIALIZED;
    }

    if (hwnd == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (hwnd != NULL) {
        if (!IsWindow(hwnd)) {
            return DDERR_INVALIDPARAMS;
        }
    }

    // TODO multi-monitor suport

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (hwnd == NULL) {
        if (self->region != NULL) {
            allocator_free(self->manager->allocator, self->region);
            self->region = NULL;
        }

        self->hwnd = NULL;
    }
    else if (self->hwnd != hwnd) {
        RGNDATA* region = NULL;
        if (SUCCEEDED(hr = allocator_allocate(self->manager->allocator,
            MEM_TAG_DIRECTDRAWCLIPPER, sizeof(RGNDATAHEADER) + sizeof(RECT), &region))) {
            region->rdh.dwSize = sizeof(RGNDATAHEADER);
            region->rdh.iType = RDH_RECTANGLES;
            region->rdh.nCount = 1;
            region->rdh.nRgnSize = sizeof(RECT);

            RECT rect;
            GetClientRect(hwnd, &rect);
            ClientToScreen(hwnd, (POINT*)&rect.left);
            ClientToScreen(hwnd, (POINT*)&rect.right);

            CopyMemory(&region->rdh.rcBound, &rect, sizeof(RECT));
            CopyMemory(&region->Buffer, &rect, sizeof(RECT));

            if (self->region != NULL) {
                allocator_free(self->manager->allocator, self->region);
            }

            self->hwnd = hwnd;
            self->region = region;
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddc_get_region(ddc* self, RGNDATA** region) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (region == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;

    if (self->hwnd != NULL) {
        hr = ddc_update_hwnd_region(self);
    }

    *region = self->region;

    return hr;
}

HRESULT ddc_clip_region(ddc* self, RECT* rect, RGNDATA* region) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (region == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (rect == NULL) {
        return DD_OK;
    }

    // The rect is exactly matching or covering the entire region boundaries.
    if (rect->left <= region->rdh.rcBound.left
        && rect->top <= region->rdh.rcBound.top
        && rect->right >= region->rdh.rcBound.right
        && rect->bottom >= region->rdh.rcBound.bottom) {
        return DD_OK;
    }

    for (u32 i = 0; i < region->rdh.nCount; i++) {
        RECT* current =
            (RECT*)(region->Buffer + i * sizeof(RECT));

        current->left = max(rect->left, current->left);
        current->right = min(rect->right, current->right);
        current->top = max(rect->top, current->top);
        current->bottom = min(rect->bottom, current->bottom);
    }

    return ddc_compact_region(self, region);
}

HRESULT ddc_compact_region(ddc* self, RGNDATA* region) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (region == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    u32 target = 0;
    RECT* rects = (RECT*)region->Buffer;

    for (u32 i = 0; i < region->rdh.nCount; i++) {
        if (rects[i].bottom > rects[i].top && rects[i].right > rects[i].left) {
            if (target != i) {
                CopyMemory(&rects[target], &rects[i], sizeof(RECT));
            }

            target++;
        }
    }

    region->rdh.nCount = target;

    return DD_OK;
}

HRESULT ddc_update_hwnd_region(ddc* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->hwnd == NULL) {
        return DDERR_NOHWND;
    }

    RECT rect;
    GetClientRect(self->hwnd, &rect);
    ClientToScreen(self->hwnd, (POINT*)&rect.left);
    ClientToScreen(self->hwnd, (POINT*)&rect.right);

    CopyMemory(&self->region->rdh.rcBound, &rect, sizeof(RECT));
    CopyMemory(&self->region->Buffer, &rect, sizeof(RECT));

    return DD_OK;
}
