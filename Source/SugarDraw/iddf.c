#include "ddf.h"
#include "iddf.h"

#define DDFCDD_NONE                     0x00000000L

static HRESULT SUGARCALL iddf_create_direct_draw1(iddf*, GUID*, HWND, DWORD, DWORD, LPUNKNOWN, LPDIRECTDRAW*);
static HRESULT SUGARCALL iddf_direct_draw_enumerate1(iddf*, LPDDENUMCALLBACKA, LPVOID);

static HRESULT SUGARCALL iddf_create_direct_draw2(iddf*, GUID*, HWND, DWORD, DWORD, LPUNKNOWN, LPDIRECTDRAW4*);
static HRESULT SUGARCALL iddf_direct_draw_enumerate2(iddf*, LPDDENUMCALLBACKEXA, LPVOID, DWORD);

typedef struct iddfu_vft {
    IDDFQUERYINTERFACE          QueryInterface;
    IDDFADDREF                  AddRef;
    IDDFRELEASE                 Release;
} iddfu_vft;

const static iddfu_vft iddfu_self = {
    iddf_query_interface,
    iddf_add_ref,
    iddf_remove_ref
};

typedef struct iddf1_vft {
    IDDFQUERYINTERFACE          QueryInterface;
    IDDFADDREF                  AddRef;
    IDDFRELEASE                 Release;
    IDDFCREATEDIRECTDRAW1       CreateDirectDraw;
    IDDFDIRECTDRAWENUMERATE1    DirectDrawEnumerate;
} iddf1_vft;

const static iddf1_vft iddf1_self = {
    iddf_query_interface,
    iddf_add_ref,
    iddf_remove_ref,
    iddf_create_direct_draw1,
    iddf_direct_draw_enumerate1
};

typedef struct iddf2_vft {
    IDDFQUERYINTERFACE          QueryInterface;
    IDDFADDREF                  AddRef;
    IDDFRELEASE                 Release;
    IDDFCREATEDIRECTDRAW2       CreateDirectDraw;
    IDDFDIRECTDRAWENUMERATE2    DirectDrawEnumerate;
} iddf2_vft;

const static iddf2_vft iddf2_self = {
    iddf_query_interface,
    iddf_add_ref,
    iddf_remove_ref,
    iddf_create_direct_draw2,
    iddf_direct_draw_enumerate2
};

HRESULT SUGARCALL iddf_create(sugar* manager, const GUID* riid, iddf** object) {
    if (manager == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    LOGENTER(manager->logger, "%s, 0x%p", guid_to_string(riid), object);

    if (riid == NULL || object == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (!IsEqualGUID(&IID_IUnknown, riid)
        && !IsEqualGUID(&IID_IDirectDrawFactory, riid)) {
        LOGLEAVE(manager->logger, E_NOINTERFACE);
    }

    HRESULT hr = S_OK;
    iddf* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_IDIRECTDRAWFACTORY, sizeof(iddf), &instance))) {
        if (IsEqualGUID(&IID_IUnknown, riid)) {
            instance->self = &iddfu_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawFactory, riid)) {
            instance->self = &iddf1_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawFactory2, riid)) {
            instance->self = &iddf2_self;
        }

        instance->manager = manager;
        instance->logger = manager->logger;
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    LOGLEAVESELF(manager->logger, hr, instance);
}

VOID SUGARCALL iddf_release(iddf* self) {
    if (self != NULL) {
        allocator_free(self->manager->allocator, self);
    }
}

HRESULT SUGARCALL iddf_query_interface(iddf* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p", guid_to_string(riid), object);

    LEAVE(ddf_query_interface(self->instance, riid, object));
}

ULONG SUGARCALL iddf_add_ref(iddf* self) {
    if (self == NULL) {
        return 0;
    }

    REFCOUNT(InterlockedIncrement(&self->refs));
}

ULONG SUGARCALL iddf_remove_ref(iddf* self) {
    if (self == NULL) {
        return 0;
    }

    s32 result = InterlockedDecrement(&self->refs);

    if ((result = max(result, 0)) == 0) {
        self->refs = 0;

        if (self->instance != NULL) {
            ddf_remove_ref(self->instance, self);
        }

        iddf_release(self);
    }

    REFCOUNT(result);
}

HRESULT SUGARCALL iddf_create_direct_draw1(iddf* self, GUID* pGUID, HWND hWnd, DWORD dwCoopLevelFlags, DWORD dwReserved, LPUNKNOWN pUnkOuter, LPDIRECTDRAW* ppDirectDraw) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%08X, %s, 0x%0x8, 0x%p, 0x%p", guid_to_string(pGUID), hWnd, ddscl_to_string(dwCoopLevelFlags), dwReserved, pUnkOuter, ppDirectDraw);

    if (dwReserved != DDFCDD_NONE || ppDirectDraw == NULL || pUnkOuter != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddf_create_direct_draw(self->instance, &IID_IDirectDraw, pGUID, hWnd, dwCoopLevelFlags, ppDirectDraw));
}

HRESULT SUGARCALL iddf_direct_draw_enumerate1(iddf* self, LPDDENUMCALLBACKA lpCallback, LPVOID lpContext) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%p", lpCallback, lpContext);

    if (lpCallback == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    CHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    strcpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, SUGARDRAW_DEVICE_NAME);

    CHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    strcpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, SUGARDRAW_DEVICE_DESCRIPTION);

    lpCallback(&device, name, description, lpContext);

    LEAVE(DD_OK);
}

HRESULT SUGARCALL iddf_create_direct_draw2(iddf* self, GUID* pGUID, HWND hWnd, DWORD dwCoopLevelFlags, DWORD dwReserved, LPUNKNOWN pUnkOuter, LPDIRECTDRAW4* ppDirectDraw) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%08X, %s, 0x%0x8, 0x%p, 0x%p", guid_to_string(pGUID), hWnd, ddscl_to_string(dwCoopLevelFlags), dwReserved, pUnkOuter, ppDirectDraw);

    if (dwReserved != DDFCDD_NONE || ppDirectDraw == NULL || pUnkOuter != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddf_create_direct_draw(self->instance, &IID_IDirectDraw4, pGUID, hWnd, dwCoopLevelFlags, ppDirectDraw));
}

HRESULT SUGARCALL iddf_direct_draw_enumerate2(iddf* self, LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%p, %s", lpCallback, lpContext, ddenum_to_string(dwFlags));

    if (lpCallback == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags == DDENUM_NONE) || (dwFlags & ~DDENUM_VALID)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    GUID device;
    CopyMemory(&device, &SUGARDRAW_DEVICE_GUID, sizeof(GUID));

    CHAR name[SUGARDRAW_DEVICE_NAME_LENGTH];
    strcpy_s(name, SUGARDRAW_DEVICE_NAME_LENGTH, SUGARDRAW_DEVICE_NAME);

    CHAR description[SUGARDRAW_DEVICE_DESCRIPTION_LENGTH];
    strcpy_s(description, SUGARDRAW_DEVICE_DESCRIPTION_LENGTH, SUGARDRAW_DEVICE_DESCRIPTION);

    lpCallback(&device, name, description, lpContext, NULL); // TODO Monitor Handle

    LEAVE(DD_OK);
}
