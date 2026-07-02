#include "idd.h"
#include "dd.h"

static HRESULT SUGARCALL idd_compact(idd*);
static HRESULT SUGARCALL idd_create_clipper(idd*, DWORD, LPDIRECTDRAWCLIPPER*, IUnknown*);
static HRESULT SUGARCALL idd_create_palette(idd*, DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE*, IUnknown*);
static HRESULT SUGARCALL idd_create_surface1(idd*, LPDDSURFACEDESC, LPDIRECTDRAWSURFACE*, IUnknown*);
static HRESULT SUGARCALL idd_duplicate_surface1(idd*, LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE*);
static HRESULT SUGARCALL idd_enum_display_modes1(idd*, DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK);
static HRESULT SUGARCALL idd_enum_surfaces1(idd*, DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMSURFACESCALLBACK);
static HRESULT SUGARCALL idd_flip_to_gdi_surface(idd*);
static HRESULT SUGARCALL idd_get_caps(idd*, LPDDCAPS, LPDDCAPS);
static HRESULT SUGARCALL idd_get_display_mode1(idd*, LPDDSURFACEDESC);
static HRESULT SUGARCALL idd_get_fourcc_codes(idd*, LPDWORD, LPDWORD);
static HRESULT SUGARCALL idd_get_gdi_surface1(idd*, LPDIRECTDRAWSURFACE*);
static HRESULT SUGARCALL idd_get_monitor_frequency(idd*, LPDWORD);
static HRESULT SUGARCALL idd_get_scan_line(idd*, LPDWORD);
static HRESULT SUGARCALL idd_get_vertical_blank_status(idd*, LPBOOL);
static HRESULT SUGARCALL idd_initialize(idd*, const GUID*);
static HRESULT SUGARCALL idd_restore_display_mode(idd*);
static HRESULT SUGARCALL idd_set_cooperative_level(idd*, HWND, DWORD);
static HRESULT SUGARCALL idd_set_display_mode1(idd*, DWORD, DWORD, DWORD);
static HRESULT SUGARCALL idd_wait_for_vertical_blank(idd*, DWORD, HANDLE);

static HRESULT SUGARCALL idd_set_display_mode2(idd*, DWORD, DWORD, DWORD, DWORD, DWORD);
static HRESULT SUGARCALL idd_get_available_vid_mem2(idd*, LPDDSCAPS, LPDWORD, LPDWORD);

static HRESULT SUGARCALL idd_create_surface4(idd*, LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE4*, IUnknown*);
static HRESULT SUGARCALL idd_duplicate_surface4(idd*, LPDIRECTDRAWSURFACE4, LPDIRECTDRAWSURFACE4*);
static HRESULT SUGARCALL idd_enum_display_modes4(idd*, DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMMODESCALLBACK2);
static HRESULT SUGARCALL idd_enum_surfaces4(idd*, DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMSURFACESCALLBACK2);
static HRESULT SUGARCALL idd_get_display_mode4(idd*, LPDDSURFACEDESC2);
static HRESULT SUGARCALL idd_get_gdi_surface4(idd*, LPDIRECTDRAWSURFACE4*);
static HRESULT SUGARCALL idd_get_available_vid_mem4(idd*, LPDDSCAPS2, LPDWORD, LPDWORD);
static HRESULT SUGARCALL idd_restore_all_surfaces4(idd*);
static HRESULT SUGARCALL idd_get_surface_from_dc4(idd*, HDC, LPDIRECTDRAWSURFACE4*);
static HRESULT SUGARCALL idd_test_cooperative_level4(idd*);
static HRESULT SUGARCALL idd_get_device_identifier4(idd*, LPDDDEVICEIDENTIFIER, DWORD);

static HRESULT SUGARCALL idd_create_surface7(idd*, LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE7*, IUnknown*);
static HRESULT SUGARCALL idd_duplicate_surface7(idd*, LPDIRECTDRAWSURFACE7, LPDIRECTDRAWSURFACE7*);
static HRESULT SUGARCALL idd_enum_surfaces7(idd*, DWORD, LPDDSURFACEDESC2, LPVOID, LPDDENUMSURFACESCALLBACK7);
static HRESULT SUGARCALL idd_get_gdi_surface7(idd*, LPDIRECTDRAWSURFACE7*);
static HRESULT SUGARCALL idd_get_surface_from_dc7(idd*, HDC, LPDIRECTDRAWSURFACE7*);
static HRESULT SUGARCALL idd_get_device_identifier7(idd*, LPDDDEVICEIDENTIFIER2, DWORD);
static HRESULT SUGARCALL idd_start_mode_test7(idd*, LPSIZE, DWORD, DWORD);
static HRESULT SUGARCALL idd_evaluate_mode7(idd*, DWORD, DWORD*);

