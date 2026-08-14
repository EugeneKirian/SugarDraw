#include <sugar.h>

#include <wchar.h>

// TODO driver managemnt
#include <gdi.h>

static sugar* manager;

BOOL APIENTRY
DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        char path[MAX_PATH];
        ZeroMemory(path, MAX_PATH);
        if (GetModuleFileNameA(hModule, path, MAX_PATH) != 0) {
            if (strcat_s(path, MAX_PATH, ".log") == 0) {
                for (u32 i = 0; path[i] != '\0'; i++) {
                    path[i] = tolower(path[i]);
                }

                // TODO driver management
                // For now just GDI driver
                gdi* g = NULL;
                gdi_create(&g);

                driver* driver = NULL;
                gdi_get_driver(g, &driver);
                // TODO driver management + memory leaks

                allocator* allocator = NULL;
                if (SUCCEEDED(allocator_create(&allocator))) {
                    logger* logger = NULL;
                    // TODO log path and log level from the settings
                    if (SUCCEEDED(logger_create(allocator, path, LOG_LEVEL_TRACE, &logger))) {
                        if (SUCCEEDED(sugar_create(allocator, logger, driver, &manager))) {
                            return TRUE;
                        }

                        logger_release(logger);
                    }

                    allocator_release(allocator);
                }
            }
        }

        return FALSE;
    }break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH: {}break;
    case DLL_PROCESS_DETACH: {
        if (manager != NULL) {
            sugar_release(manager);
        }
    }break;
    }

    return TRUE;
}

VOID WINAPI
acquire_dd_thread_lock() {

}

BOOL WINAPI
complete_create_sysmem_surface(LPVOID lcl) {
    return FALSE;
}

HRESULT WINAPI
d3d_parse_unknown_command(LPVOID lpvCommands, LPVOID* lplpvReturnedCommand) {
    LOGLEAVE(manager->logger, DDERR_UNSUPPORTED);
}

HRESULT WINAPI
dd_get_attached_surface_lcl(LPVOID lcl, LPDDSCAPS2 lpDDSCaps, LPVOID* lplpDDAttachedSurfaceLcl) {
    LOGLEAVE(manager->logger, DDERR_UNSUPPORTED);
}

HRESULT WINAPI
dd_internal_lock(LPVOID lcl, LPVOID* lpBits) {
    LOGLEAVE(manager->logger, DDERR_UNSUPPORTED);
}

HRESULT WINAPI
dd_internal_unlock(LPVOID lcl) {
    LOGLEAVE(manager->logger, DDERR_UNSUPPORTED);
}

HRESULT WINAPI
dsound_help(HWND hWnd, WNDPROC lpWndProc, DWORD pid) {
    LOGLEAVE(manager->logger, DDERR_UNSUPPORTED);
}

HRESULT WINAPI
direct_draw_create(LPGUID lpGUID, LPDIRECTDRAW* lplpDD, LPUNKNOWN pUnkOuter) {
    LOGENTER(manager->logger, "%s, 0x%p, 0x%p", guid_to_string(lpGUID), lplpDD, pUnkOuter);

    if (lplpDD == NULL || pUnkOuter != NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (lpGUID != NULL && lpGUID != (LPGUID)DDCREATE_HARDWAREONLY
        && lpGUID != (LPGUID)DDCREATE_EMULATIONONLY) {
        if (!IsEqualGUID(&SUGARDRAW_DEVICE_GUID, lpGUID)) {
            LOGLEAVE(manager->logger, DDERR_INVALIDDIRECTDRAWGUID);
        }
    }

    LOGLEAVE(manager->logger,
        sugar_create_dd(manager, lpGUID, &CLSID_DirectDraw, &IID_IDirectDraw, lplpDD));
}

HRESULT WINAPI
direct_draw_create_clipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, LPUNKNOWN pUnkOuter) {
    LOGENTER(manager->logger, "0x%p, 0x%p", lplpDDClipper, pUnkOuter);

    if (lplpDDClipper == NULL || pUnkOuter != NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    LOGLEAVE(manager->logger,
        sugar_create_ddc(manager, &CLSID_DirectDrawClipper, &IID_IDirectDrawClipper, lplpDDClipper));
}

HRESULT WINAPI
direct_draw_create_ex(LPGUID lpGUID, LPVOID* lplpDD, REFIID riid, LPUNKNOWN pUnkOuter) {
    LOGENTER(manager->logger, "%s, 0x%p, 0x%p", guid_to_string(lpGUID), lplpDD, pUnkOuter);

    if (lplpDD == NULL || pUnkOuter != NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (lpGUID != NULL && lpGUID != (LPGUID)DDCREATE_HARDWAREONLY
        && lpGUID != (LPGUID)DDCREATE_EMULATIONONLY) {
        if (!IsEqualGUID(&SUGARDRAW_DEVICE_GUID, lpGUID)) {
            LOGLEAVE(manager->logger, DDERR_INVALIDDIRECTDRAWGUID);
        }
    }

    if (!IsEqualGUID(&IID_IDirectDraw7, riid)) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    LOGLEAVE(manager->logger,
        sugar_create_dd(manager, lpGUID, &CLSID_DirectDraw7, &IID_IDirectDraw7, lplpDD));
}

HRESULT WINAPI
direct_draw_enumerate_ansi(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext) {
    LOGENTER(manager->logger, "0x%p, 0x%p", lpCallback, lpContext);

    if (lpCallback == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    CHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    strcpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, SUGARDRAW_DEVICE_NAME);

    CHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    strcpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, SUGARDRAW_DEVICE_DESCRIPTION);

    lpCallback(&device, name, description, lpContext);

    LOGLEAVE(manager->logger, DD_OK);
}

HRESULT WINAPI
direct_draw_enumerate_ex_ansi(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags) {
    LOGENTER(manager->logger, "0x%p, 0x%p, %s", lpCallback, lpContext, ddenum_to_string(dwFlags));

    if (lpCallback == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (dwFlags == DDENUM_NONE || (dwFlags & ~DDENUM_VALID)) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    CHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    strcpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, SUGARDRAW_DEVICE_NAME);

    CHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    strcpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, SUGARDRAW_DEVICE_DESCRIPTION);

    lpCallback(&device, name, description, lpContext, NULL); // TODO Monitor Handle

    LOGLEAVE(manager->logger, DD_OK);
}

