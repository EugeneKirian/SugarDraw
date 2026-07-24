#pragma once

#include "arr.h"

typedef struct dds dds;

typedef struct ddsfc {
    allocator*          allocator;
    dds*                instance;
    arr*                surfaces;
    CRITICAL_SECTION    lock;
} ddsfc;

HRESULT ddsfc_create(allocator* allocator, dds* surface, ddsfc** object);
void ddsfc_release(ddsfc* self);

HRESULT ddsfc_add_surface(ddsfc* self, dds* surface);
HRESULT ddsfc_remove_surface(ddsfc* self, dds* surface);

HRESULT ddsfc_flip(ddsfc* self, dds* override);
