#pragma once

#include "common.h"

typedef enum memory_tag {
    MEM_TAG_NONE                = 0,
    MEM_TAG_SUGAR               = 1,
    MEM_TAG_DIRECTDRAW          = 2,
    MEM_TAG_IDIRECTDRAW         = 3,
    MEM_TAG_DIRECTDRAWSURFACE   = 4,
    MEM_TAG_IDIRECTDRAWSURFACE  = 5,
    MEM_TAG_DIRECTDRAWPALETTE   = 6,
    MEM_TAG_IDIRECTDRAWPALETTE  = 7,
    MEM_TAG_COUNT               = 8,
    MEM_TAG_FORCE_DWORD         = 0x7FFFFFFF
} memory_tag;

typedef struct allocator allocator;

HRESULT allocator_create(allocator** object);
void allocator_release(allocator* self);

HRESULT allocator_allocate(allocator* self, memory_tag tag, size_t size, void** object);
HRESULT allocator_reallocate(allocator* self, void* ptr, size_t size, void** object);
HRESULT allocator_free(allocator* self, void* object);
