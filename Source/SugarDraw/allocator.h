#pragma once
#include "common.h"

typedef struct allocator allocator;

HRESULT allocator_create(allocator** object);
void allocator_release(allocator* self);

HRESULT allocator_allocate(allocator* self, memory_tag tag, size_t size, void** object);
HRESULT allocator_reallocate(allocator* self, void* ptr, size_t size, void** object);
HRESULT allocator_free(allocator* self, void* object);
