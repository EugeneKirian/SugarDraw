#pragma once

#include "allocator.h"

// Run at 60 Hz -> ~16.666 ms -> 166,666 units of 100-nanoseconds.

typedef struct timer timer;

HRESULT timer_create(allocator* allocator, timer** object);
void timer_release(timer* self);

HRESULT timer_start(timer* self, s64 period);
HRESULT timer_stop(timer* self);

HRESULT timer_register_event(timer* self, HANDLE event);
HRESULT timer_unregister_event(timer* self, HANDLE event);
