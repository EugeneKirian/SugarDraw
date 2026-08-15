#include "timer.h"
#include "waiter.h"

#define INSTANT         0
#define WAIT_NONE       0

typedef HANDLE(WINAPI CREATEWAITABLETIMER)(LPSECURITY_ATTRIBUTES lpTimerAttributes,
    BOOL bManualReset, LPCSTR lpTimerName);
typedef HANDLE(WINAPI CREATEWAITABLETIMEREX)(LPSECURITY_ATTRIBUTES lpTimerAttributes,
    LPCSTR lpTimerName, DWORD dwFlags, DWORD dwDesiredAccess);

typedef BOOL(WINAPI CANCELWAITABLETIMER)(HANDLE hTimer);

typedef BOOL(WINAPI SETWAITABLETIMER)(HANDLE hTimer,
    const LARGE_INTEGER* lpDueTime, LONG lPeriod, PTIMERAPCROUTINE pfnCompletionRoutine,
    LPVOID lpArgToCompletionRoutine, BOOL fResume);
typedef BOOL(WINAPI SETWAITABLETIMEREX)(HANDLE hTimer,
    const LARGE_INTEGER* lpDueTime, LONG lPeriod, PTIMERAPCROUTINE pfnCompletionRoutine,
    LPVOID lpArgToCompletionRoutine, PREASON_CONTEXT WakeContext, ULONG TolerableDelay);

typedef struct timer {
    allocator*                  allocator;
    CRITICAL_SECTION            lock;
    s64                         period;
    HANDLE                      tick, timer;
    HANDLE                      start, stop, exit;
    HANDLE                      worker;
    struct {
        CREATEWAITABLETIMER*    create_waitable_timer;
        CREATEWAITABLETIMEREX*  create_waitable_timer_ex;
        CANCELWAITABLETIMER*    cancel_waitable_timer;
        SETWAITABLETIMER*       set_waitable_timer;
        SETWAITABLETIMEREX*     set_waitable_timer_ex;
    } functions;
    waiter*                     waiter;
} timer;

static DWORD WINAPI timer_worker(timer* self);
static DWORD WINAPI timer_worker_timer(timer* self);
static DWORD WINAPI timer_worker_waitable(timer* self);

static HRESULT timer_initialize(timer* self);

