#include "arena.h"

#define ARENA_BLOCK_SIZE        (256 * 1024)   /* 256 KB */
#define ARENA_ALIGNMENT         64

#define ARENA_ALIGN(X, A)       (((size_t)(X) + (A) - 1) & ~((A) - 1))
#define ARENA_ALIGNBLOCK(X)     ARENA_ALIGN((X), ARENA_ALIGNMENT)

typedef struct block block;

struct arena {
    allocator*  allocator;
    memory_tag  tag;
    block*      head;
    block*      tail;
};

struct block {
    block*      next;
    size_t      size, capacity;
    byte*       data;
};

static HRESULT block_create(arena* arena, const size_t size, block** object);
static HRESULT block_allocate(block* self, const size_t size, void** object);

HRESULT arena_create(allocator* allocator, memory_tag tag, arena** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    arena* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, tag, sizeof(arena), &instance))) {
        instance->allocator = allocator;
        instance->tag = tag;
        *object = instance;
    }

    return hr;
}

void arena_release(arena* self) {
    if (self != NULL) {
        block* current = self->head;
        while (current != NULL) {
            block* next = current->next;
            allocator_free(self->allocator, current);
            current = next;
        }

        allocator_free(self->allocator, self);
    }
}

HRESULT arena_allocate(arena* self, const size_t size, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (size == 0 || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    block* current = self->head;
    const size_t allocation = ARENA_ALIGNBLOCK(size);

    // Search for a block that has enough space for the allocation.
    while (current != NULL) {
        if (allocation <= current->capacity - current->size) {
            return block_allocate(current, size, object);
        }

        // Next to the next block, if any...
        current = current->next;
    }

    // Allocate new block if existing blocks are unable to host requested size.
    HRESULT hr = DD_OK;
    block* instance = NULL;
    if (SUCCEEDED(hr = block_create(self, size, &instance))) {
        if (self->head == NULL) {
            self->head = instance;
            self->tail = instance;
        }
        else {
            self->tail->next = instance;
            self->tail = instance;
        }

        return block_allocate(instance, size, object);
    }

    return hr;
}

HRESULT arena_clear(arena* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    block* current = self->head;
    while (current != NULL) {
        current->size = 0;
        current = current->next;
    }

    return DD_OK;
}


HRESULT block_create(arena* arena, const size_t size, block** object) {
    if (arena == NULL || size == 0 || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    const size_t length = ARENA_ALIGNBLOCK(size);

    size_t capacity = ARENA_BLOCK_SIZE;
    while (capacity < length) {
        capacity += ARENA_BLOCK_SIZE;
    }

    const size_t offset = ARENA_ALIGNBLOCK(sizeof(block));
    const size_t total = capacity + offset + (ARENA_ALIGNMENT - 1);

    HRESULT hr = DD_OK;
    block* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(arena->allocator, arena->tag, total, &instance))) {
        instance->capacity = capacity;
        instance->data = (byte*)ARENA_ALIGNBLOCK((size_t)instance + offset);

        *object = instance;
    }

    return hr;
}

HRESULT block_allocate(block* self, const size_t size, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (size == 0 || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    const size_t length = ARENA_ALIGNBLOCK(size);

    byte* init = self->data + self->size;
    byte* result = (byte*)ARENA_ALIGNBLOCK(init);

    ZeroMemory(result, size);

    self->size += length;

    *object = result;

    return S_OK;
}
