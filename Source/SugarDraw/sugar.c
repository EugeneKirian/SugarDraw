#include "dd.h"
#include "utilities.h"

HRESULT sugar_create(allocator* allocator, logger* logger, sugar** object) {
    if (allocator == NULL || logger == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    sugar* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_SUGAR, sizeof(sugar), &instance))) {
        instance->allocator = allocator;
        instance->logger = logger;
        InitializeCriticalSection(&instance->lock);
        if (SUCCEEDED(hr = arr_create(allocator, MEM_TAG_SUGAR, &instance->items))) {
            logger_log(logger, LOG_LEVEL_TRACE, "SugarDraw started successfully.");

            {
                DEVMODEA* mode = &instance->display_modes.initial;
                mode->dmSize = sizeof(DEVMODEA);

                HDC hdc = GetDC(NULL);
                mode->dmPelsWidth = GetDeviceCaps(hdc, HORZRES);
                mode->dmPelsHeight = GetDeviceCaps(hdc, VERTRES);
                mode->dmBitsPerPel = GetDeviceCaps(hdc, BITSPIXEL);
                mode->dmDisplayFrequency = GetDeviceCaps(hdc, VREFRESH);
                mode->dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
                ReleaseDC(NULL, hdc);

                CopyMemory(&instance->display_modes.current, mode, sizeof(DEVMODEA));
            }

            *object = instance;
            return hr;
        }

        logger_log(logger, LOG_LEVEL_ERROR, "SugarDraw could not start: %s.", hresult_to_string(hr));
        sugar_release(instance);
    }

    return hr;
}

void sugar_release(sugar* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);
        logger_log(self->logger, LOG_LEVEL_TRACE, "SugarDraw is shutting down.");

        if (self->items != NULL) {
            const s32 item_count = arr_get_count(self->items);
            for (s32 i = 0; i < item_count; i++) {
                dd* instance = NULL;
                if (SUCCEEDED(arr_get_item(self->items, i, &instance))) {
                    dd_release(instance, RELEASE_NONE);
                }
            }

            arr_release(self->items);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self);
        logger_release(self->logger);
        allocator_release(self->allocator);
    }
}

HRESULT sugar_create_direct_draw(sugar* self, const GUID* rclsid, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rclsid == NULL || riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsEqualGUID(&CLSID_DirectDraw, rclsid)
        && !IsEqualGUID(&CLSID_DirectDraw7, rclsid)) {
        return E_NOINTERFACE;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    dd* instance = NULL;
    if (SUCCEEDED(hr = dd_create(self, rclsid, &instance))) {
        if (SUCCEEDED(hr = dd_initialize(instance, riid))) {
            idd* intfc = NULL;
            if (SUCCEEDED(hr = dd_query_interface(instance, riid, &intfc))) {
                if (SUCCEEDED(hr = arr_add_item(self->items, instance))) {
                    *object = intfc;
                    goto exit;
                }
            }
        }

        dd_release(instance, RELEASE_NONE);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT sugar_remove_direct_draw(sugar* self, dd* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    const s32 item_count = arr_get_count(self->items);
    for (s32 i = 0; i < item_count; i++) {
        dd* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->items, i, &instance))) {
            if (instance == object) {
                hr = arr_remove_item(self->items, i);
                goto exit;
            }
        }
    }

    hr = DDERR_NOTFOUND;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT sugar_get_display_mode(sugar* self, DEVMODEA* mode) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (mode == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    CopyMemory(mode, &self->display_modes.current, sizeof(DEVMODEA));

    return DD_OK;
}

HRESULT sugar_set_display_mode(sugar* self, u32 width, u32 height, u32 bpp, u32 rate) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    DEVMODEA mode;
    ZeroMemory(&mode, sizeof(DEVMODEA));

    mode.dmSize = sizeof(DEVMODEA);
    mode.dmPelsWidth = width;
    mode.dmPelsHeight = height;
    mode.dmBitsPerPel = bpp;
    mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    if (rate != 0) {
        mode.dmDisplayFrequency = rate;
        mode.dmFields |= DM_DISPLAYFREQUENCY;
    }

    if (ChangeDisplaySettingsA(&mode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL) {
        CopyMemory(&self->display_modes.current, &mode, sizeof(DEVMODEA));
        return DD_OK;
    }

    return DDERR_UNSUPPORTEDMODE;
}
