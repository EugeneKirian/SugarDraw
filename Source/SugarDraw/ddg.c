#include "dd.h"
#include "ddc.h"
#include "ddg.h"
#include "dds.h"
#include "ddsd.h"
#include "driver.h"

#define FREQUENCY       30 /* TODO Settings */

#define INSTANT         0
#define WAIT_NONE       0

static DWORD WINAPI ddg_worker(ddg* self);

static HRESULT ddg_get_surface_desc(ddg* self, DDSURFACEDESC2* desc);
static HRESULT ddg_stop_worker(ddg* self);

HRESULT ddg_create(sugar* manager, blitter* blitter, driver* driver, ddg** object) {
    if (manager == NULL || blitter == NULL || driver == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    ddg* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_DIRECTDRAWGRAPHICS, sizeof(ddg), &instance))) {
        instance->manager = manager;
        instance->blitter = blitter;
        instance->driver = driver;

        instance->desc.dwSize = sizeof(DDSURFACEDESC2);
        if (SUCCEEDED(hr = ddg_get_surface_desc(instance, &instance->desc))) {
            InitializeCriticalSection(&instance->lock);

            instance->start = CreateEventA(NULL, TRUE, FALSE, NULL);
            instance->stop = CreateEventA(NULL, FALSE, FALSE, NULL);
            instance->exit = CreateEventA(NULL, FALSE, FALSE, NULL);
            instance->ready = CreateEventA(NULL, TRUE, TRUE, NULL);
            instance->updating = CreateEventA(NULL, TRUE, TRUE, NULL);
            instance->worker = CreateThread(NULL, 0, ddg_worker, instance, INSTANT, NULL);

            *object = instance;
            return hr;
        }

        allocator_free(manager->allocator, instance);
    }

    return hr;
}

void ddg_release(ddg* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        ddg_stop_worker(self);

        // TODO driver - what to do?

        CloseHandle(self->start);
        CloseHandle(self->stop);
        CloseHandle(self->exit);
        CloseHandle(self->ready);
        CloseHandle(self->updating);
        CloseHandle(self->worker);

        ddsd_release(self->surface);

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        allocator_free(self->manager->allocator, self);
    }
}

HRESULT ddg_get_status(ddg* self, u32* status) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->surface == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    if (status == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    *status = self->status;

    return DD_OK;
}

HRESULT ddg_is_ready(ddg* self, bool wait) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->status & DDGSTATUS_UPDATING) {
        if (wait) {
            return WaitForSingleObject(self->ready, INFINITE) == WAIT_OBJECT_0
                ? DD_OK : DDERR_GENERIC;
        }

        return DDERR_WASSTILLDRAWING;
    }

    return DD_OK;
}

