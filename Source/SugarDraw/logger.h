#pragma once
#include "allocator.h"

typedef enum log_level {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARNING = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_TRACE = 3,
    LOG_LEVEL_COUNT = 4,
    LOG_LEVEL_FORCE_DWORD = 0x7FFFFFFF
} log_level;

typedef struct logger logger;

HRESULT logger_create(allocator* allocator, const char* path, log_level level, logger** object);
void logger_release(logger* self);

HRESULT logger_log(logger* self, log_level level, const char* format, ...);
