#include "dd.h"
#include "ddc.h"
#include "converter.h"

#define CDS_NONE            0x00000000

static HRESULT sugar_enumerate_dispaly_modes(sugar* self);

HRESULT sugar_create(allocator* allocator, logger* logger, driver* driver, sugar** object) {
    if (allocator == NULL || logger == NULL || driver == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    sugar* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_SUGAR, sizeof(sugar), &instance))) {
        instance->allocator = allocator;
        instance->logger = logger;
        instance->driver = driver;
        if (SUCCEEDED(sugar_enumerate_dispaly_modes(instance))) {
            if (SUCCEEDED(hr = arr_create(allocator, MEM_TAG_SUGAR, &instance->clippers))) {
                if (SUCCEEDED(hr = arr_create(allocator, MEM_TAG_SUGAR, &instance->items))) {
                    logger_log(logger, LOG_LEVEL_TRACE, "SugarDraw started successfully.");
                    InitializeCriticalSection(&instance->lock);

                    *object = instance;

                    return hr;
                }

                allocator_free(allocator, instance->clippers);
            }

            allocator_free(allocator, instance->modes.modes);
        }

        logger_log(logger, LOG_LEVEL_ERROR,
            "SugarDraw could not start: %s.", hresult_to_string(hr));
        allocator_free(allocator, instance);
    }

    return hr;
}

void sugar_release(sugar* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);
        logger_log(self->logger, LOG_LEVEL_TRACE, "SugarDraw is shutting down.");

        if (self->clippers != NULL) {
            // TODO
        }

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

        // Restore display mode if needed.
        if (!CompareMemory(&self->modes.current, &self->modes.initial, sizeof(DEVMODEA))) {
            const s32 result = ChangeDisplaySettingsA(NULL, CDS_NONE);
            if (result != DISP_CHANGE_SUCCESSFUL) {
                ERR("->ChangeDisplaySettings(NULL, CDS_NONE) -> %s", disp_change_to_string(result));
            }
        }

        if (self->modes.modes != NULL) {
            allocator_free(self->allocator, self->modes.modes);
        }

        // TODO driver

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self);
        logger_release(self->logger);
        allocator_release(self->allocator);
    }
}

HRESULT sugar_set_driver(sugar* self, driver* driver) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (driver == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    // TODO initialize driver with the current settings
    // TODO go through all dd objects and set driver
    // TODO release previous driver

    return DDERR_UNSUPPORTED;
}

