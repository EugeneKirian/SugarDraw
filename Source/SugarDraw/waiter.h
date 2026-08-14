#pragma once

#include "allocator.h"

typedef struct waiter waiter;

HRESULT waiter_create(allocator* allocator, memory_tag tag, waiter** object);
void waiter_release(waiter* self);

HRESULT waiter_add(waiter* self, HANDLE event);
HRESULT waiter_remove(waiter* self, HANDLE event);

HRESULT waiter_set(waiter* self);

u32 waiter_get_count(waiter* self);
