#pragma once

#include "allocator.h"

typedef struct connector connector;

HRESULT connector_create(allocator* allocator, memory_tag tag, connector** object);
void connector_release(connector* self);

HRESULT connector_add_item(connector* self, void* item);
HRESULT connector_get_item(connector* self, s32 index, void* object);
HRESULT connector_remove_item(connector* self, s32 index);

s32 connector_get_count(connector* self);
