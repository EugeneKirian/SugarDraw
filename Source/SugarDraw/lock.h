#pragma once

#include "allocator.h"

typedef struct lock lock;

HRESULT lock_create(allocator* allocator, memory_tag tag, lock** object);
void lock_release(lock* self);

HRESULT lock_clear(lock* self);
HRESULT lock_get_item(lock* self, u32 index, RECT* rect);

HRESULT lock_acquire(lock* self, const RECT* rect);
HRESULT lock_unacquire(lock* self, const RECT* rect);

s32 lock_get_count(lock* self);