HRESULT WINAPI
direct_draw_enumerate_ex_wide(LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags) {
    LOGENTER(manager->logger, "0x%p, 0x%p, %s", lpCallback, lpContext, ddenum_to_string(dwFlags));

    if (lpCallback == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (dwFlags == DDENUM_NONE || (dwFlags & ~DDENUM_VALID)) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    WCHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    wcscpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, TEXT(SUGARDRAW_DEVICE_NAME));

    WCHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    wcscpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, TEXT(SUGARDRAW_DEVICE_DESCRIPTION));

    lpCallback(&device, name, description, lpContext, NULL); // TODO Monitor Handle

    LOGLEAVE(manager->logger, DD_OK);
}

HRESULT WINAPI
direct_draw_enumerate_wide(LPDDENUMCALLBACKW lpCallback, LPVOID lpContext) {
    LOGENTER(manager->logger, "0x%p, 0x%p", lpCallback, lpContext);

    if (lpCallback == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    WCHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    wcscpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, TEXT(SUGARDRAW_DEVICE_NAME));

    WCHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    wcscpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, TEXT(SUGARDRAW_DEVICE_DESCRIPTION));

    lpCallback(&device, name, description, lpContext);

    LOGLEAVE(manager->logger, DD_OK);
}

HRESULT WINAPI
dll_can_unload_now() {
    LOGLEAVE(manager->logger, sugar_can_unload(manager));
}

HRESULT WINAPI
dll_get_class_object(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    LOGENTER(manager->logger, "%s, %s, 0x%p", guid_to_string(rclsid), guid_to_string(riid), ppv);

    if (rclsid == NULL || riid == NULL || ppv == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;
    LPVOID instance = NULL;
    if (SUCCEEDED(hr = sugar_create_cf(manager, rclsid, riid, &instance))) {
        *ppv = instance;
    }

    LOGLEAVE(manager->logger, hr);
}

LPVOID WINAPI
get_dd_surface_local(LPVOID lcl, DWORD handle, BOOL* isnew) {
    return NULL;
}

ULONG_PTR WINAPI
get_ole_thunk_data(ULONG_PTR dwOrdinal) {
    LOGLEAVE(manager->logger, DDERR_UNSUPPORTED);
}

HRESULT WINAPI
get_surface_from_dc(HDC hDC, LPDIRECTDRAWSURFACE* ppDDS, HDC* phDCDriver) {
    LOGENTER(manager->logger, "0x%08X, 0x%p, 0x%p", hDC, ppDDS, phDCDriver);

    // TODO iterate over all instances and all surfaces with DC -> return match
    // DDERR_NOTFOUND

    LOGLEAVE(manager->logger, DDERR_UNSUPPORTED);
}

HRESULT WINAPI
register_special_case(DWORD dwParam1, DWORD dwParam2, DWORD dwParam3, DWORD dwParam4) {
    LOGLEAVE(manager->logger, DDERR_UNSUPPORTED);
}

VOID WINAPI
release_dd_thread_lock() {

}

HRESULT WINAPI
set_app_compat_data(DWORD dwType, DWORD dwValue) {
    LOGLEAVE(manager->logger, DDERR_UNSUPPORTED);
}