typedef struct idd1_vft {
    IDDQUERYINTERFACE           QueryInterface;
    IDDADDREF                   AddRef;
    IDDRELEASE                  Release;
    IDDCOMPACT                  Compact;
    IDDCREATECLIPPER            CreateClipper;
    IDDCREATEPALETTE            CreatePalette;
    IDDCREATESURFACE1           CreateSurface;
    IDDDUPLICATESURFACE1        DuplicateSurface;
    IDDENUMDISPLAYMODES1        EnumDisplayModes;
    IDDENUMSURFACES1            EnumSurfaces;
    IDDFLIPTOGDISURFACE         FlipToGDISurface;
    IDDGETCAPS                  GetCaps;
    IDDGETDISPLAYMODE1          GetDisplayMode;
    IDDGETFOURCCCODES           GetFourCCCodes;
    IDDGETGDISURFACE1           GetGDISurface;
    IDDGETMONITORFREQUENCY      GetMonitorFrequency;
    IDDGETSCANLINE              GetScanLine;
    IDDGETVERTICALBLANKSTATUS   GetVerticalBlankStatus;
    IDDINITIALIZE               Initialize;
    IDDRESTOREDISPLAYMODE       RestoreDisplayMode;
    IDDSETCOOPERATIVELEVEL      SetCooperativeLevel;
    IDDSETDISPLAYMODE1          SetDisplayMode;
    IDDWAITFORVERTICALBLANK     WaitForVerticalBlank;
} idd1_vft;

const static idd1_vft idd1_self = {
    idd_query_interface,
    idd_add_ref,
    idd_remove_ref,
    idd_compact,
    idd_create_clipper,
    idd_create_palette,
    idd_create_surface1,
    idd_duplicate_surface1,
    idd_enum_display_modes1,
    idd_enum_surfaces1,
    idd_flip_to_gdi_surface,
    idd_get_caps,
    idd_get_display_mode1,
    idd_get_fourcc_codes,
    idd_get_gdi_surface1,
    idd_get_monitor_frequency,
    idd_get_scan_line,
    idd_get_vertical_blank_status,
    idd_initialize,
    idd_restore_display_mode,
    idd_set_cooperative_level,
    idd_set_display_mode1,
    idd_wait_for_vertical_blank
};

typedef struct idd2_vft {
    IDDQUERYINTERFACE           QueryInterface;
    IDDADDREF                   AddRef;
    IDDRELEASE                  Release;
    IDDCOMPACT                  Compact;
    IDDCREATECLIPPER            CreateClipper;
    IDDCREATEPALETTE            CreatePalette;
    IDDCREATESURFACE1           CreateSurface;
    IDDDUPLICATESURFACE1        DuplicateSurface;
    IDDENUMDISPLAYMODES1        EnumDisplayModes;
    IDDENUMSURFACES1            EnumSurfaces;
    IDDFLIPTOGDISURFACE         FlipToGDISurface;
    IDDGETCAPS                  GetCaps;
    IDDGETDISPLAYMODE1          GetDisplayMode;
    IDDGETFOURCCCODES           GetFourCCCodes;
    IDDGETGDISURFACE1           GetGDISurface;
    IDDGETMONITORFREQUENCY      GetMonitorFrequency;
    IDDGETSCANLINE              GetScanLine;
    IDDGETVERTICALBLANKSTATUS   GetVerticalBlankStatus;
    IDDINITIALIZE               Initialize;
    IDDRESTOREDISPLAYMODE       RestoreDisplayMode;
    IDDSETCOOPERATIVELEVEL      SetCooperativeLevel;
    IDDSETDISPLAYMODE2          SetDisplayMode;
    IDDWAITFORVERTICALBLANK     WaitForVerticalBlank;
    IDDGETAVAILABLEVIDMEM2      GetAvailableVidMem;
} idd2_vft;

