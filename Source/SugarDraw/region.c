#include "region.h"

#define DEFAULT_CAPACITY            8
#define DEFAULT_CAPACITY_MULTIPLIER 2

static HRESULT region_compact(region* self);
static HRESULT region_capacity(region* self, u32 count);
static HRESULT region_resize(region* self);

HRESULT region_create(allocator* allocator, memory_tag tag, region** object) {
    if (allocator == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    region* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(allocator, tag, sizeof(region), &instance))) {
        instance->allocator = allocator;
        instance->capacity = DEFAULT_CAPACITY;

        const u32 size = sizeof(RGNDATAHEADER) + instance->capacity * sizeof(RECT);
        if (SUCCEEDED(hr = allocator_allocate(allocator, tag, size, &instance->region))) {
            RGNDATAHEADER* header = &instance->region->rdh;
            header->dwSize = sizeof(RGNDATAHEADER);
            header->iType = RDH_RECTANGLES;

            *object = instance;
            return hr;
        }

        allocator_free(allocator, instance);
    }

    return hr;
}

void region_release(region* self) {
    if (self != NULL) {
        if (self->region != NULL) {
            allocator_free(self->allocator, self->region);
        }

        allocator_free(self->allocator, self);
    }
}

HRESULT region_clear(region* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    self->region->rdh.nCount = 0;
    self->region->rdh.nRgnSize = 0;
    ZeroMemory(&self->region->rdh.rcBound, sizeof(RECT));

    return DD_OK;
}

HRESULT region_add_rect(region* self, const RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (rect == NULL) {
        return DD_OK;
    }

    if (!IsValidRect(rect)) {
        return DDERR_INVALIDRECT;
    }

    HRESULT hr = DD_OK;
    if (self->capacity <= self->region->rdh.nCount + 1) {
        if (FAILED(hr = region_resize(self))) {
            return hr;
        }
    }

    RECT* dst = (RECT*)(self->region->Buffer + self->region->rdh.nCount * sizeof(RECT));
    CopyMemory(dst, rect, sizeof(RECT));

    self->region->rdh.nCount++;
    self->region->rdh.nRgnSize = self->region->rdh.nCount * sizeof(RECT);

    return region_compact(self);
}

HRESULT region_add_region(region* self, const RGNDATA* region) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (region == NULL) {
        return DD_OK;
    }

    if (region->rdh.dwSize != sizeof(RGNDATAHEADER)
        || region->rdh.iType != RDH_RECTANGLES) {
        return DDERR_INVALIDPARAMS;
    }

    if (region->rdh.nCount == 0) {
        return DD_OK;
    }

    HRESULT hr = DD_OK;
    const u32 count = self->region->rdh.nCount + region->rdh.nCount;
    if (self->capacity < count) {
        if (FAILED(hr = region_capacity(self, count))) {
            return hr;
        }
    }

    RECT* dst = (RECT*)(self->region->Buffer + self->region->rdh.nCount * sizeof(RECT));
    CopyMemory(dst, region->Buffer, region->rdh.nCount * sizeof(RECT));

    self->region->rdh.nCount = count;
    self->region->rdh.nRgnSize = count * sizeof(RECT);

    return region_compact(self);
}

HRESULT region_set_region(region* self, const RGNDATA* region) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (region == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (region->rdh.dwSize != sizeof(RGNDATAHEADER)
        || region->rdh.iType != RDH_RECTANGLES) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    if (self->capacity < region->rdh.nCount) {
        if (FAILED(hr = region_capacity(self, region->rdh.nCount))) {
            return hr;
        }
    }

    self->region->rdh.nCount = region->rdh.nCount;
    self->region->rdh.nRgnSize = self->region->rdh.nCount * sizeof(RECT);
    CopyMemory(self->region->Buffer, region->Buffer, self->region->rdh.nRgnSize);

    return region_compact(self);
}

