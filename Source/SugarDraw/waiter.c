#include "waiter.h"

#define DEFAULT_CAPACITY            8
#define DEFAULT_CAPACITY_MULTIPLIER 2

typedef struct waiter {
    allocator*          allocator;
    u32                 count, capacity;
    HANDLE*             events;
    CRITICAL_SECTION    lock;
} waiter;

static HRESULT waiter_resize(waiter* self);

HRESULT waiter_create(allocator* allocator, memory_tag tag, waiter** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    waiter* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, tag, sizeof(waiter), &instance))) {
        instance->allocator = allocator;
        instance->count = 0;
        instance->capacity = DEFAULT_CAPACITY;
        if (SUCCEEDED(hr = allocator_allocate(allocator, tag,
            instance->capacity * sizeof(HANDLE), (void**)&instance->events))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void waiter_release(waiter* self) {
    if (self != NULL) {
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self->events);
        allocator_free(self->allocator, self);
    }
}

HRESULT waiter_add(waiter* self, HANDLE event) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (event == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (self->capacity < self->count + 1) {
        if (FAILED(hr = waiter_resize(self))) {
            goto exit;
        }
    }

    self->events[self->count++] = event;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT waiter_remove(waiter* self, HANDLE event) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (event == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    for (u32 i = 0; i < self->count; i++) {
        if (self->events[i] == event) {
            MoveMemory(&self->events[i],
                &self->events[i + 1], (self->count - i - 1) * sizeof(HANDLE));
            self->count--;
            goto exit;
        }
    }

    hr = DDERR_NOTFOUND;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT waiter_set(waiter* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    for (u32 i = 0; i < self->count; i++) {
        SetEvent(self->events[i]);
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

u32 waiter_get_count(waiter* self) {
    return self == NULL ? 0 : self->count;
}

HRESULT waiter_resize(waiter* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    const u32 capacity = max(self->capacity, 1) * DEFAULT_CAPACITY_MULTIPLIER;
    if (SUCCEEDED(hr = allocator_reallocate(self->allocator, self->events, capacity * sizeof(HANDLE), (void**)&self->events))) {
        self->capacity = capacity;
    }

    return hr;
}