const static idd2_vft idd2_self = {
    idd_query_interface,
    idd_add_ref,
    idd_remove_ref,
    idd_compact,
    idd_create_clipper,
    idd_create_palette,
    idd_create_surface1,
    idd_duplicate_surface1,
    idd_enum_display_modes1,
    idd_enum_surfaces1,
    idd_flip_to_gdi_surface,
    idd_get_caps,
    idd_get_display_mode1,
    idd_get_fourcc_codes,
    idd_get_gdi_surface1,
    idd_get_monitor_frequency,
    idd_get_scan_line,
    idd_get_vertical_blank_status,
    idd_initialize,
    idd_restore_display_mode,
    idd_set_cooperative_level,
    idd_set_display_mode2,
    idd_wait_for_vertical_blank,
    idd_get_available_vid_mem2
};

typedef struct idd4_vft {
    IDDQUERYINTERFACE           QueryInterface;
    IDDADDREF                   AddRef;
    IDDRELEASE                  Release;
    IDDCOMPACT                  Compact;
    IDDCREATECLIPPER            CreateClipper;
    IDDCREATEPALETTE            CreatePalette;
    IDDCREATESURFACE4           CreateSurface;
    IDDDUPLICATESURFACE4        DuplicateSurface;
    IDDENUMDISPLAYMODES4        EnumDisplayModes;
    IDDENUMSURFACES4            EnumSurfaces;
    IDDFLIPTOGDISURFACE         FlipToGDISurface;
    IDDGETCAPS                  GetCaps;
    IDDGETDISPLAYMODE4          GetDisplayMode;
    IDDGETFOURCCCODES           GetFourCCCodes;
    IDDGETGDISURFACE4           GetGDISurface;
    IDDGETMONITORFREQUENCY      GetMonitorFrequency;
    IDDGETSCANLINE              GetScanLine;
    IDDGETVERTICALBLANKSTATUS   GetVerticalBlankStatus;
    IDDINITIALIZE               Initialize;
    IDDRESTOREDISPLAYMODE       RestoreDisplayMode;
    IDDSETCOOPERATIVELEVEL      SetCooperativeLevel;
    IDDSETDISPLAYMODE2          SetDisplayMode;
    IDDWAITFORVERTICALBLANK     WaitForVerticalBlank;
    IDDGETAVAILABLEVIDMEM4      GetAvailableVidMem;
    IDDGETSURFACEFROMDC4        GetSurfaceFromDC;
    IDDRESTOREALLSURFACES4      RestoreAllSurfaces;
    IDDTESTCOOPERATIVELEVEL4    TestCooperativeLevel;
    IDDGETDEVICEIDENTIFIER4     GetDeviceIdentifier;
} idd4_vft;

const static idd4_vft idd4_self = {
    idd_query_interface,
    idd_add_ref,
    idd_remove_ref,
    idd_compact,
    idd_create_clipper,
    idd_create_palette,
    idd_create_surface4,
    idd_duplicate_surface4,
    idd_enum_display_modes4,
    idd_enum_surfaces4,
    idd_flip_to_gdi_surface,
    idd_get_caps,
    idd_get_display_mode4,
    idd_get_fourcc_codes,
    idd_get_gdi_surface4,
    idd_get_monitor_frequency,
    idd_get_scan_line,
    idd_get_vertical_blank_status,
    idd_initialize,
    idd_restore_display_mode,
    idd_set_cooperative_level,
    idd_set_display_mode2,
    idd_wait_for_vertical_blank,
    idd_get_available_vid_mem4,
    idd_get_surface_from_dc4,
    idd_restore_all_surfaces4,
    idd_test_cooperative_level4,
    idd_get_device_identifier4
};