HRESULT ddg_is_updating(ddg* self, bool wait) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (!(self->status & DDGSTATUS_UPDATING)) {
        if (wait) {
            return WaitForSingleObject(self->updating, INFINITE) == WAIT_OBJECT_0
                ? DD_OK : DDERR_GENERIC;
        }

        return DDERR_SURFACEBUSY;
    }

    return DD_OK;
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
    if (SUCCEEDED(hr = ddsd_create(self->manager->allocator, self->blitter, &instance))) {
        if (SUCCEEDED(hr = ddsd_initialize(instance, &self->desc))) {
            self->instance = object;
            self->surface = instance;
            SetEvent(self->start);
            goto exit;
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

    MAKEDEVMODEA(mode);
    if (SUCCEEDED(hr = sugar_get_display_mode(self->manager, &mode))) {
        if (mode.dmPelsWidth > self->desc.dwWidth || mode.dmPelsHeight > self->desc.dwHeight) {
            if (SUCCEEDED(hr = ddg_stop_worker(self))) {
                ddsd* instance = NULL;
                if (SUCCEEDED(hr = ddsd_create(self->manager->allocator, self->blitter, &instance))) {
                    MAKEDDSURFACEDESC2(desc);
                    if (SUCCEEDED(hr = ddg_get_surface_desc(self, &desc))) {
                        if (SUCCEEDED(hr = ddsd_initialize(instance, &desc))) {
                            ddsd_release(InterlockedExchangePointer(&self->surface, instance));
                            self->worker = CreateThread(NULL, 0, ddg_worker, self, INSTANT, NULL);
                        }
                    }
                }
            }
        }
    }

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT ddg_signal_update(ddg* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->surface == NULL) {
        return DDERR_NOTINITIALIZED;
    }

    self->status |= DDGSTATUS_SIGNALED;

    return DD_OK;
}

DWORD WINAPI ddg_worker(ddg* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    MAKETYPE(RECT, window);
    LARGE_INTEGER counter, interval, time, now;

    QueryPerformanceFrequency(&counter);
    QueryPerformanceCounter(&time);

    interval.QuadPart = counter.QuadPart / FREQUENCY;

    if (WaitForSingleObject(self->start, INFINITE) != WAIT_OBJECT_0) { goto exit; }

    while (TRUE) {
        bool sleep = TRUE;
        HRESULT hr = DD_OK;

        if (WaitForSingleObject(self->stop, WAIT_NONE) == WAIT_OBJECT_0) { break; }

        HWND hwnd = self->instance->cooperation.hwnd;
        if (!IsIconic(hwnd)) {
            // TODO handle lost primary surface and overlays

            if (self->instance != NULL
                && self->instance->primary != NULL
                && (self->status & DDGSTATUS_SIGNALED)) {
                QueryPerformanceCounter(&now);

                if (now.QuadPart - time.QuadPart >= interval.QuadPart) {
                    ResetEvent(self->ready);
                    SetEvent(self->updating);
                    EnterCriticalSection(&self->lock);
                    self->status = DDGSTATUS_UPDATING;

                    sleep = FALSE;
                    time.QuadPart = now.QuadPart;

                    RECT client, rectangle;
                    GetClientRect(hwnd, &client);
                    CopyMemory(&rectangle, &client, sizeof(RECT));
                    ClientToScreen(hwnd, (POINT*)&rectangle.left);
                    ClientToScreen(hwnd, (POINT*)&rectangle.right);

                    dds* primary = self->instance->primary;
                    ddsd* surface = primary->surface;

                    // TODO: it seems like DirectDraw keeps track of areas where user put pixels
                    // into the primary surface after resizing/moving window
                    // so that only those are actually being drawn, and not the whole available surface
                    // stretch.exe

                    // TODO is this needed in case of tracking rimary surface + back buffers blit/dc/lock regions?
                    // TODO make this configurable, same as with primary surface creation
                    if (!CompareMemory(&window, &rectangle, sizeof(RECT))) {
                        MAKETYPE(RECT, rect);
                        CopyMemory(&window, &rectangle, sizeof(RECT));
                        if (SUCCEEDED(hr = ddsd_get_rect(self->surface, &rect))) {
                            HDC sdc = NULL;
                            if (SUCCEEDED(hr = ddsd_get_dc(self->surface, &sdc))) {
                                FillRect(sdc, &rect, (HBRUSH)GetClassLongPtrA(hwnd, GCLP_HBRBACKGROUND));
                                hr = ddsd_release_dc(self->surface, sdc);
                            }
                        }
                    }

                    RECT dst, src;
                    if (SUCCEEDED(hr = ddsd_get_rect(self->surface, &dst))) {
                        if (IntersectRect(&dst, &dst, &rectangle)) {
                            // TODO what to do if window was moved to another screen?
                            if (SUCCEEDED(hr = ddsd_get_rect(surface, &src))) {
                                if (IntersectRect(&src, &src, &rectangle))
                                {
                                    // TODO what to do if window was moved to another screen?

                                    // TODO: use gamma control if present - primary surface only
                                    // Use it only in exclusive full screen mode?

                                    // TODO: use color control if present - primary and overlay surfaces
                                    // How to apply the values? Need example!

                                    RGNDATA* region = NULL;
                                    if (primary->clipper.instance != NULL) {
                                        ddc_get_region(primary->clipper.instance, &region);
                                    }

                                    // Blit the primary surface into the grahics surface.
                                    if (SUCCEEDED(hr = ddsd_blt(self->surface, &dst, surface, &src, region, DDBLT_WAIT, NULL))) {
                                        // Blit all visible overlays on top of the primary surface into the graphics surface.
                                        const u32 item_count = connector_get_count(primary->overlays);
                                        for (u32 i = 0; i < item_count; i++) {
                                            MAKETYPE(iddsconn, connector);
                                            if (SUCCEEDED(hr = connector_get_item(primary->overlays, i, &connector))) {
                                                dds* overlay = connector.instance;
                                                if (overlay->desc.ddsCaps.dwCaps & DDSCAPS_VISIBLE) {

                                                    // TODO
                                                    // The problem with overlay keys here is that the target surface is always 32-bit
                                                    // while rimary surface can be non-32 bit, and the color key is in the primary surface
                                                    // color space. Therefore we need a stencil here...
                                                    // ddoverlay.exe

                                                    u32 flags = DDBLT_WAIT;
                                                    MAKEDDBLTFX(effects);
                                                    if (overlay->overlay.flags & DDOVER_KEYSRC) {
                                                        flags |= DDBLT_DDFX | DDBLT_KEYSRCOVERRIDE;
                                                        CopyMemory(&effects.ddckSrcColorkey,
                                                            &overlay->desc.ddckCKSrcOverlay, sizeof(DDCOLORKEY));
                                                    }

                                                    if (overlay->overlay.flags & DDOVER_KEYDEST) {
                                                        flags |= DDBLT_DDFX | DDBLT_KEYDESTOVERRIDE;
                                                        CopyMemory(&effects.ddckDestColorkey,
                                                            &overlay->desc.ddckCKDestOverlay, sizeof(DDCOLORKEY));
                                                    }

                                                    if (overlay->overlay.flags & DDOVER_KEYSRCOVERRIDE) {
                                                        flags |= DDBLT_DDFX | DDBLT_KEYSRCOVERRIDE;
                                                        CopyMemory(&effects.ddckSrcColorkey,
                                                            &overlay->overlay.effects.dckSrcColorkey, sizeof(DDCOLORKEY));
                                                    }

                                                    if (overlay->overlay.flags & DDOVER_KEYDESTOVERRIDE) {
                                                        flags |= DDBLT_DDFX | DDBLT_KEYDESTOVERRIDE;
                                                        CopyMemory(&effects.ddckDestColorkey,
                                                            &overlay->overlay.effects.dckDestColorkey, sizeof(DDCOLORKEY));
                                                    }

                                                    // Applications might call update overlay and update
                                                    // destination and source rectangles a great number of times.
                                                    // Make sure to preserve the rectangles to avoid them being changed via updates,
                                                    // which could lead to permanent locking of surfaces on window resize, etc.
                                                    MAKETYPE(RECT, target);
                                                    MAKETYPE(RECT, source);
                                                    CopyMemory(&target, &overlay->overlay.dst, sizeof(RECT));
                                                    CopyMemory(&source, &overlay->overlay.src, sizeof(RECT));
                                                    ddsd_blt(self->surface, &target, overlay->surface, &source, region, flags, &effects);
                                                }
                                            }
                                        }

                                        // TODO move to the driver
                                        {
                                            HDC sdc = NULL;
                                            if (SUCCEEDED(hr = ddsd_get_dc(self->surface, &sdc))) {
                                                HDC hdc = GetDC(hwnd);
                                                BitBlt(hdc, 0, 0, client.right, client.bottom, sdc, rectangle.left, rectangle.top, SRCCOPY);
                                                ReleaseDC(hwnd, hdc);
                                                if (SUCCEEDED(hr = ddsd_release_dc(self->surface, sdc))) {
                                                    // TODO move to settings (as optional thing)
                                                    InvalidateRect(hwnd, NULL, FALSE); // Send WM_PAINT message.
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    self->status &= ~DDGSTATUS_UPDATING;
                    LeaveCriticalSection(&self->lock);
                    ResetEvent(self->updating);
                }
            }

            SetEvent(self->ready);
        }

        if (sleep) { Sleep(1); }
    }

exit:
    SetEvent(self->exit);

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

    HRESULT hr = DD_OK;
    MAKEDEVMODEA(mode);
    if (SUCCEEDED(hr = sugar_get_display_mode(self->manager, &mode))) {
        ZeroMemory(desc, sizeof(DDSURFACEDESC2));
        desc->dwSize = sizeof(DDSURFACEDESC2);
        desc->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        desc->dwWidth = mode.dmPelsWidth;
        desc->dwHeight = mode.dmPelsHeight;
        desc->ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;

        desc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        desc->ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        desc->ddpfPixelFormat.dwRGBBitCount = 32;
        desc->ddpfPixelFormat.dwRBitMask = 0x00FF0000;
        desc->ddpfPixelFormat.dwGBitMask = 0x0000FF00;
        desc->ddpfPixelFormat.dwBBitMask = 0x000000FF;
        desc->ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
    }

    return hr;
}

HRESULT ddg_stop_worker(ddg* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->worker != NULL) {
        u32 code = 0;
        SetEvent(self->start);
        if (GetExitCodeThread(self->worker, &code)) {
            if (code == STILL_ACTIVE) {
                SetEvent(self->stop);
                if (WaitForSingleObject(self->exit, INFINITE) != WAIT_OBJECT_0) {
                    return DDERR_GENERIC;
                }
            }
        }
    }

    return DD_OK;
}
