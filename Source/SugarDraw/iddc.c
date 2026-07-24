#include "ddc.h"
#include "idd.h"
#include "iddc.h"

static HRESULT SUGARCALL iddc_get_clip_list(iddc*, LPRECT, LPRGNDATA, LPDWORD);
static HRESULT SUGARCALL iddc_get_hwnd(iddc*, HWND*);
static HRESULT SUGARCALL iddc_initialize(iddc*, LPDIRECTDRAW, DWORD);
static HRESULT SUGARCALL iddc_is_clip_list_changed(iddc*, LPBOOL);
static HRESULT SUGARCALL iddc_set_clip_list(iddc*, LPRGNDATA, DWORD);
static HRESULT SUGARCALL iddc_set_hwnd(iddc*, DWORD, HWND);

typedef struct iddcu_vft {
    IDDCQUERYINTERFACE      QueryInterface;
    IDDCADDREF              AddRef;
    IDDCRELEASE             Release;
} iddcu_vft;

const static iddcu_vft iddcu_self = {
    iddc_query_interface,
    iddc_add_ref,
    iddc_remove_ref
};

typedef struct iddc_vft {
    IDDCQUERYINTERFACE      QueryInterface;
    IDDCADDREF              AddRef;
    IDDCRELEASE             Release;
    IDDCGETCLIPLIST         GetClipList;
    IDDCGETHWND             GetHWnd;
    IDDCINITIALIZE          Initialize;
    IDDCISCLIPLISTCHANGED   IsClipListChanged;
    IDDCSETCLIPLIST         SetClipList;
    IDDCSETHWND             SetHWnd;
} iddc_vft;

const static iddc_vft iddc_self = {
    iddc_query_interface,
    iddc_add_ref,
    iddc_remove_ref,
    iddc_get_clip_list,
    iddc_get_hwnd,
    iddc_initialize,
    iddc_is_clip_list_changed,
    iddc_set_clip_list,
    iddc_set_hwnd
};

HRESULT SUGARCALL iddc_create(sugar* manager, const GUID* riid, iddc** object) {
    if (manager == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    LOGENTER(manager->logger, "%s, 0x%p", guid_to_string(riid), object);

    if (riid == NULL || object == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (!IsEqualGUID(&IID_IUnknown, riid)
        && !IsEqualGUID(&IID_IDirectDrawClipper, riid)) {
        LOGLEAVE(manager->logger, E_NOINTERFACE);
    }

    HRESULT hr = DD_OK;
    iddc* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_IDIRECTDRAWCLIPPER, sizeof(iddc), &instance))) {
        if (IsEqualGUID(&IID_IUnknown, riid)) {
            instance->self = &iddcu_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawClipper, riid)) {
            instance->self = &iddc_self;
        }

        instance->manager = manager;
        instance->logger = manager->logger;
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    LOGLEAVESELF(manager->logger, hr, instance);
}

void SUGARCALL iddc_release(iddc* self) {
    if (self != NULL) {
        allocator_free(self->manager->allocator, self);
    }
}

HRESULT SUGARCALL iddc_query_interface(iddc* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p", guid_to_string(riid), object);

    LEAVE(ddc_query_interface(self->instance, riid, object));
}

ULONG SUGARCALL iddc_add_ref(iddc* self) {
    if (self == NULL) {
        return 0;
    }

    REFCOUNT(InterlockedIncrement(&self->refs));
}

ULONG SUGARCALL iddc_remove_ref(iddc* self) {
    if (self == NULL) {
        return 0;
    }

    s32 result = InterlockedDecrement(&self->refs);

    if ((result = max(result, 0)) == 0) {
        self->refs = 0;

        if (self->instance != NULL) {
            ddc_remove_ref(self->instance, self);
        }

        iddc_release(self);
    }

    REFCOUNT(result);
}

HRESULT SUGARCALL iddc_get_clip_list(iddc* self, LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p, 0x%p", rect_to_string(lpRect), lpClipList, lpdwSize);

    LEAVE(ddc_get_clip_list(self->instance, lpRect, lpClipList, lpdwSize));
}

HRESULT SUGARCALL iddc_get_hwnd(iddc* self, HWND* lphWnd) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lphWnd);

    LEAVE(ddc_get_hwnd(self->instance, lphWnd));
}

HRESULT SUGARCALL iddc_initialize(iddc* self, LPDIRECTDRAW lpDD, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%08X", lpDD, dwFlags);

    if (lpDD == NULL || dwFlags != DDCCAPS_NONE) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddc_initialize(self->instance, ((idd*)lpDD)->instance));
}

HRESULT SUGARCALL iddc_is_clip_list_changed(iddc* self, LPBOOL lpbChanged) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpbChanged);

    if (lpbChanged == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddc_is_clip_list_changed(self->instance, lpbChanged));
}

HRESULT SUGARCALL iddc_set_clip_list(iddc* self, LPRGNDATA lpClipList, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%08X", lpClipList, dwFlags);

    if (dwFlags != DDCSCL_NONE) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddc_set_clip_list(self->instance, lpClipList));
}

HRESULT SUGARCALL iddc_set_hwnd(iddc* self, DWORD dwFlags, HWND hWnd) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%08X, 0x%p", dwFlags, hWnd);

    if (dwFlags != DDCSHWND_NONE) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddc_set_hwnd(self->instance, hWnd));
}
