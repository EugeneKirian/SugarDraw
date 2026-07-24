#include "dd.h"
#include "idd.h"
#include "idds.h"

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

static HRESULT SUGARCALL idd_get_surface_from_dc3(idd*, HDC, LPDIRECTDRAWSURFACE*);

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

typedef struct iddu_vft {
    IDDQUERYINTERFACE           QueryInterface;
    IDDADDREF                   AddRef;
    IDDRELEASE                  Release;
} iddu_vft;

const static iddu_vft iddu_self = {
    idd_query_interface,
    idd_add_ref,
    idd_remove_ref
};

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

typedef struct idd3_vft {
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
    IDDGETSURFACEFROMDC3        GetSurfaceFromDC;
} idd3_vft;

const static idd3_vft idd3_self = {
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
    idd_get_available_vid_mem2,
    idd_get_surface_from_dc3,
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
    if (manager == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    LOGENTER(manager->logger, "%s, 0x%p", guid_to_string(riid), object);
    
    if (riid == NULL || object == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (!IsEqualGUID(&IID_IUnknown, riid)
        && !IsEqualGUID(&IID_IDirectDraw, riid)
        && !IsEqualGUID(&IID_IDirectDraw2, riid)
        && !IsEqualGUID(&IID_IDirectDraw4, riid)
        && !IsEqualGUID(&IID_IDirectDraw7, riid)) {
        LOGLEAVE(manager->logger, E_NOINTERFACE);
    }

    HRESULT hr = DD_OK;
    idd* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_IDIRECTDRAW, sizeof(idd), &instance))) {
        if (IsEqualGUID(&IID_IUnknown, riid)) {
            instance->self = &iddu_self;
        }
        else if (IsEqualGUID(&IID_IDirectDraw, riid)) {
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
        instance->logger = manager->logger;
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    LOGLEAVESELF(manager->logger, hr, instance);
}

void SUGARCALL idd_release(idd* self) {
    if (self != NULL) {
        allocator_free(self->manager->allocator, self);
    }
}

HRESULT SUGARCALL idd_query_interface(idd* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p", guid_to_string(riid), object);

    LEAVE(dd_query_interface(self->instance, riid, object));
}

ULONG SUGARCALL idd_add_ref(idd* self) {
    if (self == NULL) {
        return 0;
    }

    REFCOUNT(InterlockedIncrement(&self->refs));
}

ULONG SUGARCALL idd_remove_ref(idd* self) {
    if (self == NULL) {
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

    REFCOUNT(result);
}

HRESULT SUGARCALL idd_compact(idd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dd_compact(self->instance));
}

HRESULT SUGARCALL idd_create_clipper(idd* self, DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%08X, 0x%p, 0x%p", dwFlags, lplpDDClipper, pUnkOuter);

    if (dwFlags != DDCCAPS_NONE || lplpDDClipper == NULL || pUnkOuter != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dd_create_clipper(self->instance, lplpDDClipper));
}

HRESULT SUGARCALL idd_create_palette(idd* self, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE* lplpDDPalette, IUnknown* pUnkOuter) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p, 0x%p, 0x%p",
        ddpcaps_to_string(dwFlags), lpDDColorArray, lplpDDPalette, pUnkOuter);

    if (lpDDColorArray == NULL || lplpDDPalette == NULL || pUnkOuter != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dd_create_palette(self->instance, dwFlags, lpDDColorArray, lplpDDPalette));
}

HRESULT SUGARCALL idd_create_surface1(idd* self, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE* lplpDDSurface, IUnknown* pUnkOuter) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p, 0x%p", ddsurfacedesc_to_string(lpDDSurfaceDesc), lplpDDSurface, pUnkOuter);

    if (lpDDSurfaceDesc == NULL || lplpDDSurface == NULL || pUnkOuter != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
    CopyMemory(&desc, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
    desc.dwSize = sizeof(DDSURFACEDESC2);

    LEAVE(dd_create_surface(self->instance, &IID_IDirectDrawSurface, &desc, lplpDDSurface));
}

HRESULT SUGARCALL idd_duplicate_surface1(idd* self, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE* lplpDupDDSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%p", lpDDSurface, lplpDupDDSurface);

    if (lpDDSurface == NULL || lplpDupDDSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dd_duplicate_surface(self->instance,
        ((idds*)lpDDSurface)->instance, &IID_IDirectDrawSurface, lplpDupDDSurface));
}

HRESULT SUGARCALL idd_enum_display_modes1(idd* self, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, %s, 0x%p, 0x%p", ddedm_to_string(dwFlags), ddsurfacedesc_to_string(lpDDSurfaceDesc), lpContext, lpEnumModesCallback);

    // TODO proper implementation

    LEAVE(dd_enum_display_modes(self->instance)); // TODO
}

HRESULT SUGARCALL idd_enum_surfaces1(idd* self, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, %s, 0x%p, 0x%p", ddenumsurfaces_to_string(dwFlags), ddsurfacedesc_to_string(lpDDSD), lpContext, lpEnumSurfacesCallback);

    // TODO proper implementation

    LEAVE(dd_enum_surfaces(self->instance)); // TODO
}

HRESULT SUGARCALL idd_flip_to_gdi_surface(idd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dd_flip_to_gdi_surface(self->instance));
}

