#pragma once

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define NOMINMAX
#include <windows.h>

#define DIRECTDRAW_VERSION 0x0700
#include <ddraw.h>
#include <dvp.h>

#define SUGARCALL __stdcall

#define DDENUM_NONE								0x00000000L
#define DDENUM_VALID							(DDENUM_ATTACHEDSECONDARYDEVICES \
                                                    | DDENUM_DETACHEDSECONDARYDEVICES | DDENUM_NONDISPLAYDEVICES)

#define DDGDI_NONE                              0x00000000L

typedef int bool;

typedef int s32;
typedef unsigned int u32;

typedef float f32;
typedef double f64;

typedef char s8;
typedef unsigned char u8;

typedef enum log_level {
    LOG_LEVEL_ERROR         = 0,
    LOG_LEVEL_WARNING       = 1,
    LOG_LEVEL_INFO          = 2,
    LOG_LEVEL_TRACE         = 3,
    LOG_LEVEL_COUNT         = 4,
    LOG_LEVEL_FORCE_DWORD   = 0x7FFFFFFF
} log_level;

typedef enum memory_tag {
    MEM_TAG_NONE        = 0,
    MEM_TAG_SUGAR       = 1,
    MEM_TAG_DIRECTDRAW  = 2,
    MEM_TAG_IDIRECTDRAW = 3,
    MEM_TAG_COUNT       = 4,
    MEM_TAG_FORCE_DWORD = 0x7FFFFFFF
} memory_tag;

const extern GUID SUGARDRAW_DEVICE_GUID;

#define SUGARDRAW_DEVICE_FILE		            "sugar.dll"
#define SUGARDRAW_DEVICE_FILE_LENGTH            32

#define SUGARDRAW_DEVICE_NAME		            "SugarDraw Device"
#define SUGARDRAW_DEVICE_NAME_LENGTH            32

#define SUGARDRAW_DEVICE_DESCRIPTION            "SugarDraw Device Driver"
#define SUGARDRAW_DEVICE_DESCRIPTION_LENGTH     32

const char* log_level_to_string(log_level level);
const char* hresult_to_string(HRESULT hr);

size_t next_power_of_2(size_t n);
