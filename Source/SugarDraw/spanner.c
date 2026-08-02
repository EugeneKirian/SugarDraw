#include "spanner.h"

#define SPANNER_SIZE_MULTILIER  2

static s32 span_compare(const void* a, const void* b) {
    const span* sa = (const span*)a;
    const span* sb = (const span*)b;

    if (sa->y != sb->y) {
        return (sa->y > sb->y) - (sa->y < sb->y);
    }

    if (sa->x0 != sb->x0) {
        return (sa->x0 > sb->x0) - (sa->x0 < sb->x0);
    }

    return (sa->x1 > sb->x1) - (sa->x1 < sb->x1);
}

static HRESULT spanner_compact(spanner* self);
static HRESULT spanner_initialize(spanner* self,
    const RECT* rect, const RECT* dst, const RGNDATA* region);
static HRESULT spanner_insert(spanner* self, s32 y, s32 x0, s32 x1);
static HRESULT spanner_resize(spanner* self);

HRESULT spanner_create(arena* arena, const RECT* rect,
    const RECT* dst, const RGNDATA* region, spanner** object) {
    if (arena == NULL || rect == NULL || dst == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsValidRect(rect)) {
        return DDERR_INVALIDRECT;
    }

    HRESULT hr = DD_OK;
    spanner* instance = NULL;
    if (SUCCEEDED(hr = arena_allocate(arena, sizeof(spanner), (void**)&instance))) {
        instance->arena = arena;
        instance->capacity = (u32)(rect->bottom - rect->top);
        if (SUCCEEDED(hr = arena_allocate(arena, instance->capacity * sizeof(span), (void**)&instance->spans))) {
            if (SUCCEEDED(hr = spanner_initialize(instance, rect, dst, region))) {
                *object = instance;
            }
        }
    }

    return hr;
}

HRESULT spanner_initialize(spanner* self,
    const RECT* rect, const RECT* dst, const RGNDATA* region) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    MAKETYPE(RECT, active);
    if (!IntersectRect(&active, dst, rect)) {
        // There is nothing to do in case there is no intersection.
        return DD_OK;
    }

    HRESULT hr = DD_OK;
    if (region == NULL) {
        // Create destination spans based on the input rectangles overlap
        // in case when no clipping region is provided.
        for (s32 y = active.top; y < active.bottom; y++) {
            if (FAILED(hr = spanner_insert(self, y, active.left, active.right))) {
                return hr;
            }
        }

        return hr;
    }

    // Validate region header.
    if (region->rdh.dwSize != sizeof(RGNDATAHEADER)
        || region->rdh.iType != RDH_RECTANGLES) {
        return DDERR_INVALIDPARAMS;
    }

    MAKETYPE(RECT, intersect);
    // Check if there is overlap with the region boundaries.
    if (!IntersectRect(&intersect, &active, &region->rdh.rcBound)) {
        // There is nothing to do in case there is no intersection.
        return DD_OK;
    }

    const u32 count = region->rdh.nCount;
    const RECT* rectangles = (const RECT*)region->Buffer;

    for (u32 i = 0; i < count; i++) {
        if (IntersectRect(&intersect, &active, &rectangles[i])) {
            for (s32 y = intersect.top; y < intersect.bottom; y++) {
                if (FAILED(hr = spanner_insert(self, y, intersect.left, intersect.right))) {
                    return hr;
                }
            }
        }
    }

    return spanner_compact(self);
}

HRESULT spanner_insert(spanner* self, s32 y, s32 x0, s32 x1) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (x1 <= x0) {
        return DDERR_INVALIDPARAMS;
    }

    // Iterate through existing destination spans in attempt to
    // find either partial or complete overlap.
    for (u32 i = 0; i < self->count; i++) {
        span* spans = self->spans;
        if (spans[i].y == y) {
            if (x0 <= spans[i].x1) {
                spans[i].x1 = x1;
                if (x0 < spans[i].x0) {
                    spans[i].x0 = x0;
                }

                return DD_OK;
            }
        }
    }

    HRESULT hr = DD_OK;

    // Check if new span can be added...
    if (self->count + 1 == self->capacity) {
        if (FAILED(hr = spanner_resize(self))) {
            return hr;
        }
    }

    self->spans[self->count].x0 = x0;
    self->spans[self->count].x1 = x1;
    self->spans[self->count].y = y;
    self->count++;

    return hr;
}

HRESULT spanner_compact(spanner* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->count < 2) {
        return DD_OK;
    }

    qsort(self->spans, self->count, sizeof(span), span_compare);

    u32 target = 0;
    for (u32 current = 1; current < self->count; current++) {
        if (self->spans[current].y == self->spans[target].y
            && self->spans[current].x0 <= self->spans[target].x1) {
            if (self->spans[current].x1 > self->spans[target].x1) {
                self->spans[target].x1 = self->spans[current].x1;
            }
        }
        else {
            target++;
            if (target != current) {
                CopyMemory(&self->spans[target], &self->spans[current], sizeof(span));
            }
        }
    }

    self->count = target + 1;

    return DD_OK;
}

HRESULT spanner_resize(spanner* self) {
    if (self == NULL) {
        return E_POINTER;
    }

    HRESULT hr = DD_OK;
    span* spans = NULL;
    const u32 capacity = self->capacity * SPANNER_SIZE_MULTILIER;
    if (SUCCEEDED(hr = arena_allocate(self->arena, capacity * sizeof(span), (void**)&spans))) {
        CopyMemory(spans, self->spans, self->count * sizeof(span));

        self->spans = spans;
        self->capacity = capacity;
    }

    return hr;
}
