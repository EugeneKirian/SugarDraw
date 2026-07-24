#include "ddp.h"
#include "idd.h"
#include "iddp.h"

static HRESULT SUGARCALL iddp_get_caps(iddp*, LPDWORD);
static HRESULT SUGARCALL iddp_get_entries(iddp*, DWORD, DWORD, DWORD, LPPALETTEENTRY);
static HRESULT SUGARCALL iddp_initialize(iddp*, LPDIRECTDRAW, DWORD, LPPALETTEENTRY);
static HRESULT SUGARCALL iddp_set_entries(iddp*, DWORD, DWORD, DWORD, LPPALETTEENTRY);

typedef struct iddpu_vft {
    IDDPQUERYINTERFACE          QueryInterface;
    IDDPADDREF                  AddRef;
    IDDPRELEASE                 Release;
} iddpu_vft;

const static iddpu_vft iddpu_self = {
    iddp_query_interface,
    iddp_add_ref,
    iddp_remove_ref
};

typedef struct iddp_vft {
    IDDPQUERYINTERFACE          QueryInterface;
    IDDPADDREF                  AddRef;
    IDDPRELEASE                 Release;
    IDDPGETCAPS                 GetCaps;
    IDDPGETENTRIES              GetEntries;
    IDDPINITIALIZE              Initialize;
    IDDPSETENTRIES              SetEntries;
} iddp_vft;

const static iddp_vft iddp_self = {
    iddp_query_interface,
    iddp_add_ref,
    iddp_remove_ref,
    iddp_get_caps,
    iddp_get_entries,
    iddp_initialize,
    iddp_set_entries
};

HRESULT SUGARCALL iddp_create(sugar* manager, const GUID* riid, iddp** object) {
    if (manager == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (riid == NULL || object == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (!IsEqualGUID(&IID_IUnknown, riid)
        && !IsEqualGUID(&IID_IDirectDrawPalette, riid)) {
        LOGLEAVE(manager->logger, E_NOINTERFACE);
    }

    HRESULT hr = DD_OK;
    iddp* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_IDIRECTDRAWPALETTE, sizeof(iddp), &instance))) {
        if (IsEqualGUID(&IID_IUnknown, riid)) {
            instance->self = &iddpu_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawPalette, riid)) {
            instance->self = &iddp_self;
        }

        instance->manager = manager;
        instance->logger = manager->logger;
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    LOGLEAVESELF(manager->logger, hr, instance);
}

void SUGARCALL iddp_release(iddp* self) {
    if (self != NULL) {
        allocator_free(self->manager->allocator, self);
    }
}

HRESULT SUGARCALL iddp_query_interface(iddp* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p", guid_to_string(riid), object);

    LEAVE(ddp_query_interface(self->instance, riid, object));
}

ULONG SUGARCALL iddp_add_ref(iddp* self) {
    if (self == NULL) {
        return 0;
    }

    REFCOUNT(InterlockedIncrement(&self->refs));
}

ULONG SUGARCALL iddp_remove_ref(iddp* self) {
    if (self == NULL) {
        return 0;
    }

    s32 result = InterlockedDecrement(&self->refs);

    if ((result = max(result, 0)) == 0) {
        self->refs = 0;

        if (self->instance != NULL) {
            ddp_remove_ref(self->instance, self);
        }

        iddp_release(self);
    }

    REFCOUNT(result);
}

HRESULT SUGARCALL iddp_get_caps(iddp* self, LPDWORD lpdwCaps) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpdwCaps);

    LEAVE(ddp_get_caps(self->instance, lpdwCaps));
}

HRESULT SUGARCALL iddp_get_entries(iddp* self, DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%08X, %u, %u, 0x%p", dwFlags, dwBase, dwNumEntries, lpEntries);

    LEAVE(ddp_get_entries(self->instance, dwFlags, dwBase, dwNumEntries, lpEntries));
}

HRESULT SUGARCALL iddp_initialize(iddp* self, LPDIRECTDRAW lpDD, DWORD dwFlags, LPPALETTEENTRY lpDDColorTable) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%08X, 0x%p", lpDD, dwFlags, lpDDColorTable);

    if (lpDD == NULL || dwFlags != DDPCAPS_NONE || lpDDColorTable != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(ddp_initialize(self->instance, ((idd*)lpDD)->instance, DDPCAPS_NONE));
}

HRESULT SUGARCALL iddp_set_entries(iddp* self, DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%08X, %u, %u, 0x%p", dwFlags, dwStartingEntry, dwCount, lpEntries);

    LEAVE(ddp_set_entries(self->instance, dwFlags, dwStartingEntry, dwCount, lpEntries));
}
