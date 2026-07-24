#pragma once

#include "allocator.h"

typedef enum log_level {
    LOG_LEVEL_ERROR         = 0,
    LOG_LEVEL_WARNING       = 1,
    LOG_LEVEL_INFO          = 2,
    LOG_LEVEL_TRACE         = 3,
    LOG_LEVEL_COUNT         = 4,
    LOG_LEVEL_FORCE_DWORD   = 0x7FFFFFFF
} log_level;

typedef struct logger logger;

HRESULT logger_create(allocator* allocator, const char* path, log_level level, logger** object);
void logger_release(logger* self);

HRESULT logger_log(logger* self, log_level level, const char* format, ...);

#if _DEBUG
#define ENTER(M, ...)           logger_log(self->logger, LOG_LEVEL_INFO, "0x%p->%s("##M##")", self, __FUNCTION__, ##__VA_ARGS__)
#define LOGENTER(LOG, M, ...)   logger_log(LOG, LOG_LEVEL_INFO, "%s("##M##")", __FUNCTION__, ##__VA_ARGS__)

#define LEAVE(X)                                                                        \
    const HRESULT __exit__ = X;                                                         \
    logger_log(self->logger, SUCCEEDED(__exit__) ? LOG_LEVEL_INFO : LOG_LEVEL_ERROR,    \
        "0x%p->%s() -> %s", self, __FUNCTION__, hresult_to_string(__exit__));           \
    return __exit__;

#define LOGLEAVE(LOG, X)                                                        \
    const HRESULT __exit__ = X;                                                 \
    logger_log(LOG, SUCCEEDED(__exit__) ? LOG_LEVEL_INFO : LOG_LEVEL_ERROR,     \
        "%s() -> %s", __FUNCTION__, hresult_to_string(__exit__));               \
    return __exit__;

#define LOGLEAVESELF(LOG, X, R)                                                 \
    const HRESULT __exit__ = X;                                                 \
    logger_log(LOG, SUCCEEDED(__exit__) ? LOG_LEVEL_INFO : LOG_LEVEL_ERROR,     \
        "%s() -> %s 0x%p", __FUNCTION__, hresult_to_string(__exit__), R);       \
    return __exit__;

#define REFCOUNT(X)                                                                                 \
    const ULONG __count__ = X;                                                                      \
    logger_log(self->logger, LOG_LEVEL_INFO, "0x%p->%s() -> %d", self, __FUNCTION__, __count__);    \
    return __count__;

#else
#define ENTER(M, ...)
#define LOGENTER(LOG, M, ...)

#define LEAVE(X)                    return X;
#define LOGLEAVE(LOG, X)            return X;
#define LOGLEAVESELF(LOG, X, R)     return X;

#define REFCOUNT(X)                 return X;
#endif

#define ERR(M, ...)         logger_log(self->logger, LOG_LEVEL_ERROR, "0x%p->%s()"##M##, self, __FUNCTION__, ##__VA_ARGS__)
#define ERRPARAM(P, M, ...) logger_log(self->logger, LOG_LEVEL_ERROR, "0x%p->%s("##P##")"##M##, self, __FUNCTION__, ##__VA_ARGS__)

#define LOGERR(LOG, M, ...)         logger_log(LOG, LOG_LEVEL_ERROR, "%s()"##M##, self, __FUNCTION__, ##__VA_ARGS__)
#define LOGERRPARAM(LOG, P, M, ...) logger_log(LOG, LOG_LEVEL_ERROR, "%s("##P##")"##M##, self, __FUNCTION__, ##__VA_ARGS__)
