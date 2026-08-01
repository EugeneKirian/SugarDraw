#pragma once

#include "common.h"

typedef enum memory_tag {
    MEM_TAG_NONE                        = 0,
    MEM_TAG_SUGAR                       = 1,
    MEM_TAG_BLITTER                     = 2,
    MEM_TAG_CLASSFACTORY                = 3,
    MEM_TAG_ICLASSFACTORY               = 4,
    MEM_TAG_DIRECTDRAW                  = 5,
    MEM_TAG_IDIRECTDRAW                 = 6,
    MEM_TAG_DIRECTDRAWFACTORY           = 7,
    MEM_TAG_IDIRECTDRAWFACTORY          = 8,
    MEM_TAG_DIRECTDRAWGRAPHICS          = 9,
    MEM_TAG_DIRECTDRAWCLIPPER           = 10,
    MEM_TAG_IDIRECTDRAWCLIPPER          = 11,
    MEM_TAG_DIRECTDRAWCOLORCONTROL      = 12,
    MEM_TAG_IDIRECTDRAWCOLORCONTROL     = 13,
    MEM_TAG_DIRECTDRAWGAMMACONTROL      = 14,
    MEM_TAG_IDIRECTDRAWGAMMACONTROL     = 15,
    MEM_TAG_DIRECTDRAWPALETTE           = 16,
    MEM_TAG_IDIRECTDRAWPALETTE          = 17,
    MEM_TAG_DIRECTDRAWSURFACE           = 18,
    MEM_TAG_DIRECTDRAWSURFACEDATA       = 19,
    MEM_TAG_DIRECTDRAWSURFACEFLIPCHAIN  = 20,
    MEM_TAG_IDIRECTDRAWSURFACE          = 21,
    MEM_TAG_COUNT                       = 22,
    MEM_TAG_FORCE_DWORD                 = 0x7FFFFFFF
} memory_tag;

typedef struct allocator allocator;

HRESULT allocator_create(allocator** object);
void allocator_release(allocator* self);

HRESULT allocator_allocate(allocator* self, memory_tag tag, size_t size, void** object);
HRESULT allocator_reallocate(allocator* self, void* ptr, size_t size, void** object);
HRESULT allocator_free(allocator* self, void* object);
