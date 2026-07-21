#include "blt.h"
#include "dd.h"
#include "ddg.h"
#include "dds.h"
#include "ddsd.h"
#include "driver.h"

#define FREQUENCY       60

#define WAIT_NONE       0

static DWORD WINAPI ddg_worker(ddg* self);

static HRESULT ddg_get_surface_desc(ddg* self, DDSURFACEDESC2* desc);
static HRESULT ddg_stop_worker(ddg* self);

HRESULT ddg_create(sugar* manager, driver* driver, ddg** object) {
    if (manager == NULL || driver == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    ddg* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWGRAPHICS, sizeof(ddg), &instance))) {
        instance->manager = manager;
        instance->driver = driver;
        InitializeCriticalSection(&instance->lock);

        instance->done = CreateEventA(NULL, FALSE, FALSE, NULL);
        instance->stop = CreateEventA(NULL, FALSE, FALSE, NULL);
        instance->waitable = CreateEventA(NULL, TRUE, TRUE, NULL);
        instance->worker = CreateThread(NULL, 0, ddg_worker, instance, CREATE_SUSPENDED, NULL);

        *object = instance;
    }

    return hr;
}

void ddg_release(ddg* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        ddg_stop_worker(self);

        // TODO driver - what to do?

        CloseHandle(self->done);
        CloseHandle(self->stop);
        CloseHandle(self->waitable);
        CloseHandle(self->worker);

        ddsd_release(self->surface);

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT ddg_initialize(ddg* self, dd* object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->surface != NULL) {
        return DDERR_ALREADYINITIALIZED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    ddsd* instance = NULL;
    if (SUCCEEDED(hr = ddsd_create(self->manager->allocator, &instance))) {
        DDSURFACEDESC2 desc;
        ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
        desc.dwSize = sizeof(DDSURFACEDESC2);

        if (SUCCEEDED(hr = ddg_get_surface_desc(self, &desc))) {
            if (SUCCEEDED(hr = ddsd_initialize(instance, &desc))) {
                self->instance = object;
                self->surface = instance;
                ResumeThread(self->worker);
                goto exit;
            }
        }

        ddsd_release(instance);
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddg_set_driver(ddg* self, driver* driver) {

    // TODO stop thread
    // TODO do driver thing
    // TODO start thread

    return DDERR_UNSUPPORTED;
}

HRESULT ddg_recreate_surface(ddg* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->surface == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (SUCCEEDED(hr = ddg_stop_worker(self))) {
        ddsd* instance = NULL;
        if (SUCCEEDED(hr = ddsd_create(self->manager->allocator, &instance))) {
            DDSURFACEDESC2 desc;
            ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
            desc.dwSize = sizeof(DDSURFACEDESC2);

            if (SUCCEEDED(hr = ddg_get_surface_desc(self, &desc))) {
                if (SUCCEEDED(hr = ddsd_initialize(instance, &desc))) {
                    ddsd_release(InterlockedExchangePointer(&self->surface, instance));
                    self->worker = CreateThread(NULL, 0, ddg_worker, self, 0, NULL);
                }
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddg_get_status(ddg* self, u32 flags, u32* status) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (flags & ~(DDGSTATUS_SIGNALED | DDGSTATUS_UPDATING)) {
        return DDERR_INVALIDPARAMS;
    }

    if (status == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (self->surface == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (flags & DDGSTATUS_SIGNALED) {
        *status = self->update
            ? DDGSTATUS_SIGNALED : DDGSTATUS_NONE;
    }

    if (flags & DDGSTATUS_UPDATING) {
        *status = self->updating
            ? DDGSTATUS_UPDATING : DDGSTATUS_NONE;
    }

    return DD_OK;
}

HRESULT ddg_signal_update(ddg* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->surface == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    InterlockedExchange(&self->update, TRUE);

    return DD_OK;
}

HRESULT ddg_can_update(ddg* self, bool wait) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->updating) {
        if (!wait) {
            return DDERR_WASSTILLDRAWING;
        }

        return WaitForSingleObject(self->waitable, INFINITE) == WAIT_OBJECT_0
            ? DD_OK : DDERR_GENERIC;
    }

    return DD_OK;
}

DWORD WINAPI ddg_worker(ddg* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LARGE_INTEGER counter, interval, time, now;

    QueryPerformanceFrequency(&counter);
    QueryPerformanceCounter(&time);

    interval.QuadPart = counter.QuadPart / FREQUENCY;

    while (TRUE) {
        bool sleep = TRUE;
        HRESULT hr = DD_OK;

        if (WaitForSingleObject(self->stop, WAIT_NONE) == WAIT_OBJECT_0) { break; }

        // TODO: keep track of window size changes...
        // TODO keep track of window position on the screen...

        // TODO what to do if window was moved to another screen?

        if (self->instance != NULL
            && self->instance->primary != NULL
            && self->update && !self->updating) {
            QueryPerformanceCounter(&now);

            if (now.QuadPart - time.QuadPart >= interval.QuadPart) {
                ResetEvent(self->waitable);
                EnterCriticalSection(&self->lock);
                InterlockedExchange(&self->update, FALSE);
                InterlockedExchange(&self->updating, TRUE);

                sleep = FALSE;
                time.QuadPart = now.QuadPart;

                RECT dst, src;
                dds* primary = self->instance->primary;
                ddsd* surface = primary->surface;
                if (SUCCEEDED(hr = ddsd_get_rect(self->surface, &dst))) {
                    if (SUCCEEDED(hr = ddsd_get_rect(surface, &src))) {
                        // TODO: use gamma control if present - primary surface only
                        // Use it only in exclusive full screen mode?

                        // TODO: use color control if present - primary and overlay surfaces
                        // How to aply those values? Need example!

                        // Blit the primary surface into the grahics surface.
                        if (SUCCEEDED(hr = ddsd_blt_fast(self->surface, &dst, surface, &src, DDBLTFAST_NOCOLORKEY))) {
                            // Blit all visible overlays on top of the primary surface into the graphics surface.
                            const s32 item_count = connector_get_count(primary->overlays);
                            for (s32 i = 0; i < item_count; i++) {
                                iddsconn connector;
                                ZeroMemory(&connector, sizeof(iddsconn));
                                if (SUCCEEDED(hr = connector_get_item(primary->overlays, i, &connector))) {
                                    dds* overlay = connector.instance;
                                    if (overlay->desc.ddsCaps.dwCaps & DDSCAPS_VISIBLE) {
                                        // TODO proper implementation
                                        hr = ddsd_blt_fast(self->surface, &overlay->overlay.dst, overlay->surface, &overlay->overlay.src, DDBLTFAST_NOCOLORKEY); // TODO color keys..
                                    }
                                }
                            }

                            // TODO move to the driver
                            {
                                HDC sdc = NULL;
                                if (SUCCEEDED(hr = ddsd_get_dc(self->surface, &sdc))) {
                                    HWND hwnd = self->instance->cooperation.hwnd;
                                    HDC hdc = GetDC(hwnd);
                                    RECT rect;
                                    GetClientRect(hwnd, &rect);
                                    
                                    ClientToScreen(hwnd, (POINT*)&rect);
                                    BitBlt(hdc, 0, 0, rect.right, rect.bottom, sdc, rect.left, rect.top, SRCCOPY);
                                    hr = ddsd_release_dc(self->surface, sdc);
                                }
                            }
                        }
                    }
                }

                InterlockedExchange(&self->updating, FALSE);
                LeaveCriticalSection(&self->lock);
            }
        }

        SetEvent(self->waitable);

        if (sleep) { Sleep(1); }
    }

    SetEvent(self->done);

    return EXIT_SUCCESS;
}

static HRESULT ddg_get_surface_desc(ddg* self, DDSURFACEDESC2* desc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (desc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (desc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    DEVMODEA mode;
    ZeroMemory(&mode, sizeof(DEVMODEA));
    mode.dmSize = sizeof(DEVMODEA);

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = sugar_get_display_mode(self->manager, &mode))) {
        ZeroMemory(desc, sizeof(DDSURFACEDESC2));
        desc->dwSize = sizeof(DDSURFACEDESC2);
        desc->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        desc->dwWidth = mode.dmPelsWidth;
        desc->dwHeight = mode.dmPelsHeight;
        desc->ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;

        desc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        desc->ddpfPixelFormat.dwRGBBitCount = 32;
        desc->ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
        desc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
        desc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
        desc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
    }

    return hr;
}

HRESULT ddg_stop_worker(ddg* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->worker != NULL) {
        u32 code = 0;
        ResumeThread(self->worker);
        if (GetExitCodeThread(self->worker, &code)) {
            if (code == STILL_ACTIVE) {
                SetEvent(self->stop);
                if (WaitForSingleObject(self->done, INFINITE) != WAIT_OBJECT_0) {
                    return DDERR_GENERIC;
                }
            }
        }
    }

    return DD_OK;
}

