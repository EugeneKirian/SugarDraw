#include "arr.h"

#define DEFAULT_CAPACITY            8
#define DEFAULT_CAPACITY_MULTIPLIER 2

struct arr {
    allocator*          allocator;
    int                 count, capacity;
    void**              items;
    CRITICAL_SECTION    lock;
};

static HRESULT arr_resize(arr* self);

HRESULT arr_create(allocator* allocator, memory_tag tag, arr** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (tag < MEM_TAG_NONE || tag >= MEM_TAG_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    arr* instance = NULL;

    if (SUCCEEDED(hr = allocator_allocate(allocator, tag, sizeof(arr), &instance))) {
        instance->allocator = allocator;
        instance->count = 0;
        instance->capacity = DEFAULT_CAPACITY;
        if (SUCCEEDED(hr = allocator_allocate(allocator, tag,
            instance->capacity * sizeof(void*), (void**)&instance->items))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        arr_release(instance);
    }

    return hr;
}

void arr_release(arr* self) {
    if (self != NULL) {
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self->items);
        allocator_free(self->allocator, self);
    }
}

HRESULT arr_add_item(arr* self, void* item) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (item == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (self->capacity < self->count + 1) {
        if (SUCCEEDED(hr = arr_resize(self))) {
            goto exit;
        }
    }

    self->items[self->count++] = item;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT arr_get_item(arr* self, int index, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->count < index + 1 || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);
    *object = self->items[index];
    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT arr_remove_item(arr* self, int index, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->count < index + 1) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);

    if (object != NULL) {
        *object = self->items[index];
    }

    if (self->count != index + 1) {
        MoveMemory(&self->items[index],
            &self->items[index + 1], (self->count - index - 1) * sizeof(void*));
    }

    self->count--;
    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

int arr_get_count(arr* self) {
    return self == NULL ? 0 : self->count;
}

HRESULT arr_resize(arr* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    const size_t capacity = max(self->capacity, 1) * DEFAULT_CAPACITY_MULTIPLIER;
    const size_t size = capacity * sizeof(void*);

    if (SUCCEEDED(hr = allocator_reallocate(self->allocator, self->items, size, (void**)&self->items))) {
        self->capacity = capacity;
    }

    return hr;
}
