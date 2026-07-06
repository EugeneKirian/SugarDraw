#pragma once
#include "logger.h"

const char* log_level_to_string(log_level level);
const char* hresult_to_string(HRESULT hr);

size_t next_power_of_2(size_t n);