HRESULT timer_create(allocator* allocator, timer** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    timer* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_TIMER, sizeof(timer), &instance))) {
        instance->allocator = allocator;
        if (SUCCEEDED(hr = waiter_create(allocator, MEM_TAG_TIMER, &instance->waiter))) {
            if (SUCCEEDED(hr = timer_initialize(instance))) {
                InitializeCriticalSection(&instance->lock);
                *object = instance;
                return hr;
            }

            waiter_release(instance->waiter);
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void timer_release(timer* self) {
    if (self != NULL) {
        EnterCriticalSection(&self->lock);

        timer_stop(self);

        CloseHandle(self->start);
        CloseHandle(self->stop);
        CloseHandle(self->exit);

        if (self->waiter != NULL) {
            waiter_release(self->waiter);
        }

        LeaveCriticalSection(&self->lock);
        DeleteCriticalSection(&self->lock);

        allocator_free(self->allocator, self);
    }
}

HRESULT timer_start(timer* self, s64 period) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (period <= 0) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (WaitForSingleObject(self->start, 0) == WAIT_OBJECT_0) {
        EXITCODE(DDERR_GENERIC);
    }

    self->period = period;

    if (self->functions.create_waitable_timer_ex != NULL
        && self->functions.set_waitable_timer_ex != NULL) {
        self->timer = self->functions.create_waitable_timer_ex(NULL, NULL,
            CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    }

    if (self->timer == NULL
        && self->functions.create_waitable_timer != NULL
        && self->functions.set_waitable_timer != NULL) {
        self->timer = self->functions.create_waitable_timer(NULL, FALSE, NULL);
    }

    if (self->timer == NULL) {
        self->tick = CreateEventA(NULL, FALSE, FALSE, NULL);
    }

    ResetEvent(self->start);
    ResetEvent(self->stop);
    ResetEvent(self->exit);

    self->worker = CreateThread(NULL, 0, timer_worker, self, INSTANT, NULL);

    SetEvent(self->start);

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT timer_stop(timer* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    if (self->worker) {
        u32 code = 0;
        SetEvent(self->start);
        if (GetExitCodeThread(self->worker, &code)) {
            if (code == STILL_ACTIVE) {
                SetEvent(self->stop);
                if (WaitForSingleObject(self->exit, INFINITE) != WAIT_OBJECT_0) {
                    EXITCODE(DDERR_GENERIC);
                }

                CloseHandle(self->worker);
                self->worker = NULL;
            }
        }
    }

    if (self->tick != NULL) {
        CloseHandle(self->tick);
        self->tick = NULL;
    }

    if (self->timer != NULL) {
        self->functions.cancel_waitable_timer(self->timer);
        CloseHandle(self->timer);
        self->timer = NULL;
    }

exit:
    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT timer_register_event(timer* self, HANDLE event) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (event == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    hr = waiter_add(self->waiter, event);

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT timer_unregister_event(timer* self, HANDLE event) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (event == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    hr = waiter_remove(self->waiter, event);

    LeaveCriticalSection(&self->lock);

    return hr;
}

HRESULT timer_initialize(timer* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    if (kernel == NULL) {
        return DDERR_GENERIC;
    }

    self->functions.create_waitable_timer_ex =
        (CREATEWAITABLETIMEREX*)GetProcAddress(kernel, "CreateWaitableTimerExA");
    self->functions.create_waitable_timer =
        (CREATEWAITABLETIMER*)GetProcAddress(kernel, "CreateWaitableTimerA");

    self->functions.cancel_waitable_timer =
        (CANCELWAITABLETIMER*)GetProcAddress(kernel, "CancelWaitableTimer");

    self->functions.set_waitable_timer_ex =
        (SETWAITABLETIMEREX*)GetProcAddress(kernel, "SetWaitableTimerEx");
    self->functions.set_waitable_timer =
        (SETWAITABLETIMER*)GetProcAddress(kernel, "SetWaitableTimer");

    self->start = CreateEventA(NULL, TRUE, FALSE, NULL);
    self->stop = CreateEventA(NULL, FALSE, FALSE, NULL);
    self->exit = CreateEventA(NULL, FALSE, FALSE, NULL);

    return DD_OK;
}

DWORD WINAPI timer_worker(timer* self) {
    if (self == NULL) {
        return EXIT_FAILURE;
    }

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    if (WaitForSingleObject(self->start, INFINITE) != WAIT_OBJECT_0) {
        return EXIT_FAILURE;
    }

    return self->timer == NULL
        ? timer_worker_waitable(self) : timer_worker_timer(self);
}

DWORD WINAPI timer_worker_timer(timer* self) {
    if (self == NULL) {
        return EXIT_FAILURE;
    }

    LARGE_INTEGER counter, time, now, due;
    HANDLE waitables[2] = { self->stop, self->timer };

    QueryPerformanceFrequency(&counter);
    QueryPerformanceCounter(&time);

    const s64 interval =
        (self->period * counter.QuadPart) / 10000000LL;

    s64 target = time.QuadPart + interval;

    while (TRUE) {
        if (WaitForSingleObject(self->stop, WAIT_NONE) == WAIT_OBJECT_0) { break; }

        QueryPerformanceCounter(&now);

        bool is_set = FALSE;
        const s64 remaining = target - now.QuadPart;

        if (remaining > 0) {
            due.QuadPart = -((remaining * 10000000LL) / counter.QuadPart);

            if (self->functions.set_waitable_timer_ex != NULL) {
                is_set = self->functions.set_waitable_timer_ex(
                    self->timer,
                    &due,
                    0,          // Period (0 = manual re-arm)
                    NULL,       // Completion routine
                    NULL,       // Arg to completion routine
                    NULL,       // WakeContext (NULL if no explicit power reason needed)
                    0           // TolerableDelay in ms (0 = strict timing)
                );
            }

            if (!is_set) {
                is_set = self->functions.set_waitable_timer(self->timer, &due, 0, NULL, NULL, FALSE);
            }
        }

        if (is_set) {
            if (WaitForMultipleObjects(2, waitables, FALSE, INFINITE) == WAIT_OBJECT_0) {
                goto exit;
            }

            waiter_set(self->waiter);
        }

        target += interval;

        // Prevention against permanent lag accumulation.
        QueryPerformanceCounter(&now);
        if (target < now.QuadPart) {
            target = now.QuadPart + interval;
        }
    }

exit:
    SetEvent(self->exit);

    return EXIT_SUCCESS;
}

DWORD WINAPI timer_worker_waitable(timer* self) {
    if (self == NULL) {
        return EXIT_FAILURE;
    }

    LARGE_INTEGER counter, time, now;
    QueryPerformanceFrequency(&counter);
    QueryPerformanceCounter(&time);

    const s64 interval =
        (self->period * counter.QuadPart) / 10000000LL;

    s64 target = time.QuadPart + interval;

    while (TRUE) {
        if (WaitForSingleObject(self->stop, WAIT_NONE) == WAIT_OBJECT_0) { break; }

        QueryPerformanceCounter(&now);

        const s64 remaining = target - now.QuadPart;
        const u32 timeout = remaining > 0
            ? (u32)((remaining * 1000LL) / counter.QuadPart) : 0;

        if (WaitForSingleObject(self->tick, timeout) == WAIT_TIMEOUT) {
            waiter_set(self->waiter);
        }

        target += interval;

        // Prevention against permanent lag accumulation.
        QueryPerformanceCounter(&now);
        if (target < now.QuadPart) {
            target = now.QuadPart + interval;
        }
    }

    SetEvent(self->exit);

    return EXIT_SUCCESS;
}
