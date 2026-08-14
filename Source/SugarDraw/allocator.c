#include "allocator.h"

#define SIGNATURE                   0xCAFEBABE

#define HEAP_NONE					0

#define ALLOCATOR_MAX_NODE_COUNT    128
#define ALLOCATOR_MAX_MEMORY_SIZE   0x7FFFFFFF

typedef struct node {
    memory_tag  tag;
    size_t      size;
    u8*         data;
    u32*        signature;
} node;

typedef struct block {
    struct block*   next;
    node*           nodes[ALLOCATOR_MAX_NODE_COUNT];
} block;

typedef struct allocator {
    HANDLE              heap;
    block*              blocks;
    CRITICAL_SECTION    lock;
} allocator;

static HRESULT allocator_create_block(allocator* self, block** object);
static HRESULT allocator_create_node(allocator* self, memory_tag tag, size_t size, node** object);
static HRESULT allocator_reallocate_node(allocator* self, size_t size, node* current, void** object);

HRESULT allocator_create(allocator** object) {
    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HANDLE heap = GetProcessHeap();
    if (heap == NULL) {
        return DDERR_GENERIC;
    }

    allocator* result =
        (allocator*)HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(allocator));
    if (result == NULL) {
        return DDERR_OUTOFMEMORY;
    }

    result->heap = heap;

    HRESULT hr = DD_OK;
    if (FAILED(hr = allocator_create_block(result, &result->blocks))) {
        HeapFree(heap, HEAP_NONE, result);
        return hr;
    }

    InitializeCriticalSection(&result->lock);

    *object = result;

    return hr;
}

void allocator_release(allocator* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->blocks != NULL) {
            for (block* it = self->blocks; it->next != NULL; it = it->next) {
                for (size_t i = 0; i < ALLOCATOR_MAX_NODE_COUNT; i++) {
                    node* item = it->nodes[i];
                    if (item != NULL) {
                        HeapFree(self->heap, HEAP_NONE, item);
                    }
                }
            }

            block* next = self->blocks;
            while (next != NULL) {
                block* current = next;
                next = current->next;
                HeapFree(self->heap, HEAP_NONE, current);
            }
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);
        HeapFree(self->heap, HEAP_NONE, self);
    }
}

HRESULT allocator_allocate(allocator* self, memory_tag tag, size_t size, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (tag < MEM_TAG_NONE || tag >= MEM_TAG_COUNT
        || size > ALLOCATOR_MAX_MEMORY_SIZE || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // Search for a block with available node for allocation.
    for (block* it = self->blocks; it != NULL; it = it->next) {
        for (size_t i = 0; i < ALLOCATOR_MAX_NODE_COUNT; i++) {
            if (it->nodes[i] == NULL) {
                if (SUCCEEDED(hr = allocator_create_node(self, tag, size, &it->nodes[i]))) {
                    *object = it->nodes[i]->data;
                }

                goto exit;
            }
        }
    }

    // If no block with available node was found,
    // allocate a new block and then allocate a node in it.
    block* next = NULL;
    if (SUCCEEDED(hr = allocator_create_block(self, &next))) {
        for (block* it = self->blocks; it != NULL; it = it->next) {
            if (it->next == NULL) {
                it->next = next;
                if (SUCCEEDED(hr = allocator_create_node(self, tag, size, &it->nodes[0]))) {
                    *object = it->nodes[0]->data;
                }

                goto exit;
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT allocator_reallocate(allocator* self, void* ptr, size_t size, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (ptr == NULL || size > ALLOCATOR_MAX_MEMORY_SIZE || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DDERR_INVALIDPARAMS;
    EnterCriticalSection(&self->lock);

    for (block* it = self->blocks; it != NULL; it = it->next) {
        for (size_t i = 0; i < ALLOCATOR_MAX_NODE_COUNT; i++) {
            node* item = it->nodes[i];
            if (item != NULL && item->data == ptr) {
                const u32 signature = *item->signature;

                if (SUCCEEDED(hr = (signature == SIGNATURE ? DD_OK : DDERR_GENERIC))) {
                    hr = allocator_reallocate_node(self, size, item, object);
                }

                goto exit;
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT allocator_free(allocator* self, void* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DDERR_INVALIDPARAMS;
    EnterCriticalSection(&self->lock);

    for (block* it = self->blocks; it != NULL; it = it->next) {
        for (size_t i = 0; i < ALLOCATOR_MAX_NODE_COUNT; i++) {
            node* item = it->nodes[i];
            if (item != NULL && item->data == object) {
                const u32 signature = *item->signature;

                if (SUCCEEDED(hr = (signature == SIGNATURE ? DD_OK : DDERR_GENERIC))) {
                    HeapFree(self->heap, HEAP_NONE, item);
                    it->nodes[i] = NULL;
                }

                goto exit;
            }
        }
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT allocator_create_block(allocator* self, block** object) {
    block* result =
        (block*)HeapAlloc(self->heap, HEAP_ZERO_MEMORY, sizeof(block));

    if (result == NULL) {
        return DDERR_OUTOFMEMORY;
    }

    *object = result;

    return DD_OK;
}

HRESULT allocator_create_node(allocator* self, memory_tag tag, size_t size, node** object) {
    node* result =
        (node*)HeapAlloc(self->heap, HEAP_ZERO_MEMORY, sizeof(node));
    if (result == NULL) {
        return DDERR_OUTOFMEMORY;
    }

    result->tag = tag;
    result->size = size;
    result->data =
        (u8*)HeapAlloc(self->heap, HEAP_ZERO_MEMORY, size + sizeof(u32));
    if (result->data == NULL) {
        HeapFree(self->heap, HEAP_NONE, result);
        return DDERR_OUTOFMEMORY;
    }

    result->signature = (u32*)(result->data + size);
    *result->signature = SIGNATURE;

    *object = result;

    return DD_OK;
}

HRESULT allocator_reallocate_node(allocator* self, size_t size, node* current, void** object) {
    u8* result =
        (u8*)HeapReAlloc(self->heap, HEAP_NONE, current->data, size + sizeof(u32));
    if (result == NULL) {
        return DDERR_OUTOFMEMORY;
    }

    current->size = size;
    current->data = result;

    current->signature = (u32*)(current->data + size);
    *current->signature = SIGNATURE;

    *object = result;

    return DD_OK;
}
