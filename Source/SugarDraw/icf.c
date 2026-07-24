#include "icf.h"
#include "cf.h"

static HRESULT SUGARCALL icf_create_instance(icf*, LPUNKNOWN, REFIID, LPVOID*);
static HRESULT SUGARCALL icf_lock_server(icf*, BOOL);

typedef struct icfu_vft {
    ICFQUERYINTERFACE           QueryInterface;
    ICFADDREF                   AddRef;
    ICFRELEASE                  Release;
} icfu_vft;

const static icfu_vft icfu_self = {
    icf_query_interface,
    icf_add_ref,
    icf_remove_ref
};

typedef struct icf_vft {
    ICFQUERYINTERFACE           QueryInterface;
    ICFADDREF                   AddRef;
    ICFRELEASE                  Release;
    ICFCREATEINSTANCE           CreateInstance;
    ICFLOCKSERVER               LockServer;
} icf_vft;

const static icf_vft icf_self = {
    icf_query_interface,
    icf_add_ref,
    icf_remove_ref,
    icf_create_instance,
    icf_lock_server
};

HRESULT SUGARCALL icf_create(sugar* manager, const GUID* riid, icf** object) {
    if (manager == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    LOGENTER(manager->logger, "%s, 0x%p", guid_to_string(riid), object);

    if (riid == NULL || object == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (!IsEqualGUID(&IID_IUnknown, riid)
        && !IsEqualGUID(&IID_IClassFactory, riid)) {
        LOGLEAVE(manager->logger, E_NOINTERFACE);
    }

    HRESULT hr = S_OK;
    icf* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_ICLASSFACTORY, sizeof(icf), &instance))) {
        if (IsEqualGUID(&IID_IUnknown, riid)) {
            instance->self = &icfu_self;
        }
        else if (IsEqualGUID(&IID_IClassFactory, riid)) {
            instance->self = &icf_self;
        }

        instance->manager = manager;
        instance->logger = manager->logger;
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    LOGLEAVESELF(manager->logger, hr, instance);
}

VOID SUGARCALL icf_release(icf* self) {
    if (self != NULL) {
        allocator_free(self->manager->allocator, self);
    }
}

HRESULT SUGARCALL icf_query_interface(icf* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, 0x%p", guid_to_string(riid), object);

    LEAVE(cf_query_interface(self->instance, riid, object));
}

ULONG SUGARCALL icf_add_ref(icf* self) {
    if (self == NULL) {
        return 0;
    }

    REFCOUNT(InterlockedIncrement(&self->refs));
}

ULONG SUGARCALL icf_remove_ref(icf* self) {
    if (self == NULL) {
        return 0;
    }

    s32 result = InterlockedDecrement(&self->refs);

    if ((result = max(result, 0)) == 0) {
        self->refs = 0;

        if (self->instance != NULL) {
            cf_remove_ref(self->instance, self);
        }

        icf_release(self);
    }

    REFCOUNT(result);
}

HRESULT SUGARCALL icf_create_instance(icf* self, LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppOut) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, %s, 0x%p", pUnkOuter, guid_to_string(riid), ppOut);

    if (pUnkOuter != NULL) {
        LEAVE(CLASS_E_NOAGGREGATION);
    }

    if (riid == NULL || ppOut == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(cf_create_instance(self->instance, riid, ppOut));
}

HRESULT SUGARCALL icf_lock_server(icf* self, BOOL bLock) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%08X", bLock);

    LEAVE(DD_OK);
}
