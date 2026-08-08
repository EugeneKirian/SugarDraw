#include "ddcc.h"
#include "dds.h"
#include "iddcc.h"

HRESULT ddcc_create(sugar* manager, ddcc** object) {
    if (manager == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    ddcc* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWCOLORCONTROL, sizeof(ddcc), &instance))) {
        instance->manager = manager;
        if (SUCCEEDED(hr = intfc_create(manager->allocator, MEM_TAG_DIRECTDRAWCOLORCONTROL, &instance->interfaces))) {
            InitializeCriticalSection(&instance->lock);

            instance->control.dwSize = sizeof(DDCOLORCONTROL);
            instance->control.dwFlags = DDCOLOR_COLORENABLE | DDCOLOR_GAMMA | DDCOLOR_SHARPNESS
                | DDCOLOR_SATURATION | DDCOLOR_HUE | DDCOLOR_CONTRAST | DDCOLOR_BRIGHTNESS;

            instance->control.lBrightness = 750;
            instance->control.lContrast = 10000;
            instance->control.lHue = 0;
            instance->control.lSaturation = 10000;
            instance->control.lSharpness = 5;
            instance->control.lGamma = 1;
            instance->control.lColorEnable = TRUE;

            *object = instance;
            return hr;
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void ddcc_release(ddcc* self, u32 flags) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->interfaces != NULL) {
            const u32 item_count = intfc_get_count(self->interfaces);
            for (u32 i = 0; i < item_count; i++) {
                iddcc* instance = NULL;
                if (SUCCEEDED(intfc_get_item(self->interfaces, i, &instance))) {
                    iddcc_release(instance);
                }
            }

            intfc_release(self->interfaces);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        if (flags & RELEASE_NOTIFY) {
            if (self->instance != NULL) {
                dds_remove_color_control(self->instance);
            }
        }

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT ddcc_get_interface(ddcc* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return intfc_query_item(self->interfaces, riid, object);
}

HRESULT ddcc_query_interface(ddcc* self, const GUID* riid, void** object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    iddcc* instance = NULL;
    if (SUCCEEDED(hr = intfc_query_item(self->interfaces, riid, &instance))) {
        iddcc_add_ref(instance);
        *object = instance;
        goto exit;
    }

    if (IsEqualGUID(&IID_IUnknown, riid)
        || IsEqualGUID(&IID_IDirectDrawColorControl, riid)) {
        if (SUCCEEDED(hr = iddcc_create(self->manager, riid, &instance))) {
            instance->instance = self;
            if (SUCCEEDED(hr = ddcc_add_ref(self, instance))) {
                *object = instance;
                goto exit;
            }

            iddcc_release(instance);
            goto exit;
        }
    }

    hr = E_NOINTERFACE;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddcc_add_ref(ddcc* self, iddcc* object) {
    return intfc_add_item(self->interfaces, &object->id, object);
}

HRESULT ddcc_remove_ref(ddcc* self, iddcc* object) {
    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = intfc_remove_item(self->interfaces, &object->id))) {
        if (intfc_get_count(self->interfaces) == 0) {
            ddcc_release(self, RELEASE_NOTIFY);
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddcc_get_color_controls(ddcc* self, DDCOLORCONTROL* control) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance = NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (control == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (control->dwSize != sizeof(DDCOLORCONTROL)) {
        return DDERR_INVALIDPARAMS;
    }

    if (!(self->instance->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OVERLAY))) {
        return DDERR_UNSUPPORTED;
    }

    EnterCriticalSection(&self->lock);

    CopyMemory(control, &self->control, sizeof(DDCOLORCONTROL));

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddcc_set_color_controls(ddcc* self, DDCOLORCONTROL* control) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->instance = NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (control == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (control->dwSize != sizeof(DDCOLORCONTROL)) {
        return DDERR_INVALIDPARAMS;
    }

    if (control->dwFlags == DDCOLOR_NONE || !(control->dwFlags & DDCOLOR_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    if (control->dwFlags & DDCOLOR_BRIGHTNESS) {
        if (control->lBrightness < 0 || control->lBrightness > 10000) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (control->dwFlags & DDCOLOR_CONTRAST) {
        if (control->lContrast < 0 || control->lContrast > 20000) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (control->dwFlags & DDCOLOR_HUE) {
        if (control->lHue < -180 || control->lHue > 180) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (control->dwFlags & DDCOLOR_SATURATION) {
        if (control->lSaturation < 0 || control->lSaturation > 20000) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (control->dwFlags & DDCOLOR_SHARPNESS) {
        if (control->lSharpness < 0 || control->lSharpness > 10) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (control->dwFlags & DDCOLOR_GAMMA) {
        if (control->lGamma < 1 || control->lGamma > 500) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (control->dwFlags & DDCOLOR_COLORENABLE) {
        if (control->lColorEnable < FALSE || control->lColorEnable > TRUE) {
            return DDERR_INVALIDPARAMS;
        }
    }

    if (!(self->instance->desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OVERLAY))) {
        return DDERR_UNSUPPORTED;
    }

    EnterCriticalSection(&self->lock);

    CopyMemory(&self->control, control, sizeof(DDCOLORCONTROL));

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT ddcc_initialize(ddcc* self, dds* instance) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (instance == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->instance != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    EnterCriticalSection(&self->lock);

    self->instance = instance;

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}
