#pragma once
#include "allocator.h"

typedef struct logger logger;

HRESULT logger_create(allocator* allocator, const char* path, log_level level, logger** object);
void logger_release(logger* self);

HRESULT logger_log(logger* self, log_level level, const char* format, ...);
