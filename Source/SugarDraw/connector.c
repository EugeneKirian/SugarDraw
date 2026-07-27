#include "connector.h"

#define DEFAULT_CAPACITY            8
#define DEFAULT_CAPACITY_MULTIPLIER 2

typedef struct conn {
    GUID                id;
    void*               instance;
} conn;

struct connector {
    allocator*          allocator;
    u32                 count, capacity;
    conn*               items;
    CRITICAL_SECTION    lock;
};

static HRESULT connector_resize(connector* self);

HRESULT connector_create(allocator* allocator, memory_tag tag, connector** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (tag < MEM_TAG_NONE || tag >= MEM_TAG_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    connector* instance = NULL;

    if (SUCCEEDED(hr = allocator_allocate(allocator, tag, sizeof(connector), &instance))) {
        instance->allocator = allocator;
        instance->count = 0;
        instance->capacity = DEFAULT_CAPACITY;
        if (SUCCEEDED(hr = allocator_allocate(allocator, tag,
            instance->capacity * sizeof(conn), (void**)&instance->items))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void connector_release(connector* self) {
    if (self != NULL) {
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self->items);
        allocator_free(self->allocator, self);
    }
}

HRESULT connector_add_item(connector* self, void* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (self->capacity < self->count + 1) {
        if (FAILED(hr = connector_resize(self))) {
            goto exit;
        }
    }

    CopyMemory(&self->items[self->count++], object, sizeof(conn));

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT connector_get_item(connector* self, u32 index, void* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->count < index + 1 || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);
    CopyMemory(object, &self->items[index], sizeof(conn));
    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT connector_remove_item(connector* self, u32 index) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->count < index + 1) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);

    if (self->count != index + 1) {
        MoveMemory(&self->items[index],
            &self->items[index + 1], (self->count - index - 1) * sizeof(conn));
    }

    self->count--;
    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

u32 connector_get_count(connector* self) {
    return self == NULL ? 0 : self->count;
}

HRESULT connector_resize(connector* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    const size_t capacity = max(self->capacity, 1) * DEFAULT_CAPACITY_MULTIPLIER;
    const size_t size = capacity * sizeof(conn);

    if (SUCCEEDED(hr = allocator_reallocate(self->allocator, self->items, size, &self->items))) {
        self->capacity = capacity;
    }

    return hr;
}
