#include "arr.h"

#define DEFAULT_CAPACITY            8
#define DEFAULT_CAPACITY_MULTIPLIER 2

typedef struct arr {
    allocator*          allocator;
    u32                 count, capacity;
    void**              items;
    CRITICAL_SECTION    lock;
} arr;

static HRESULT arr_resize(arr* self);

HRESULT arr_create(allocator* allocator, memory_tag tag, arr** object) {
    if (allocator == NULL || object == NULL) {
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

        allocator_free(allocator, instance);
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

HRESULT arr_add_item(arr* self, void* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (self->capacity < self->count + 1) {
        if (FAILED(hr = arr_resize(self))) {
            goto exit;
        }
    }

    self->items[self->count++] = object;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT arr_get_item(arr* self, u32 index, void** object) {
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

HRESULT arr_remove_item(arr* self, const void* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    for (u32 i = 0; i < self->count; i++) {
        if (self->items[i] == object) {
            MoveMemory(&self->items[i],
                &self->items[i + 1], (self->count - i - 1) * sizeof(void*));
            self->count--;
            goto exit;
        }
    }

    hr = DDERR_NOTFOUND;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT arr_get_index(arr* self, const void* object, u32* index) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL || index == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    for (u32 i = 0; i < self->count; i++) {
        if (self->items[i] == object) {
            *index = i;
            goto exit;
        }
    }

    hr = DDERR_NOTFOUND;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

u32 arr_get_count(arr* self) {
    return self == NULL ? 0 : self->count;
}

HRESULT arr_resize(arr* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    const u32 capacity = max(self->capacity, 1) * DEFAULT_CAPACITY_MULTIPLIER;
    if (SUCCEEDED(hr = allocator_reallocate(self->allocator, self->items, capacity * sizeof(void*), (void**)&self->items))) {
        self->capacity = capacity;
    }

    return hr;
}
