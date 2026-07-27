#include "lock.h"

#define DEFAULT_CAPACITY            8
#define DEFAULT_CAPACITY_MULTIPLIER 2

struct lock {
    allocator*          allocator;
    s32                 count, capacity;
    RECT*               items;
    CRITICAL_SECTION    lock;
};

static HRESULT lock_add_item(lock* self, const RECT* rect);
static HRESULT lock_remove_item(lock* self, const RECT* rect);
static HRESULT lock_resize(lock* self);

HRESULT lock_create(allocator* allocator, memory_tag tag, lock** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (tag < MEM_TAG_NONE || tag >= MEM_TAG_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    lock* instance = NULL;

    if (SUCCEEDED(hr = allocator_allocate(allocator, tag, sizeof(lock), &instance))) {
        instance->allocator = allocator;
        instance->count = 0;
        instance->capacity = DEFAULT_CAPACITY;
        if (SUCCEEDED(hr = allocator_allocate(allocator, tag,
            instance->capacity * sizeof(RECT), &instance->items))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
            return hr;
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void lock_release(lock* self) {
    if (self != NULL) {
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self->items);
        allocator_free(self->allocator, self);
    }
}

HRESULT lock_clear(lock* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    EnterCriticalSection(&self->lock);
    self->count = 0;
    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT lock_get_item(lock* self, s32 index, RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->count < index + 1 || rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&self->lock);
    CopyMemory(rect, &self->items[index], sizeof(RECT));
    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

HRESULT lock_acquire(lock* self, const RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsValidRect(rect)) {
        return DDERR_INVALIDRECT;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    MAKETYPE(RECT, overlap);
    for (s32 i = 0; i < self->count; i++) {
        if (IntersectRect(&overlap, rect, &self->items[i])) {
            EXITCODE(DDERR_SURFACEBUSY);
        }
    }

    hr = lock_add_item(self, rect);

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT lock_unacquire(lock* self, const RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsValidRect(rect)) {
        return DDERR_INVALIDRECT;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    for (s32 i = 0; i < self->count; i++) {
        if (CompareMemory(rect, &self->items[i], sizeof(RECT))) {
            EXITCODE(lock_remove_item(self, rect));
        }
    }

    hr = DDERR_NOTLOCKED;

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT lock_add_item(lock* self, const RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (self->capacity < self->count + 1) {
        if (FAILED(hr = lock_resize(self))) {
            goto exit;
        }
    }

    CopyMemory(&self->items[self->count++], rect, sizeof(RECT));

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT lock_remove_item(lock* self, const RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->count == 0) {
        return DDERR_NOTFOUND;
    }

    EnterCriticalSection(&self->lock);

    for (s32 i = 0; i < self->count; i++) {
        if (CompareMemory(&self->items[i], rect, sizeof(RECT))) {
            for (s32 k = i; k < self->count - 1; k++) {
                CopyMemory(&self->items[k], &self->items[k + 1], sizeof(RECT));
            }

            self->count--;

            break;
        }
    }

    LeaveCriticalSection(&self->lock);

    return DD_OK;
}

s32 lock_get_count(lock* self) {
    return self == NULL ? 0 : self->count;
}

HRESULT lock_resize(lock* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    const size_t capacity = max(self->capacity, 1) * DEFAULT_CAPACITY_MULTIPLIER;
    const size_t size = capacity * sizeof(RECT);

    if (SUCCEEDED(hr = allocator_reallocate(self->allocator, self->items, size, &self->items))) {
        self->capacity = capacity;
    }

    return hr;
}
