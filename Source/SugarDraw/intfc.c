#include "intfc.h"

#define DEFAULT_CAPACITY            8
#define DEFAULT_CAPACITY_MULTIPLIER 2

typedef struct intf {
    GUID    id;
    void*   item;
} intf;

typedef struct intfc {
    allocator*          allocator;
    u32                 count, capacity;
    intf*               items;
    CRITICAL_SECTION    lock;
} intfc;

static HRESULT intfc_resize(intfc* self);

HRESULT intfc_create(allocator* allocator, memory_tag tag, intfc** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    intfc* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, tag, sizeof(intfc), &instance))) {
        instance->allocator = allocator;
        instance->count = 0;
        instance->capacity = DEFAULT_CAPACITY;
        if (SUCCEEDED(hr = allocator_allocate(allocator, tag,
            instance->capacity * sizeof(intf), &instance->items))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void intfc_release(intfc* self) {
    if (self != NULL) {
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self->items);
        allocator_free(self->allocator, self);
    }
}

HRESULT intfc_get_item(intfc* self, u32 index, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->count < index + 1 || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);
    *object = self->items[index].item;
    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT intfc_query_item(intfc* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->count == 0) {
        return E_NOINTERFACE;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    for (u32 i = 0; i < self->count; i++) {
        if (IsEqualGUID(riid, &self->items[i].id)) {
            *object = self->items[i].item;
            EXITCODE(DD_OK);
        }
    }

    hr = E_NOINTERFACE;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT intfc_add_item(intfc* self, const GUID* riid, void* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (self->capacity < self->count + 1) {
        if (FAILED(hr = intfc_resize(self))) {
            goto exit;
        }
    }

    CopyMemory(&self->items[self->count].id, riid, sizeof(GUID));
    self->items[self->count++].item = object;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT intfc_remove_item(intfc* self, const GUID* riid) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->count == 0) {
        return DDERR_NOTFOUND;
    }

    EnterCriticalSection(&self->lock);

    for (u32 i = 0; i < self->count; i++) {
        if (IsEqualGUID(riid, &self->items[i].id)) {
            MoveMemory(&self->items[i],
                &self->items[i + 1], (self->count - i - 1) * sizeof(intf));
            self->count--;
            break;
        }
    }

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

u32 intfc_get_count(intfc* self) {
    return self == NULL ? 0 : self->count;
}

HRESULT intfc_resize(intfc* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    const u32 capacity = max(self->capacity, 1) * DEFAULT_CAPACITY_MULTIPLIER;
    if (SUCCEEDED(hr = allocator_reallocate(self->allocator, self->items, capacity * sizeof(intf), &self->items))) {
        self->capacity = capacity;
    }

    return hr;
}
