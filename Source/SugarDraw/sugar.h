#pragma once
#include "arr.h"
#include "logger.h"

typedef struct sugar {
    allocator*          allocator;
    logger*             logger;
    arr*                items;
    CRITICAL_SECTION    lock;
} sugar;

HRESULT sugar_create(allocator* allocator, logger* logger, sugar** object);
void sugar_release(sugar* self);

HRESULT sugar_create_direct_draw(sugar* self, const GUID* rclsid, const GUID* riid, void** object);
