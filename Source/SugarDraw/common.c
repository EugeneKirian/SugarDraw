#include "common.h"

const GUID SUGARDRAW_DEVICE_GUID = { 0xDDDDDDDD, 0xDDDD, 0xDDDD, { 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD } };

const GUID CLSID_DirectDrawFactory = { 0x4FD2A832, 0x86C8, 0x11D0, { 0x8F, 0xCA, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D } };

const GUID IID_IDirectDrawFactory = { 0x4FD2A833, 0x86C8, 0x11D0, { 0x8F, 0xCA, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D } };

const GUID IID_IDirectDraw3 = { 0x618F8AD4, 0x8B7A, 0x11D0, { 0x8F, 0xCC, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0x9D } };

BOOL IsInsideRect(const RECT* bounds, const RECT* rect) {
    if (bounds == NULL || rect == NULL) {
        return FALSE;
    }

    if (rect->left < bounds->left || rect->top < bounds->top
        || rect->right > bounds->right || rect->bottom > bounds->bottom) {
        return FALSE;
    }

    return TRUE;
}

BOOL IsSameSizeRect(const RECT* rect, const RECT* value) {
    if (rect == NULL || value == NULL) {
        return FALSE;
    }

    if ((rect->bottom - rect->top) == (value->bottom - value->top)
        && (rect->right - rect->left) == (value->right - value->left)) {
        return TRUE;
    }

    return FALSE;
}

BOOL IsValidRect(const RECT* rect) {
    if (rect == NULL) {
        return FALSE;
    }

    if (rect->right <= rect->left || rect->bottom <= rect->top) {
        return FALSE;
    }

    return TRUE;
}

BOOL devmodea_equal(const DEVMODEA* mode, const DEVMODEA* value) {
    if (mode == NULL || value == NULL) {
        return FALSE;
    }

    if (mode->dmSize != sizeof(DEVMODEA) || value->dmSize != sizeof(DEVMODEA)) {
        return FALSE;
    }

    const u32 flags = DM_PELSHEIGHT | DM_PELSWIDTH | DM_BITSPERPEL;

    if ((mode->dmFields & flags) != (value->dmFields & flags)
        || mode->dmPelsWidth != value->dmPelsWidth
        || mode->dmPelsHeight != value->dmPelsHeight
        || mode->dmBitsPerPel != value->dmBitsPerPel) {
        return FALSE;
    }

    if ((mode->dmFields & DM_PELSHEIGHT) && (value->dmFields & DM_PELSHEIGHT)) {
        if (mode->dmPelsHeight != value->dmPelsHeight) {
            return FALSE;
        }
    }

    return TRUE;
}
