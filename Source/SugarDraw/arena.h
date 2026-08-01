#pragma once

#include "allocator.h"

typedef struct arena arena;

HRESULT arena_create(allocator* allocator, memory_tag tag, arena** object);
void arena_release(arena* self);

HRESULT arena_allocate(arena* self, const size_t size, void** object);
HRESULT arena_clear(arena* self);
