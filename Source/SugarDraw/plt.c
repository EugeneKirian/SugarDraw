#include "plt.h"

HRESULT plt_create(arena* arena, u32 count, const RGBQUAD* quads, plt** object) {
    if (arena == NULL || object == NULL || quads == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (count != 2 && count != 4 && count != 16 && count != 256) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    plt* instance = NULL;
    if (SUCCEEDED(hr = arena_allocate(arena, sizeof(plt), &instance))) {
        // Traversal of the 32x32x32 quantized color cube
        for (u32 r = 0; r < 32; r++) {
            // Map 5-bit cell coordinate [0..31] to
            // the center of its 8-bit intensity range [0..255]
            const u32 r8 = (r << 3) | (r >> 2);

            for (u32 g = 0; g < 32; g++) {
                const u32 g8 = (g << 3) | (g >> 2);

                for (u32 b = 0; b < 32; b++) {
                    const u32 b8 = (b << 3) | (b >> 2);

                    u32 index = 0;
                    u32 min_dist_sq = UINT_MAX;

                    // Find closest palette color using integer Redmean color distance.
                    for (u32 i = 0; i < count; i++) {
                        // Mean red component accounts for
                        // non-linear human eye response across red intensities.
                        const s32 rmean = ((s32)r8 + (s32)quads[i].rgbRed) / 2;

                        const s32 dr = (s32)r8 - (s32)quads[i].rgbRed;
                        const s32 dg = (s32)g8 - (s32)quads[i].rgbGreen;
                        const s32 db = (s32)b8 - (s32)quads[i].rgbBlue;

                        // Redmean formula: ΔE^2 = (2 + rbar/256) * ΔR^2 + 4 * ΔG^2 + (2 + (255-rbar)/256) * ΔB^2
                        // Uses bitwise shifts (>> 8) instead of float divisions for fast execution.

                        const u32 dist_sq = (u32)((((512 + rmean) * dr * dr) >> 8)
                            + (4 * dg * dg) + (((767 - rmean) * db * db) >> 8));

                        if (dist_sq < min_dist_sq) {
                            min_dist_sq = dist_sq;
                            index = i;

                            // Early exit: 0 distance indicates an exact color match
                            if (dist_sq == 0) {
                                break;
                            }
                        }
                    }

                    // Store closest palette index into table cell
                    instance->colors[r][g][b] = (u8)index;
                }
            }
        }

        *object = instance;
    }

    return hr;
}
