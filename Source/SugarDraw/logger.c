#include "allocator.h"
#include "logger.h"
#include "utilities.h"

#include <stdio.h>
#include <stdarg.h>

struct logger {
    allocator*          allocator;
    log_level           level;
    HANDLE              handle;
    char*               path;
    CRITICAL_SECTION    lock;
};

#define LOGGER_MAX_LOG_MESSAGE_LENGTH   1024

static HRESULT logger_allocate(allocator* allocator, const char* path, logger** object) {
    if (allocator == NULL || path == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    logger* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_NONE, sizeof(logger), &instance))) {
        if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_NONE, strlen(path) + 1, &instance->path))) {
            *object = instance;
            return hr;
        }

        logger_release(instance);
    }

    return hr;
}

HRESULT logger_create(allocator* allocator, const char* path, log_level level, logger** object) {
    if (allocator == NULL || path == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (level < LOG_LEVEL_ERROR || level >= LOG_LEVEL_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    logger* instance = NULL;
    if (SUCCEEDED(hr = logger_allocate(allocator, path, &instance))) {
        instance->allocator = allocator;
        instance->level = level;
        instance->handle = CreateFileA(path, GENERIC_WRITE,
            FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (instance->handle == INVALID_HANDLE_VALUE) {
            logger_release(instance);
            return DDERR_GENERIC;
        }

        strcpy(instance->path, path);
        InitializeCriticalSection(&instance->lock);

        *object = instance;
    }

    return hr;
}

void logger_release(logger* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        if (self->path != NULL) {
            allocator_free(self->allocator, self->path);
        }

        if (self->handle != INVALID_HANDLE_VALUE) {
            CloseHandle(self->handle);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);
    }
}

HRESULT logger_log(logger* self, log_level level, const char* format, ...) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (level < LOG_LEVEL_ERROR || level >= LOG_LEVEL_COUNT) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->handle == INVALID_HANDLE_VALUE) {
        return DDERR_GENERIC;
    }

    HRESULT hr = DD_OK;
    char prefix[LOGGER_MAX_LOG_MESSAGE_LENGTH];
    char buffer[LOGGER_MAX_LOG_MESSAGE_LENGTH];

    if (level <= self->level) {
        SYSTEMTIME time;
        GetLocalTime(&time);
        if (sprintf_s(prefix, LOGGER_MAX_LOG_MESSAGE_LENGTH, "[%s][%d-%02d-%02d %02d:%02d:%02d:%02d] %s\r\n",
            log_level_to_string(level), time.wYear, time.wMonth, time.wDay,
            time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, format) < 0) {
            return DDERR_GENERIC;
        }

        va_list args;
        va_start(args, format);
        s32 length = vsprintf_s(buffer, LOGGER_MAX_LOG_MESSAGE_LENGTH - 1, prefix, args);
        va_end(args);

        if (length < 0) {
            return DDERR_GENERIC;
        }

        // Null-terminate the string, just in case.
        length = length < LOGGER_MAX_LOG_MESSAGE_LENGTH
            ? length : LOGGER_MAX_LOG_MESSAGE_LENGTH - 1;
        buffer[length] = (char)NULL;

        EnterCriticalSection(&self->lock);

        DWORD bytes = 0;
        hr = WriteFile(self->handle, buffer, (u32)length, &bytes, NULL) && bytes != (u32)length
            ? DD_OK : DDERR_GENERIC;

        LeaveCriticalSection(&self->lock);
    }

    return hr;
}
