#pragma once

#include "allocator.h"

typedef struct arr arr;

HRESULT arr_create(allocator* allocator, memory_tag tag, arr** object);
void arr_release(arr* self);

HRESULT arr_add_item(arr* self, void* item);
HRESULT arr_get_item(arr* self, int index, void** object);
HRESULT arr_remove_item(arr* self, int index, void** object);

int arr_get_count(arr* self);