HRESULT SUGARCALL idd_get_caps(idd* self, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%p", lpDDDriverCaps, lpDDHELCaps);

    if (lpDDDriverCaps == NULL && lpDDHELCaps == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDCAPS_DX7 caps;
    ZeroMemory(&caps, sizeof(DDCAPS_DX7));
    caps.dwSize = sizeof(DDCAPS_DX7);

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = dd_get_caps(self->instance, &caps))) {
        if (lpDDDriverCaps != NULL) {
            const u32 size = lpDDDriverCaps->dwSize;
            if (size != sizeof(DDCAPS_DX1)
                && size != sizeof(DDCAPS_DX3)
                && size != sizeof(DDCAPS_DX5)
                && size != sizeof(DDCAPS_DX6)
                && size != sizeof(DDCAPS_DX7)) {
                LEAVE(DDERR_INVALIDPARAMS);
            }

            CopyMemory(lpDDDriverCaps, &caps, size);
            lpDDDriverCaps->dwSize = size;
        }

        if (lpDDHELCaps != NULL) {
            const u32 size = lpDDHELCaps->dwSize;
            if (size != sizeof(DDCAPS_DX1)
                && size != sizeof(DDCAPS_DX3)
                && size != sizeof(DDCAPS_DX5)
                && size != sizeof(DDCAPS_DX6)
                && size != sizeof(DDCAPS_DX7)) {
                LEAVE(DDERR_INVALIDPARAMS);
            }

            CopyMemory(lpDDHELCaps, &caps, size);
            lpDDHELCaps->dwSize = size;
        }
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idd_get_display_mode1(idd* self, LPDDSURFACEDESC lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p", ddsurfacedesc_to_string(lpDDSurfaceDesc));

    if (lpDDSurfaceDesc == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
    desc.dwSize = sizeof(DDSURFACEDESC2);

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = dd_get_display_mode(self->instance, &desc))) {
        CopyMemory(lpDDSurfaceDesc, &desc, sizeof(DDSURFACEDESC));
        lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idd_get_fourcc_codes(idd* self, LPDWORD lpNumCodes, LPDWORD lpCodes) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%p", lpNumCodes, lpCodes);

    LEAVE(dd_get_fourcc_codes(self->instance, lpNumCodes, lpCodes));
}

HRESULT SUGARCALL idd_get_gdi_surface1(idd* self, LPDIRECTDRAWSURFACE* lplpGDIDDSSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lplpGDIDDSSurface);

    LEAVE(dd_get_gdi_surface(self->instance, &IID_IDirectDrawSurface, lplpGDIDDSSurface));
}

HRESULT SUGARCALL idd_get_monitor_frequency(idd* self, LPDWORD lpdwFrequency) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpdwFrequency);

    LEAVE(dd_get_monitor_frequency(self->instance, lpdwFrequency));
}

HRESULT SUGARCALL idd_get_scan_line(idd* self, LPDWORD lpdwScanLine) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpdwScanLine);

    LEAVE(dd_get_scan_line(self->instance, lpdwScanLine));
}

HRESULT SUGARCALL idd_get_vertical_blank_status(idd* self, LPBOOL lpbIsInVB) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpbIsInVB);

    LEAVE(dd_get_vertical_blank_status(self->instance, lpbIsInVB));
}

HRESULT SUGARCALL idd_initialize(idd* self, const GUID* lpGUID) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s", guid_to_string(lpGUID));

    LEAVE(dd_initialize(self->instance, lpGUID));
}

HRESULT SUGARCALL idd_restore_display_mode(idd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dd_restore_display_mode(self->instance));
}

HRESULT SUGARCALL idd_set_cooperative_level(idd* self, HWND hWnd, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%08X, %s", hWnd, ddscl_to_string(dwFlags));

    LEAVE(dd_set_cooperative_level(self->instance, hWnd, dwFlags));
}

