#pragma once

#include "allocator.h"

typedef struct connector connector;

HRESULT connector_create(allocator* allocator, memory_tag tag, connector** object);
void connector_release(connector* self);

HRESULT connector_add_item(connector* self, void* object);
HRESULT connector_get_item(connector* self, u32 index, void* object);
HRESULT connector_remove_item(connector* self, const void* object);

u32 connector_get_count(connector* self);