// Performs a 2D geometric intersection between the existing
// rectangles of the region and a single clipping rectangle.
//
// 1. OVERLAP MATRIX (Clamping Coordinates):
//
//                      rect.top
//            rect.left +-------------------+ 
//                      |                   |
//  rects[i] -> +-------+-------------------+---+
//              |       |                   |   |
//              |       |                   |   |
//              |   +---+---------------+   |   |
//              |   |   | Intersection  |   |   |
//              |   |   |               |   |   |
//              |   +---+---------------+   |   |
//              |       |                   |   |
//              +-------+-------------------+---+
//                      |                   |
//                      +-------------------+ rect.right
//                                rect.bottom
//    
// 2. CLAMPING MATH:
//      New Left = max(rect.left, rects[i].left)
//      New Top = max(rect.top, rects[i].top)
//      New Right = min(rect.right, rects[i].right)
//      New Bottom = min(rect.bottom, rects[i].bottom)
HRESULT region_clip_rect(region* self, const RECT* rect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // Skip clipping in case no clipping rectangle is provided.
    if (rect == NULL) {
        return DD_OK;
    }

    if (!IsValidRect(rect)) {
        return DDERR_INVALIDRECT;
    }

    // The rect is exactly matching or covering the entire region.
    const RECT* bounds = &self->region->rdh.rcBound;
    if (rect->left <= bounds->left && rect->top <= bounds->top
        && rect->right >= bounds->right && rect->bottom >= bounds->bottom) {
        return DD_OK;
    }

    const u32 count = self->region->rdh.nCount;
    RECT* rects = (RECT*)self->region->Buffer;
    for (u32 i = 0; i < count; i++) {
        rects[i].left = max(rect->left, rects[i].left);
        rects[i].top = max(rect->top, rects[i].top);
        rects[i].right = min(rect->right, rects[i].right);
        rects[i].bottom = min(rect->bottom, rects[i].bottom);
    }

    return region_compact(self);
}