HRESULT SUGARCALL idd_set_display_mode1(idd* self, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%u, %u, %u", dwWidth, dwHeight, dwBPP);

    LEAVE(dd_set_display_mode(self->instance, dwWidth, dwHeight, dwBPP, 0, DDSDM_NONE));
}

HRESULT SUGARCALL idd_wait_for_vertical_blank(idd* self, DWORD dwFlags, HANDLE hEvent) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%08X", ddwaitvb_to_string(dwFlags), hEvent);

    LEAVE(dd_wait_for_vertical_blank(self->instance, dwFlags, hEvent));
}

HRESULT SUGARCALL idd_set_display_mode2(idd* self, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%u, %u, %u, %u, %s", dwWidth, dwHeight, dwBPP, dwRefreshRate, ddsdm_to_string(dwFlags));

    LEAVE(dd_set_display_mode(self->instance, dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags));
}

HRESULT SUGARCALL idd_get_available_vid_mem2(idd* self, LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p, 0x%p", ddscaps_to_string(lpDDSCaps), lpdwTotal, lpdwFree);

    if (lpDDSCaps == NULL
        || (lpdwTotal == NULL && lpdwFree == NULL)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(DDSCAPS2));
    CopyMemory(&caps, lpDDSCaps, sizeof(DDSCAPS));

    LEAVE(dd_get_available_vid_mem(self->instance, &caps, lpdwTotal, lpdwFree));
}

HRESULT SUGARCALL idd_get_surface_from_dc3(idd* self, HDC hdc, LPDIRECTDRAWSURFACE* lpDDS) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%08X, 0x%p", hdc, lpDDS);

    LEAVE(dd_get_surface_from_dc(self->instance, hdc, &IID_IDirectDrawSurface, lpDDS));
}

HRESULT SUGARCALL idd_create_surface4(idd* self, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE4* lplpDDSurface, IUnknown* pUnkOuter) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p, 0x%p", ddsurfacedesc2_to_string(lpDDSurfaceDesc), lplpDDSurface, pUnkOuter);

    if (lpDDSurfaceDesc == NULL || lplpDDSurface == NULL || pUnkOuter != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC2)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSURFACEDESC2 desc;
    CopyMemory(&desc, lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));

    LEAVE(dd_create_surface(self->instance, &IID_IDirectDrawSurface4, &desc, lplpDDSurface));
}

HRESULT SUGARCALL idd_duplicate_surface4(idd* self, LPDIRECTDRAWSURFACE4 lpDDSurface, LPDIRECTDRAWSURFACE4* lplpDupDDSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%p", lpDDSurface, lplpDupDDSurface);

    if (lpDDSurface == NULL || lplpDupDDSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dd_duplicate_surface(self->instance,
        ((idds*)lpDDSurface)->instance, &IID_IDirectDrawSurface4, lplpDupDDSurface));
}

HRESULT SUGARCALL idd_enum_display_modes4(idd* self, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, %s, 0x%p, 0x%p", ddedm_to_string(dwFlags), ddsurfacedesc2_to_string(lpDDSurfaceDesc), lpContext, lpEnumModesCallback);

    // TODO proper implementation

    LEAVE(dd_enum_display_modes(self->instance)); // TODO
}

HRESULT SUGARCALL idd_enum_surfaces4(idd* self, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, %s, 0x%p, 0x%p", ddenumsurfaces_to_string(dwFlags), ddsurfacedesc2_to_string(lpDDSD), lpContext, lpEnumSurfacesCallback);

    // TODO proper implementation

    LEAVE(dd_enum_surfaces(self->instance)); // TODO
}

HRESULT SUGARCALL idd_get_display_mode4(idd* self, LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpDDSurfaceDesc);

    if (lpDDSurfaceDesc == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC2)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
    desc.dwSize = sizeof(DDSURFACEDESC2);

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = dd_get_display_mode(self->instance, &desc))) {
        CopyMemory(lpDDSurfaceDesc, &desc, sizeof(DDSURFACEDESC2));
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idd_get_gdi_surface4(idd* self, LPDIRECTDRAWSURFACE4* lplpGDIDDSSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lplpGDIDDSSurface);

    LEAVE(dd_get_gdi_surface(self->instance, &IID_IDirectDrawSurface4, lplpGDIDDSSurface));
}

HRESULT SUGARCALL idd_get_available_vid_mem4(idd* self, LPDDSCAPS2 lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p, 0x%p", ddscaps2_to_string(lpDDSCaps), lpdwTotal, lpdwFree);

    if (lpDDSCaps == NULL
        || (lpdwTotal == NULL && lpdwFree == NULL)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSCAPS2 caps;
    CopyMemory(&caps, lpDDSCaps, sizeof(DDSCAPS2));

    LEAVE(dd_get_available_vid_mem(self->instance, &caps, lpdwTotal, lpdwFree));
}

HRESULT SUGARCALL idd_get_surface_from_dc4(idd* self, HDC hdc, LPDIRECTDRAWSURFACE4* lpDDS) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%08X, 0x%p", hdc, lpDDS);

    LEAVE(dd_get_surface_from_dc(self->instance, hdc, &IID_IDirectDrawSurface4, lpDDS));
}

