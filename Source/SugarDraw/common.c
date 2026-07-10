#include "common.h"

const GUID SUGARDRAW_DEVICE_GUID = { 0xDDDDDDDD, 0xDDDD, 0xDDDD, { 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD } };

BOOL IsValidRect(const RECT* rect) {
    if (rect == NULL) {
        return FALSE;
    }

    if (rect->right < rect->left || rect->bottom < rect->top) {
        return FALSE;
    }

    return TRUE;
}