// Performs a 2D geometric intersection between two complex regions,
// each consisting of an array of non - overlapping rectangles.
//
// 1. MULTI - RECTANGLE INTERSECTION (N x M Cross-Product):
//
//   Region A (region: 2 rects)             Region B (clip: 2 rects)
//   +-------------------+                  +---------------+
//   | A1                |                  | B1            |
//   +-------------------+                  +---------------+
//   | A2                |                  | B2            |
//   +-------------------+                  +---------------+
//
// 2. FRAGMENTATION EXAMPLE (1 Rect clipped by multi-stripe region):
//
//   Region A (1 Rect)              Region B (2 Rects)          Intersection (A ∩ B)
//   +--------------------+         +--------------------+      +--------------------+
//   |                    |         | B1                 |      | Result 1 (A ∩ B1)  |
//   |                    |         +--------------------+      +--------------------+
//   |                    |
//   |                    |         +--------------------+      +--------------------+
//   |                    |         | B2                 |      | Result 2 (A ∩ B2)  |
//   +--------------------+         +--------------------+      +--------------------+
//
// 3. IN-BUFFER WORKSPACE LAYOUT:
//
//      self->region->Buffer:
//      +---------------------+------------------------------------+
//      | Source Rects (0..N) | Intersection Output (N..N + N * M) |
//      +---------------------+------------------------------------+
//          (Read Source)               (Write Intersections)
HRESULT region_clip_region(region* self, const RGNDATA* region) {
    if (self == NULL || self->region == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (region == NULL || self->region->rdh.nCount == 0) {
        return DD_OK;
    }

    if (region->rdh.dwSize != sizeof(RGNDATAHEADER)
        || region->rdh.iType != RDH_RECTANGLES) {
        return DDERR_INVALIDPARAMS;
    }

    // Clipping against an empty region yields an empty region.
    if (region->rdh.nCount == 0) {
        return region_clear(self);
    }

    HRESULT hr = DD_OK;
    const u32 src_count = self->region->rdh.nCount;
    const u32 clip_count = region->rdh.nCount;

    // Ensure capacity for both original source rects and generated intersections.
    if (SUCCEEDED(hr = region_capacity(self, src_count + src_count * clip_count))) {
        const RECT* src_rects = (RECT*)self->region->Buffer;
        const RECT* clip_rects = (RECT*)region->Buffer;

        // Intersect every existing rectangle against every clipping rectangle.
        // Write outputs right after source rects in the region buffer.
        u32 result_count = 0;
        RECT* result_rects = (RECT*)(self->region->Buffer + src_count * sizeof(RECT));
        for (u32 i = 0; i < src_count; i++) {
            for (u32 j = 0; j < clip_count; j++) {
                const s32 l = max(src_rects[i].left, clip_rects[j].left);
                const s32 t = max(src_rects[i].top, clip_rects[j].top);
                const s32 r = min(src_rects[i].right, clip_rects[j].right);
                const s32 b = min(src_rects[i].bottom, clip_rects[j].bottom);

                if (r > l && b > t) {
                    result_rects[result_count].left = l;
                    result_rects[result_count].top = t;
                    result_rects[result_count].right = r;
                    result_rects[result_count].bottom = b;
                    result_count++;
                }
            }
        }

        // Move intersection results to the beginning of the region buffer.
        if (result_count > 0) {
            MoveMemory(self->region->Buffer, result_rects, result_count * sizeof(RECT));
        }

        self->region->rdh.nCount = result_count;
        self->region->rdh.nRgnSize = result_count * sizeof(RECT);

        hr = region_compact(self);
    }

    return hr;
}

// Cleans up the region's rectangle array and updates bounding box.
//
// 1. RECTANGLE PRUNING (Filtering Zero / Negative Area Rects):
//
//  Before Compaction : [Valid R1] [Degenerate R2] [Valid R3]
//                                      |
//                       (bottom <= top OR right <= left)
//                                      |
//                                      v (Discard & Shift)
//  After Compaction : [Valid R1] [Valid R3]
//
// 2. BOUNDING BOX RECALCULATION:
//
//   bounds.left = min(...)
//   +----------------------------------------------+
//   | bounds.top = min(R1.top, R3.top)             |
//   |                                              |
//   | +------------+                               |
//   | |  Rect 1    |                               |
//   | +------------+               +-------------+ |
//   |                              |   Rect 3    | |
//   |                              +-------------+ |
//   |                                              |
//   | bounds.bottom = max(R1.bottom, R3.bottom)    |
//   +----------------------------------------------+
//                          bounds.right = max(...)
HRESULT region_compact(region* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (self->region->rdh.nCount == 0) {
        return DD_OK;
    }

    const u32 count = self->region->rdh.nCount;
    RECT* rects = (RECT*)self->region->Buffer;

    // Filter out zero and inverted area rectangles in-place.
    u32 compact = 0;
    for (u32 i = 0; i < count; i++) {
        if (rects[i].bottom > rects[i].top && rects[i].right > rects[i].left) {
            if (compact != i) {
                CopyMemory(&rects[compact], &rects[i], sizeof(RECT));
            }

            compact++;
        }
    }

    // Consolidate overlapping / mergeable rectangles.
    u32 target = 0;
    for (u32 i = 0; i < compact; i++) {
        bool merged = TRUE;
        RECT current;
        CopyMemory(&current, &rects[i], sizeof(RECT));

        // Continuously absorb any existing target rects that merge with 'current'
        while (merged) {
            merged = FALSE;
            for (u32 j = 0; j < target; j++) {
                MAKETYPE(RECT, combined);
                if (MergeRect(&combined, &rects[j], &current)) {
                    CopyMemory(&current, &combined, sizeof(RECT));
                    CopyMemory(&rects[j], &rects[target - 1], sizeof(RECT));
                    target--;
                    merged = TRUE;
                    break;
                }
            }
        }

        CopyMemory(&rects[target++], &current, sizeof(RECT));
    }

    self->region->rdh.nCount = target;
    self->region->rdh.nRgnSize = target * sizeof(RECT);

    // Update region boundaries.
    ZeroMemory(&self->region->rdh.rcBound, sizeof(RECT));

    if (target != 0) {
        RECT bounds;
        bounds.left = INT_MAX;
        bounds.top = INT_MAX;
        bounds.right = INT_MIN;
        bounds.bottom = INT_MIN;

        for (u32 i = 0; i < target; i++) {
            bounds.left = min(bounds.left, rects[i].left);
            bounds.top = min(bounds.top, rects[i].top);
            bounds.right = max(bounds.right, rects[i].right);
            bounds.bottom = max(bounds.bottom, rects[i].bottom);
        }

        if (IsValidRect(&bounds)) {
            CopyMemory(&self->region->rdh.rcBound, &bounds, sizeof(RECT));
        }
    }

    return DD_OK;
}

HRESULT region_capacity(region* self, u32 count) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    HRESULT hr = DD_OK;
    if (self->capacity < count) {
        const u32 size = sizeof(RGNDATAHEADER) + count * sizeof(RECT);
        if (SUCCEEDED(hr = allocator_reallocate(self->allocator, self->region, size, (void**)&self->region))) {
            self->capacity = count;
        }
    }

    return hr;
}

HRESULT region_resize(region* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    return region_capacity(self, max(self->capacity, 1) * DEFAULT_CAPACITY_MULTIPLIER);
}
