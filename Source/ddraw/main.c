#include <sugar.h>

#include <wchar.h>

static sugar* manager;

BOOL APIENTRY
DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        char path[MAX_PATH];
        ZeroMemory(path, MAX_PATH);
        if (GetModuleFileNameA(hModule, path, MAX_PATH) != 0) {
            if (strcat_s(path, MAX_PATH, ".log") == 0) {
                for (int i = 0; path[i] != NULL; i++) {
                    path[i] = tolower(path[i]);
                }

                allocator* allocator = NULL;
                if (SUCCEEDED(allocator_create(&allocator))) {
                    logger* logger = NULL;
                    if (SUCCEEDED(logger_create(allocator, path, LOG_LEVEL_TRACE, &logger))) {
                        if (SUCCEEDED(sugar_create(allocator, logger, &manager))) {
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
AcquireDDThreadLock() {

}

BOOL WINAPI
CompleteCreateSysmemSurface(LPVOID lcl) {
    return FALSE;
}

HRESULT WINAPI
D3DParseUnknownCommand(LPVOID lpvCommands, LPVOID* lplpvReturnedCommand) {
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
DDGetAttachedSurfaceLcl(LPVOID lcl, LPDDSCAPS2 lpDDSCaps, LPVOID* lplpDDAttachedSurfaceLcl) {
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
DDInternalLock(LPVOID lcl, LPVOID* lpBits) {
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
DDInternalUnlock(LPVOID lcl) {
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
DSoundHelp(HWND hWnd, WNDPROC lpWndProc, DWORD pid) {
    return DDERR_UNSUPPORTED;
}

// Creates an instance of a DirectDraw object.
HRESULT WINAPI
DirectDrawCreate(LPGUID lpGUID, LPDIRECTDRAW* lplpDD, LPUNKNOWN pUnkOuter) {
    if (lplpDD == NULL || pUnkOuter != NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (lpGUID != NULL && lpGUID != (LPGUID)DDCREATE_HARDWAREONLY
        && lpGUID != (LPGUID)DDCREATE_EMULATIONONLY) {
        if (!IsEqualGUID(&SUGARDRAW_DEVICE_GUID, lpGUID)) {
            return DDERR_INVALIDDIRECTDRAWGUID;
        }
    }

    return sugar_create_direct_draw(manager, &CLSID_DirectDraw, &IID_IDirectDraw, lplpDD);
}

// Creates an instance of a DirectDrawClipper object not associated with a DirectDraw object.
HRESULT WINAPI
DirectDrawCreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, LPUNKNOWN pUnkOuter) {
    return DDERR_UNSUPPORTED;
}

// Creates an instance of a DirectDraw object that supports the set of Direct3D interfaces in DirectX 7.0.
HRESULT WINAPI
DirectDrawCreateEx(LPGUID lpGUID, LPVOID* lplpDD, REFIID riid, LPUNKNOWN pUnkOuter) {
    if (lplpDD == NULL || pUnkOuter != NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (lpGUID != NULL && lpGUID != (LPGUID)DDCREATE_HARDWAREONLY
        && lpGUID != (LPGUID)DDCREATE_EMULATIONONLY) {
        if (!IsEqualGUID(&SUGARDRAW_DEVICE_GUID, lpGUID)) {
            return DDERR_INVALIDDIRECTDRAWGUID;
        }
    }

    if (!IsEqualGUID(&IID_IDirectDraw7, riid)) {
        return DDERR_INVALIDPARAMS;
    }

    return sugar_create_direct_draw(manager, &CLSID_DirectDraw7, &IID_IDirectDraw7, lplpDD);
}

// Enumerates the primary DirectDraw display device and a non-display device
// (such as a 3-D accelerator that has no 2-D capabilities) if one is installed.
// The NULL entry always identifies the primary display device shared with GDI.
HRESULT WINAPI
DirectDrawEnumerateA(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext) {
    if (lpCallback == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    CHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    strcpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, SUGARDRAW_DEVICE_NAME);

    CHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    strcpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, SUGARDRAW_DEVICE_DESCRIPTION);

    lpCallback(&device, name, description, lpContext);

    return DD_OK;
}

// Enumerates all DirectDraw devices installed on the system.
// The NULL entry always identifies the primary display device shared with GDI.
HRESULT WINAPI
DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags) {
    if (lpCallback == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (dwFlags == DDENUM_NONE || (dwFlags & ~DDENUM_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    CHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    strcpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, SUGARDRAW_DEVICE_NAME);

    CHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    strcpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, SUGARDRAW_DEVICE_DESCRIPTION);

    lpCallback(&device, name, description, lpContext, NULL);

    return DD_OK;
}

// Enumerates all DirectDraw devices installed on the system.
// The NULL entry always identifies the primary display device shared with GDI.
HRESULT WINAPI
DirectDrawEnumerateExW(LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags) {
    if (lpCallback == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (dwFlags == DDENUM_NONE || (dwFlags & ~DDENUM_VALID)) {
        return DDERR_INVALIDPARAMS;
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    WCHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    wcscpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, TEXT(SUGARDRAW_DEVICE_NAME));

    WCHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    wcscpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, TEXT(SUGARDRAW_DEVICE_DESCRIPTION));

    lpCallback(&device, name, description, lpContext, NULL);

    return DD_OK;
}

// Enumerates the primary DirectDraw display device and a non-display device
// (such as a 3-D accelerator that has no 2-D capabilities) if one is installed.
// The NULL entry always identifies the primary display device shared with GDI.
HRESULT WINAPI
DirectDrawEnumerateW(LPDDENUMCALLBACKW lpCallback, LPVOID lpContext) {
    if (lpCallback == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    WCHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    wcscpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, TEXT(SUGARDRAW_DEVICE_NAME));

    WCHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    wcscpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, TEXT(SUGARDRAW_DEVICE_DESCRIPTION));

    lpCallback(&device, name, description, lpContext);

    return DD_OK;
}

HRESULT WINAPI
DllCanUnloadNow() {
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    return DDERR_UNSUPPORTED;
}

LPVOID WINAPI
GetDDSurfaceLocal(LPVOID lcl, DWORD handle, BOOL* isnew) {
    return NULL;
}

ULONG_PTR WINAPI
GetOLEThunkData(ULONG_PTR dwOrdinal) {
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
GetSurfaceFromDC(HDC hDC, LPDIRECTDRAWSURFACE* ppDDS, HDC* phDCDriver) {
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
RegisterSpecialCase(DWORD dwParam1, DWORD dwParam2, DWORD dwParam3, DWORD dwParam4) {
    return DDERR_UNSUPPORTED;
}

VOID WINAPI
ReleaseDDThreadLock() {

}

HRESULT WINAPI
SetAppCompatData(DWORD dwType, DWORD dwValue) {
    return DDERR_UNSUPPORTED;
}