HRESULT sugar_create_dd(sugar* self, const GUID* rclsid, const GUID* riid, void** object) {
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

    // TODO. Creation should be done via the COM class factory object
    // TODO. Initialization should happen here...

    dd* instance = NULL;
    if (SUCCEEDED(hr = dd_create(self, rclsid, self->driver, &instance))) {
        if (SUCCEEDED(hr = dd_initialize(instance, NULL))) {
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

HRESULT sugar_remove_dd(sugar* self, dd* object) {
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
                if (SUCCEEDED(hr = arr_remove_item(self->items, i))) {
                    if (self->exclusive == object) {
                        self->exclusive = NULL;
                    }

                    goto exit;
                }
            }
        }
    }

    hr = DDERR_NOTFOUND;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT sugar_create_ddc(sugar* self, const GUID* rclsid, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rclsid == NULL || riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsEqualGUID(&CLSID_DirectDrawClipper, rclsid)) {
        return E_NOINTERFACE;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO. Creation should be done via the COM class factory object

    ddc* instance = NULL;
    if (SUCCEEDED(hr = ddc_create(self, rclsid, &instance))) {
        idd* intfc = NULL;
        if (SUCCEEDED(hr = ddc_query_interface(instance, riid, &intfc))) {
            if (SUCCEEDED(hr = arr_add_item(self->clippers, instance))) {
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

HRESULT sugar_remove_ddc(sugar* self, ddc* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    const s32 item_count = arr_get_count(self->clippers);
    for (s32 i = 0; i < item_count; i++) {
        ddc* instance = NULL;
        if (SUCCEEDED(hr = arr_get_item(self->clippers, i, &instance))) {
            if (instance == object) {
                if (SUCCEEDED(hr = arr_remove_item(self->clippers, i))) {
                    goto exit;
                }
            }
        }
    }

    hr = DDERR_NOTFOUND;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT sugar_get_exclusive(sugar* self, dd** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);

    *object = self->exclusive;

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT sugar_set_exclusive(sugar* self, dd* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    EnterCriticalSection(&self->lock);

    self->exclusive = object;

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT sugar_get_display_mode(sugar* self, DEVMODEA* mode) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (mode == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (mode->dmSize != sizeof(DEVMODEA)) {
        return DDERR_INVALIDPARAMS;
    }

    CopyMemory(mode, &self->modes.current, sizeof(DEVMODEA));

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
    mode.dmDisplayFrequency = rate == 0
        ? self->modes.initial.dmDisplayFrequency : rate;
    mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = sugar_supports_display_mode(self, &mode))) {
        const s32 result = ChangeDisplaySettingsA(&mode, CDS_FULLSCREEN);
        if (result == DISP_CHANGE_SUCCESSFUL) {
            CopyMemory(&self->modes.current, &mode, sizeof(DEVMODEA));
            // TODO do we need to recreate surface to match current display settings?
            // TODO what about multiple displays?
        }
        else {
            hr = DDERR_UNSUPPORTEDMODE;
            ERR("->ChangeDisplaySettings(%s, CDS_FULLSCREEN) -> %s",
                devmodea_to_string(&mode), disp_change_to_string(result));
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT sugar_supports_display_mode(sugar* self, const DEVMODEA* mode) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (mode == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (mode->dmSize != sizeof(DEVMODEA)) {
        return DDERR_INVALIDPARAMS;
    }

    for (u32 i = 0; i < self->modes.count; i++) {
        const DEVMODEA* current = &self->modes.modes[i];
        if ((mode->dmFields & DM_DISPLAYFREQUENCY)
            && (current->dmFields & DM_DISPLAYFREQUENCY)) {
            if (mode->dmDisplayFrequency != current->dmDisplayFrequency) {
                continue;
            }
        }

        // TODO
        // The resolutions and bpp have to come from the setttings.
        // The operating system does not report all suported.
        // And does all kind of tricks for 32-bit apps, and in compatibility mode.

        //if ((mode->dmFields & DM_BITSPERPEL)
        //    && (current->dmFields & DM_BITSPERPEL)) {
        //    if (mode->dmBitsPerPel != current->dmBitsPerPel) {
        //        continue;
        //    }
        //}

        if ((mode->dmFields & DM_PELSWIDTH)
            && (current->dmFields & DM_PELSWIDTH)) {
            if (mode->dmPelsWidth != current->dmPelsWidth) {
                continue;
            }
        }

        if ((mode->dmFields & DM_PELSHEIGHT)
            && (current->dmFields & DM_PELSHEIGHT)) {
            if (mode->dmDisplayFrequency != current->dmDisplayFrequency) {
                continue;
            }
        }

        return DD_OK;
    }

    return DDERR_UNSUPPORTEDMODE;
}

HRESULT sugar_restore_display_mode(sugar* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO see comments in graphics_set_display_mode

    if (CompareMemory(&self->modes.initial, &self->modes.current, sizeof(DEVMODEA))) {
        return DD_OK;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    const s32 result = ChangeDisplaySettingsA(NULL, CDS_NONE);
    if (result == DISP_CHANGE_SUCCESSFUL) {
        CopyMemory(&self->modes.current, &self->modes.initial, sizeof(DEVMODEA));
    }
    else {
        hr = DDERR_UNSUPPORTEDMODE;
        ERR("->ChangeDisplaySettings(NULL, CDS_NONE) -> %s", disp_change_to_string(result));
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT sugar_enumerate_dispaly_modes(sugar* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    DEVMODEA mode;
    ZeroMemory(&mode, sizeof(DEVMODEA));
    mode.dmSize = sizeof(DEVMODEA);

    u32 count = 0;
    while (EnumDisplaySettingsA(NULL, count, &mode)) { count++; }

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = allocator_allocate(self->allocator,
        MEM_TAG_SUGAR, count * sizeof(DEVMODEA), &self->modes.modes))) {
        self->modes.count = count;
        for (u32 i = 0; i < count; i++) {
            EnumDisplaySettingsA(NULL, i, &self->modes.modes[i]);
        }

        HDC hdc = GetDC(NULL);
        self->modes.initial.dmSize = sizeof(DEVMODEA);
        self->modes.initial.dmPelsWidth = GetDeviceCaps(hdc, HORZRES);
        self->modes.initial.dmPelsHeight = GetDeviceCaps(hdc, VERTRES);
        self->modes.initial.dmBitsPerPel = GetDeviceCaps(hdc, BITSPIXEL);
        self->modes.initial.dmDisplayFrequency = GetDeviceCaps(hdc, VREFRESH);
        self->modes.initial.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
        ReleaseDC(NULL, hdc);

        CopyMemory(&self->modes.current, &self->modes.initial, sizeof(DEVMODEA));
    }

    return hr;
}