typedef struct idd7_vft {
    IDDQUERYINTERFACE           QueryInterface;
    IDDADDREF                   AddRef;
    IDDRELEASE                  Release;
    IDDCOMPACT                  Compact;
    IDDCREATECLIPPER            CreateClipper;
    IDDCREATEPALETTE            CreatePalette;
    IDDCREATESURFACE7           CreateSurface;
    IDDDUPLICATESURFACE7        DuplicateSurface;
    IDDENUMDISPLAYMODES4        EnumDisplayModes;
    IDDENUMSURFACES7            EnumSurfaces;
    IDDFLIPTOGDISURFACE         FlipToGDISurface;
    IDDGETCAPS                  GetCaps;
    IDDGETDISPLAYMODE4          GetDisplayMode;
    IDDGETFOURCCCODES           GetFourCCCodes;
    IDDGETGDISURFACE7           GetGDISurface;
    IDDGETMONITORFREQUENCY      GetMonitorFrequency;
    IDDGETSCANLINE              GetScanLine;
    IDDGETVERTICALBLANKSTATUS   GetVerticalBlankStatus;
    IDDINITIALIZE               Initialize;
    IDDRESTOREDISPLAYMODE       RestoreDisplayMode;
    IDDSETCOOPERATIVELEVEL      SetCooperativeLevel;
    IDDSETDISPLAYMODE2          SetDisplayMode;
    IDDWAITFORVERTICALBLANK     WaitForVerticalBlank;
    IDDGETAVAILABLEVIDMEM4      GetAvailableVidMem;
    IDDGETSURFACEFROMDC7        GetSurfaceFromDC;
    IDDRESTOREALLSURFACES4      RestoreAllSurfaces;
    IDDTESTCOOPERATIVELEVEL4    TestCooperativeLevel;
    IDDGETDEVICEIDENTIFIER7     GetDeviceIdentifier;
    IDDSTARTMODETEST7           StartModeTest;
    IDDEVALUATEMODE7            EvaluateMode;
} idd7_vft;

const static idd7_vft idd7_self = {
    idd_query_interface,
    idd_add_ref,
    idd_remove_ref,
    idd_compact,
    idd_create_clipper,
    idd_create_palette,
    idd_create_surface7,
    idd_duplicate_surface7,
    idd_enum_display_modes4,
    idd_enum_surfaces7,
    idd_flip_to_gdi_surface,
    idd_get_caps,
    idd_get_display_mode4,
    idd_get_fourcc_codes,
    idd_get_gdi_surface7,
    idd_get_monitor_frequency,
    idd_get_scan_line,
    idd_get_vertical_blank_status,
    idd_initialize,
    idd_restore_display_mode,
    idd_set_cooperative_level,
    idd_set_display_mode2,
    idd_wait_for_vertical_blank,
    idd_get_available_vid_mem4,
    idd_get_surface_from_dc7,
    idd_restore_all_surfaces4,
    idd_test_cooperative_level4,
    idd_get_device_identifier7,
    idd_start_mode_test7,
    idd_evaluate_mode7
};

HRESULT SUGARCALL idd_create(sugar* manager, const GUID* riid, idd** object) {
    if (manager == NULL || riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsEqualGUID(&IID_IDirectDraw, riid)
        && !IsEqualGUID(&IID_IDirectDraw2, riid)
        && !IsEqualGUID(&IID_IDirectDraw4, riid)
        && !IsEqualGUID(&IID_IDirectDraw7, riid)) {
        return E_NOINTERFACE;
    }

    HRESULT hr = DD_OK;
    idd* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_IDIRECTDRAW, sizeof(idd), &instance))) {
        if (IsEqualGUID(&IID_IDirectDraw, riid)) {
            instance->self = &idd1_self;
        }
        else if (IsEqualGUID(&IID_IDirectDraw2, riid)) {
            instance->self = &idd2_self;
        }
        else if (IsEqualGUID(&IID_IDirectDraw4, riid)) {
            instance->self = &idd4_self;
        }
        else if (IsEqualGUID(&IID_IDirectDraw7, riid)) {
            instance->self = &idd7_self;
        }

        instance->manager = manager;
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    return hr;
}

VOID SUGARCALL idd_release(idd* self) {
    if (self != NULL) {
        allocator_free(self->manager->allocator, self);
    }
}

HRESULT SUGARCALL idd_query_interface(idd* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return E_INVALIDARG;
    }

    return dd_query_interface(self->instance, riid, object);
}

ULONG SUGARCALL idd_add_ref(idd* self) {
    if (self == NULL) {
        return 0;
    }

    return InterlockedIncrement(&self->refs);
}

ULONG SUGARCALL idd_remove_ref(idd* self) {
    if (self == NULL || self->refs == 0) {
        return 0;
    }

    s32 result = InterlockedDecrement(&self->refs);

    if ((result = max(result, 0)) == 0) {
        self->refs = 0;

        if (self->instance != NULL) {
            dd_remove_ref(self->instance, self);
        }

        idd_release(self);
    }

    return result;
}

