#pragma once

#include "allocator.h"

typedef struct intfc intfc;

HRESULT intfc_create(allocator* allocator, memory_tag tag, intfc** object);
void intfc_release(intfc* self);

HRESULT intfc_get_item(intfc* self, u32 index, void** object);
HRESULT intfc_query_item(intfc* self, const GUID* riid, void** object);

HRESULT intfc_add_item(intfc* self, const GUID* riid, void* object);
HRESULT intfc_remove_item(intfc* self, const GUID* riid);

u32 intfc_get_count(intfc* self);
