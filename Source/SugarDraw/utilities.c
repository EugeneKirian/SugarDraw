#include "utilities.h"

#include <stdio.h>

const char* log_level_to_string(log_level level) {
    switch (level) {
    case LOG_LEVEL_ERROR: { return "ERROR"; }break;
    case LOG_LEVEL_WARNING: { return "WARNING"; }break;
    case LOG_LEVEL_INFO: { return "INFO"; }break;
    case LOG_LEVEL_TRACE: { return "TRACE"; }break;
    }

    return "None";
}

size_t next_power_of_2(size_t n) {
    if (n == 0) {
        return 1;
    }

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

#if _WIN64
    n |= n >> 32;
#endif

    return (n + 1);
}

const char* hresult_to_string(HRESULT hr) {
    return "TODO"; // TODO
}
