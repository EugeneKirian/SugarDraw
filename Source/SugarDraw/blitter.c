#include "blitter.h"
#include "plt.h"
#include "sampler.h"
#include "spanner.h"

// The best way to think of blitter is to think of it as an extensive
// rasterization step in a fixed function 3D rasterization pipeline, or,
// alternatively, as a collection of pixel shaders that work together
// to support various pixel formats.
//
// The blitter is a pipeline of it own, where the source surface (rectangle)
// can be though of as a texture, and the target surface (rectangle) is
// the space on the screen that has to be filled.
typedef struct blitter {
    allocator*          allocator;
    arena*              arena;
    CRITICAL_SECTION    lock;
} blitter;

typedef struct bltwork {
    blt* submission;
    spanner* spanner;
    sampler* sampler;
    plt* palette;
    struct {
        void* copy;
        void* fill;
        struct {
            void* source;
            void* destination;
        } read;
        struct {
            void* source;
            void* destination;
        } write;
    } funcs;
} bltwork;

HRESULT blitter_create(allocator* allocator, blitter** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    blitter* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, MEM_TAG_BLITTER, sizeof(blitter), &instance))) {
        instance->allocator = allocator;
        if (SUCCEEDED(hr = arena_create(allocator, MEM_TAG_BLITTER, &instance->arena))) {
            InitializeCriticalSection(&instance->lock);
            *object = instance;
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void blitter_release(blitter* self) {
    if (self != NULL) {
        DeleteCriticalSection(&self->lock);
        arena_release(self->arena);
        allocator_free(self->allocator, self);
    }
}

HRESULT blitter_submit(blitter* self, blt* submission) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (submission == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    EnterCriticalSection(&self->lock);

    // TODO

    LeaveCriticalSection(&self->lock);

    return hr;
}