HRESULT SUGARCALL idd_restore_all_surfaces4(idd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dd_restore_all_surfaces(self->instance));
}

HRESULT SUGARCALL idd_test_cooperative_level4(idd* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dd_test_cooperative_level(self->instance));
}

HRESULT SUGARCALL idd_get_device_identifier4(idd* self, LPDDDEVICEIDENTIFIER lpdddi, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, %s", lpdddi, ddgdi_to_string(dwFlags));

    if (lpdddi == NULL
        || (dwFlags != DDGDI_NONE && dwFlags != DDGDI_GETHOSTIDENTIFIER)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDDEVICEIDENTIFIER2 identifier;
    ZeroMemory(&identifier, sizeof(DDDEVICEIDENTIFIER2));

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = dd_get_device_identifier(self->instance, &identifier))) {
        CopyMemory(lpdddi, &identifier, sizeof(DDDEVICEIDENTIFIER));
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idd_create_surface7(idd* self, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE7* lplpDDSurface, IUnknown* pUnkOuter) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p, 0x%p", ddsurfacedesc2_to_string(lpDDSurfaceDesc), lplpDDSurface, pUnkOuter);

    if (lpDDSurfaceDesc == NULL || lplpDDSurface == NULL || pUnkOuter != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC2)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSURFACEDESC2 desc;
    CopyMemory(&desc, lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));

    LEAVE(dd_create_surface(self->instance, &IID_IDirectDrawSurface7, &desc, lplpDDSurface));
}

HRESULT SUGARCALL idd_duplicate_surface7(idd* self, LPDIRECTDRAWSURFACE7 lpDDSurface, LPDIRECTDRAWSURFACE7* lplpDupDDSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%0", lpDDSurface, lplpDupDDSurface);

    if (lpDDSurface == NULL || lplpDupDDSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dd_duplicate_surface(self->instance,
        ((idds*)lpDDSurface)->instance, &IID_IDirectDrawSurface7, lplpDupDDSurface));
}

HRESULT SUGARCALL idd_enum_surfaces7(idd* self, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, %s, 0x%p, 0x%p", ddenumsurfaces_to_string(dwFlags), ddsurfacedesc2_to_string(lpDDSD), lpContext, lpEnumSurfacesCallback);

    // TODO proper implementation

    LEAVE(dd_enum_surfaces(self->instance)); // TODO
}

HRESULT SUGARCALL idd_get_gdi_surface7(idd* self, LPDIRECTDRAWSURFACE7* lplpGDIDDSSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lplpGDIDDSSurface);

    LEAVE(dd_get_gdi_surface(self->instance, &IID_IDirectDrawSurface7, lplpGDIDDSSurface));
}

HRESULT SUGARCALL idd_get_surface_from_dc7(idd* self, HDC hdc, LPDIRECTDRAWSURFACE7* lpDDS) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%p", hdc, lpDDS);

    LEAVE(dd_get_surface_from_dc(self->instance, hdc, &IID_IDirectDrawSurface7, lpDDS));
}

HRESULT SUGARCALL idd_get_device_identifier7(idd* self, LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, %s", lpdddi, ddgdi_to_string(dwFlags));

    if (lpdddi == NULL
        || (dwFlags != DDGDI_NONE && dwFlags != DDGDI_GETHOSTIDENTIFIER)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDDEVICEIDENTIFIER2 identifier;
    ZeroMemory(&identifier, sizeof(DDDEVICEIDENTIFIER2));

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = dd_get_device_identifier(self->instance, &identifier))) {
        CopyMemory(lpdddi, &identifier, sizeof(DDDEVICEIDENTIFIER2));
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idd_start_mode_test7(idd* self, SIZE* lpModesToTest, DWORD dwNumEntries, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO Enter

    // TODO validation

    // TODO proper implementation

    LEAVE(dd_start_mode_test(self->instance, lpModesToTest, dwNumEntries, dwFlags));
}

HRESULT SUGARCALL idd_evaluate_mode7(idd* self, DWORD dwFlags, DWORD* pSecondsUntilTimeout) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO Enter

    // TODO validation

    // TODO proper implementation

    LEAVE(dd_evaluate_mode(self->instance, dwFlags, pSecondsUntilTimeout));
}