HRESULT SUGARCALL idd_compact(idd* self) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_create_clipper(idd* self, DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_create_palette(idd* self, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE* lplpDDPalette, IUnknown* pUnkOuter) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_create_surface1(idd* self, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_duplicate_surface1(idd* self, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE* lplpDupDDSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_enum_display_modes1(idd* self, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_enum_surfaces1(idd* self, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_flip_to_gdi_surface(idd* self) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_caps(idd* self, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_display_mode1(idd* self, LPDDSURFACEDESC lpDDSurfaceDesc) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_fourcc_codes(idd* self, LPDWORD lpNumCodes, LPDWORD lpCodes) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_gdi_surface1(idd* self, LPDIRECTDRAWSURFACE* lplpGDIDDSSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_monitor_frequency(idd* self, LPDWORD lpdwFrequency) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_scan_line(idd* self, LPDWORD lpdwScanLine) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_vertical_blank_status(idd* self, LPBOOL lpbIsInVB) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_initialize(idd* self, const GUID* lpGUID) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpGUID == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return dd_initialize(self->instance, lpGUID);
}

HRESULT SUGARCALL idd_restore_display_mode(idd* self) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_set_cooperative_level(idd* self, HWND hWnd, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_set_display_mode1(idd* self, DWORD dwWidth, DWORD dwHeight, DWORD dwBpp) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_wait_for_vertical_blank(idd* self, DWORD dwFlags, HANDLE hEvent) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_set_display_mode2(idd* self, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_available_vid_mem2(idd* self, LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_create_surface4(idd* self, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE4* lplpDDSurface, IUnknown* pUnkOuter) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_duplicate_surface4(idd* self, LPDIRECTDRAWSURFACE4 lpDDSurface, LPDIRECTDRAWSURFACE4* lplpDupDDSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_enum_display_modes4(idd* self, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_enum_surfaces4(idd* self, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_display_mode4(idd* self, LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_gdi_surface4(idd* self, LPDIRECTDRAWSURFACE4* lplpGDIDDSSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_available_vid_mem4(idd* self, LPDDSCAPS2 lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_restore_all_surfaces4(idd* self) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_surface_from_dc4(idd* self, HDC hdc, LPDIRECTDRAWSURFACE4* lpDDS) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_test_cooperative_level4(idd* self) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_device_identifier4(idd* self, LPDDDEVICEIDENTIFIER lpdddi, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpdddi == NULL
        || (dwFlags != DDGDI_NONE && dwFlags != DDGDI_GETHOSTIDENTIFIER)) {
        return DDERR_INVALIDPARAMS;
    }

    ZeroMemory(lpdddi, sizeof(DDDEVICEIDENTIFIER));

    strcpy_s(lpdddi->szDriver, MAX_DDDEVICEID_STRING, SUGARDRAW_DEVICE_FILE);
    strcpy_s(lpdddi->szDescription, MAX_DDDEVICEID_STRING, SUGARDRAW_DEVICE_NAME);

    CopyMemory(&lpdddi->guidDeviceIdentifier, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    return DD_OK;
}

HRESULT SUGARCALL idd_create_surface7(idd* self, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE7* lplpDDSurface, IUnknown* pUnkOuter) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_duplicate_surface7(idd* self, LPDIRECTDRAWSURFACE7 lpDDSurface, LPDIRECTDRAWSURFACE7* lplpDupDDSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_enum_surfaces7(idd* self, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_gdi_surface7(idd* self, LPDIRECTDRAWSURFACE7* lplpGDIDDSSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_surface_from_dc7(idd* self, HDC hdc, LPDIRECTDRAWSURFACE7* lpDDS) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_get_device_identifier7(idd* self, LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpdddi == NULL
        || (dwFlags != DDGDI_NONE && dwFlags != DDGDI_GETHOSTIDENTIFIER)) {
        return DDERR_INVALIDPARAMS;
    }

    ZeroMemory(lpdddi, sizeof(DDDEVICEIDENTIFIER2));

    strcpy_s(lpdddi->szDriver, MAX_DDDEVICEID_STRING, SUGARDRAW_DEVICE_FILE);
    strcpy_s(lpdddi->szDescription, MAX_DDDEVICEID_STRING, SUGARDRAW_DEVICE_NAME);

    CopyMemory(&lpdddi->guidDeviceIdentifier, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    lpdddi->dwWHQLLevel = 0x7EA0701; // 2026-07-01

    return DD_OK;
}

HRESULT SUGARCALL idd_start_mode_test7(idd* self, LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idd_evaluate_mode7(idd* self, DWORD dwFlags, DWORD* pSecondsUntilTimeout) {
    return DDERR_UNSUPPORTED;
}
