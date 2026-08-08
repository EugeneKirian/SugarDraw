#pragma once

#include "allocator.h"

typedef struct region {
    allocator*  allocator;
    u32         capacity;
    RGNDATA*    region;
} region;

HRESULT region_create(allocator* allocator, memory_tag tag, region** object);
void region_release(region* self);

HRESULT region_clear(region* self);
HRESULT region_add_rect(region* self, const RECT* rect);
HRESULT region_add_region(region* self, const RGNDATA* region);
HRESULT region_set_region(region* self, const RGNDATA* region);
HRESULT region_clip_rect(region* self, const RECT* rect);
HRESULT region_clip_region(region* self, const RGNDATA* region);
