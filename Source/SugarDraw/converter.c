#include "converter.h"

#include <stdio.h>

#define CONVERTER_MAX_MESSAGE_COUNT     16
#define CONVERTER_MAX_MESSAGE_LENGTH    1024

typedef struct identifier {
    const GUID* id;
    const char* name;
} identifier;

typedef struct flag {
    u32         value;
    const char* name;
} flag;

#define DDID_COUNT                  26
static const identifier ddid[DDID_COUNT];

#define DDBLT_FLAG_COUNT            27
static const flag ddblts[DDBLT_FLAG_COUNT];

#define DDBLTFAST_FLAG_COUNT        5
static const flag ddbltfasts[DDBLTFAST_FLAG_COUNT];

#define DDBLTFX_FLAG_COUNT          9
static const flag ddbltfxs[DDBLTFX_FLAG_COUNT];

#define DDCOLOR_FLAG_COUNT          7
static const flag ddcolors[DDCOLOR_FLAG_COUNT];

#define DDEDM_FLAG_COUNT            2
static const flag ddedms[DDEDM_FLAG_COUNT];

#define DDENUM_FLAG_COUNT           3
static const flag ddenums[DDENUM_FLAG_COUNT];

#define DDENUMSURFACE_FLAG_COUNT    5
static const flag ddenumsurfaces[DDENUMSURFACE_FLAG_COUNT];

#define DDFLIP_FLAG_COUNT           9
static const flag ddflips[DDFLIP_FLAG_COUNT];

#define DDGBS_FLAG_COUNT            2
static const flag ddgbss[DDGBS_FLAG_COUNT];

#define DDGDI_FLAG_COUNT            1
static const flag ddgdis[DDGDI_FLAG_COUNT];

#define DDLOCK_FLAG_COUNT           12
static const flag ddlocks[DDLOCK_FLAG_COUNT];

#define DDPCAPS_FLAG_COUNT          11
static const flag ddpcapss[DDPCAPS_FLAG_COUNT];

#define DDPF_FLAG_COUNT             19
static const flag ddpfs[DDPF_FLAG_COUNT];

#define DDSCAPS_FLAG_COUNT          29
static const flag ddscapss[DDSCAPS_FLAG_COUNT];

#define DDSCAPS2_FLAG_COUNT         28
static const flag ddscaps2s[DDSCAPS2_FLAG_COUNT];

#define DDSCL_FLAG_COUNT            12
static const flag ddscls[DDSCL_FLAG_COUNT];

#define DDSD_FLAG_COUNT             20
static const flag ddsds[DDSD_FLAG_COUNT];

#define DDSDM_FLAG_COUNT            1
static const flag ddsdms[DDSDM_FLAG_COUNT];

#define DDSGR_FLAG_COUNT            1
static const flag ddsgrs[DDSGR_FLAG_COUNT];

#define DDWAITVB_FLAG_COUNT         3
static const flag ddwvbs[DDWAITVB_FLAG_COUNT];

#define DM_FLAG_COUNT               4
static const flag dms[DM_FLAG_COUNT];

static u32 index = 0;
static char messages[CONVERTER_MAX_MESSAGE_COUNT][CONVERTER_MAX_MESSAGE_LENGTH];

static const char* flag_to_string(const u32 flags, u32 count, const flag* values);

const char* devmodea_to_string(const DEVMODEA* mode) {
    if (mode == NULL) {
        return "NULL";
    }

    const u32 result =
        index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    // TODO incomplete

    sprintf(messages[result],
        "{ dmSize = %u, dmFields = %s, "
        "dmBitsPerPel = %u, dmPelsWidth = %u, "
        "dmPelsHeight = %u, dmDisplayFrequency = %u }",
        mode->dmSize, dm_to_string(mode->dmFields),
        mode->dmBitsPerPel, mode->dmPelsWidth,
        mode->dmPelsHeight, mode->dmDisplayFrequency);

    return messages[result];
}

const char* ddbltfx_to_string(const DDBLTFX* effects) {
    if (effects == NULL) {
        return "NULL";
    }

    const u32 result =
        index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    // TODO incomplete

    sprintf(messages[result],
        "{ dwSize = %u, dwDDFX = %s }",
        effects->dwSize, flag_to_string(effects->dwDDFX, DDBLTFX_FLAG_COUNT, ddbltfxs));

    return messages[result];
}

const char* ddcolorcontrol_to_string(const DDCOLORCONTROL* control) {
    if (control == NULL) {
        return "NULL";
    }

    const u32 result =
        index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    sprintf(messages[result],
        "{ dwSize = %u, dwFlags = %s, "
        "lBrightness = %d, lContrast = %d, "
        "lHue = %d, lSaturation = %d, "
        "lSharpness = %d, lGamma = %d, "
        "lColorEnable = %d }",
        control->dwSize, flag_to_string(control->dwFlags, DDCOLOR_FLAG_COUNT, ddcolors),
        control->lBrightness, control->lContrast,
        control->lHue, control->lSaturation,
        control->lSharpness, control->lGamma,
        control->lColorEnable);

    return messages[result];
}

const char* ddcolorkey_to_string(const DDCOLORKEY* key) {
    if (key == NULL) {
        return "NULL";
    }

    index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    sprintf(messages[index], "0x%08X, 0x%08X",
        key->dwColorSpaceLowValue, key->dwColorSpaceHighValue);

    return messages[index];
}

const char* ddpixelformat_to_string(const DDPIXELFORMAT* format) {
    if (format == NULL) {
        return "NULL";
    }

    const u32 result =
        index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    // TODO incomplete
    // TODO FourCC as string

    sprintf(messages[result],
        "{ dwSize = %u, dwFlags = %s, "
        "dwFourCC = %u, dwRGBBitCount = %u }",
        format->dwSize, ddpf_to_string(format->dwFlags),
        format->dwFourCC, format->dwRGBBitCount);

    return messages[result];
}

const char* ddscaps_to_string(const DDSCAPS* caps) {
    if (caps == NULL) {
        return "NULL";
    }

    const u32 result =
        index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    sprintf(messages[result], "{ dwCaps = %s }",
        flag_to_string(caps->dwCaps, DDSCAPS_FLAG_COUNT, ddscapss));

    return messages[result];
}

const char* ddscaps2_to_string(const DDSCAPS2* caps) {
    if (caps == NULL) {
        return "NULL";
    }

    const u32 result =
        index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    sprintf(messages[result],
        "{ dwCaps = %s, dwCaps2 = %s, dwCaps3 = 0x%08X, dwCaps4 = 0x%08X }",
        flag_to_string(caps->dwCaps, DDSCAPS_FLAG_COUNT, ddscapss),
        flag_to_string(caps->dwCaps2, DDSCAPS2_FLAG_COUNT, ddscaps2s),
        caps->dwCaps3, caps->dwCaps4);

    return messages[result];
}

const char* ddsurfacedesc_to_string(const DDSURFACEDESC* desc) {
    if (desc == NULL) {
        return "NULL";
    }

    const u32 result =
        index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    // TODO incomplete, very incomplete!
    // TODO caps

    sprintf(messages[result],
        "{ dwSize = %u, dwFlags = %s, "
        "dwHeight = %u, dwWidth = %u, "
        "ddpfPixelFormat = %s, ddsCaps = %s }",
        desc->dwSize, ddsd_to_string(desc->dwFlags),
        desc->dwHeight, desc->dwWidth,
        (desc->dwFlags & DDSD_PIXELFORMAT) ? ddpixelformat_to_string(&desc->ddpfPixelFormat) : "{}",
        ddscaps_to_string(&desc->ddsCaps));

    return messages[result];
}

const char* ddsurfacedesc2_to_string(const DDSURFACEDESC2* desc) {
    if (desc == NULL) {
        return "NULL";
    }

    const u32 result =
        index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    // TODO incomplete, very incomplete!
    // TODO caps

    sprintf(messages[result],
        "{ dwSize = %u, dwFlags = %s, "
        "dwHeight = %u, dwWidth = %u, "
        "ddpfPixelFormat = %s, ddsCaps = %s }",
        desc->dwSize, ddsd_to_string(desc->dwFlags),
        desc->dwHeight, desc->dwWidth,
        (desc->dwFlags & DDSD_PIXELFORMAT) ? ddpixelformat_to_string(&desc->ddpfPixelFormat) : "{}",
        ddscaps2_to_string(&desc->ddsCaps));

    return messages[result];
}

const char* ddblt_to_string(const u32 flags) {
    return flag_to_string(flags, DDBLT_FLAG_COUNT, ddblts);
}

const char* ddbltfast_to_string(const u32 flags) {
    return flag_to_string(flags, DDBLTFAST_FLAG_COUNT, ddbltfasts);
}

const char* ddedm_to_string(const u32 flags) {
    return flag_to_string(flags, DDEDM_FLAG_COUNT, ddedms);
}

const char* ddenum_to_string(const u32 flags) {
    return flag_to_string(flags, DDENUM_FLAG_COUNT, ddenums);
}

const char* ddenumsurfaces_to_string(const u32 flags) {
    return flag_to_string(flags, DDENUMSURFACE_FLAG_COUNT, ddenumsurfaces);
}

const char* ddflip_to_string(const u32 flags) {
    return flag_to_string(flags, DDFLIP_FLAG_COUNT, ddflips);
}

const char* ddgbs_to_string(const u32 flags) {
    return flag_to_string(flags, DDGBS_FLAG_COUNT, ddgbss);
}

const char* ddgdi_to_string(const u32 flags) {
    return flag_to_string(flags, DDGDI_FLAG_COUNT, ddgdis);
}

const char* ddlock_to_string(const u32 flags) {
    return flag_to_string(flags, DDLOCK_FLAG_COUNT, ddlocks);
}

const char* ddpcaps_to_string(const u32 flags) {
    return flag_to_string(flags, DDPCAPS_FLAG_COUNT, ddpcapss);
}

const char* ddpf_to_string(const u32 flags) {
    return flag_to_string(flags, DDPF_FLAG_COUNT, ddpfs);
}

const char* ddscl_to_string(const u32 flags) {
    return flag_to_string(flags, DDSCL_FLAG_COUNT, ddscls);
}

const char* ddsd_to_string(const u32 flags) {
    return flag_to_string(flags, DDSD_FLAG_COUNT, ddsds);
}

const char* ddsdm_to_string(const u32 flags) {
    return flag_to_string(flags, DDSDM_FLAG_COUNT, ddsdms);
}

const char* ddsgr_to_string(const u32 flags) {
    return flag_to_string(flags, DDSGR_FLAG_COUNT, ddsgrs);
}

const char* ddwaitvb_to_string(const u32 flags) {
    return flag_to_string(flags, DDWAITVB_FLAG_COUNT, ddwvbs);
}

const char* disp_change_to_string(const s32 result) {
    switch (result) {
    case DISP_CHANGE_SUCCESSFUL: { return "DISP_CHANGE_SUCCESSFUL"; }
    case DISP_CHANGE_RESTART: { return "DISP_CHANGE_RESTART"; }
    case DISP_CHANGE_FAILED: { return "DISP_CHANGE_FAILED"; }
    case DISP_CHANGE_BADMODE: { return "DISP_CHANGE_BADMODE"; }
    case DISP_CHANGE_NOTUPDATED: { return "DISP_CHANGE_NOTUPDATED"; }
    case DISP_CHANGE_BADFLAGS: { return "DISP_CHANGE_BADFLAGS"; }
    case DISP_CHANGE_BADPARAM: { return "DISP_CHANGE_BADPARAM"; }
    case DISP_CHANGE_BADDUALVIEW: { return "DISP_CHANGE_BADDUALVIEW"; }
    }

    index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    sprintf(messages[index], "%d", result);

    return messages[index];
}

const char* dm_to_string(const u32 flags) {
    return flag_to_string(flags, DM_FLAG_COUNT, dms);
}

const char* guid_to_string(const GUID* id) {
    if (id == NULL) {
        return "NULL";
    }

    if (id == (GUID*)DDCREATE_HARDWAREONLY) {
        return "DDCREATE_HARDWAREONLY";
    }

    if (id == (GUID*)DDCREATE_EMULATIONONLY) {
        return "DDCREATE_EMULATIONONLY";
    }

    for (u32 i = 0; i < DDID_COUNT; i++) {
        if (IsEqualGUID(ddid[i].id, id)) {
            return ddid[i].name;
        }
    }

    index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    sprintf(messages[index], "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        id->Data1, id->Data2, id->Data3,
        id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
        id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);

    return messages[index];
}

const char* hresult_to_string(const HRESULT hr) {
    if (hr == DD_OK) {
        return "DD_OK";
    }

    switch (hr) {
    case DDERR_UNSUPPORTED: { return "DDERR_UNSUPPORTED"; }
    case E_NOINTERFACE: { return "E_NOINTERFACE"; }
    case E_POINTER: { return "E_POINTER"; }
    case DDERR_GENERIC: { return "DDERR_GENERIC"; }
    case CLASS_E_NOAGGREGATION: { return "CLASS_E_NOAGGREGATION"; }
    case CLASS_E_CLASSNOTAVAILABLE: { return "CLASS_E_CLASSNOTAVAILABLE"; }
    case DDERR_NOTINITIALIZED: { return "DDERR_NOTINITIALIZED"; }
    case DDERR_OUTOFMEMORY: { return "DDERR_OUTOFMEMORY"; }
    case DDERR_INVALIDPARAMS: { return "DDERR_INVALIDPARAMS"; }
    case DDERR_ALREADYINITIALIZED: { return "DDERR_ALREADYINITIALIZED"; }
    case DDERR_CANNOTATTACHSURFACE: { return "DDERR_CANNOTATTACHSURFACE"; }
    case DDERR_CANNOTDETACHSURFACE: { return "DDERR_CANNOTDETACHSURFACE"; }
    case DDERR_CURRENTLYNOTAVAIL: { return "DDERR_CURRENTLYNOTAVAIL"; }
    case DDERR_EXCEPTION: { return "DDERR_EXCEPTION"; }
    case DDERR_HEIGHTALIGN: { return "DDERR_HEIGHTALIGN"; }
    case DDERR_INCOMPATIBLEPRIMARY: { return "DDERR_INCOMPATIBLEPRIMARY"; }
    case DDERR_INVALIDCAPS: { return "DDERR_INVALIDCAPS"; }
    case DDERR_INVALIDCLIPLIST: { return "DDERR_INVALIDCLIPLIST"; }
    case DDERR_INVALIDMODE: { return "DDERR_INVALIDMODE"; }
    case DDERR_INVALIDOBJECT: { return "DDERR_INVALIDOBJECT"; }
    case DDERR_INVALIDPIXELFORMAT: { return "DDERR_INVALIDPIXELFORMAT"; }
    case DDERR_INVALIDRECT: { return "DDERR_INVALIDRECT"; }
    case DDERR_LOCKEDSURFACES: { return "DDERR_LOCKEDSURFACES"; }
    case DDERR_NO3D: { return "DDERR_NO3D"; }
    case DDERR_NOALPHAHW: { return "DDERR_NOALPHAHW"; }
    case DDERR_NOSTEREOHARDWARE: { return "DDERR_NOSTEREOHARDWARE"; }
    case DDERR_NOSURFACELEFT: { return "DDERR_NOSURFACELEFT"; }
    case DDERR_NOCLIPLIST: { return "DDERR_NOCLIPLIST"; }
    case DDERR_NOCOLORCONVHW: { return "DDERR_NOCOLORCONVHW"; }
    case DDERR_NOCOOPERATIVELEVELSET: { return "DDERR_NOCOOPERATIVELEVELSET"; }
    case DDERR_NOCOLORKEY: { return "DDERR_NOCOLORKEY"; }
    case DDERR_NOCOLORKEYHW: { return "DDERR_NOCOLORKEYHW"; }
    case DDERR_NODIRECTDRAWSUPPORT: { return "DDERR_NODIRECTDRAWSUPPORT"; }
    case DDERR_NOEXCLUSIVEMODE: { return "DDERR_NOEXCLUSIVEMODE"; }
    case DDERR_NOFLIPHW: { return "DDERR_NOFLIPHW"; }
    case DDERR_NOGDI: { return "DDERR_NOGDI"; }
    case DDERR_NOMIRRORHW: { return "DDERR_NOMIRRORHW"; }
    case DDERR_NOTFOUND: { return "DDERR_NOTFOUND"; }
    case DDERR_NOOVERLAYHW: { return "DDERR_NOOVERLAYHW"; }
    case DDERR_OVERLAPPINGRECTS: { return "DDERR_OVERLAPPINGRECTS"; }
    case DDERR_NORASTEROPHW: { return "DDERR_NORASTEROPHW"; }
    case DDERR_NOROTATIONHW: { return "DDERR_NOROTATIONHW"; }
    case DDERR_NOSTRETCHHW: { return "DDERR_NOSTRETCHHW"; }
    case DDERR_NOT4BITCOLOR: { return "DDERR_NOT4BITCOLOR"; }
    case DDERR_NOT4BITCOLORINDEX: { return "DDERR_NOT4BITCOLORINDEX"; }
    case DDERR_NOT8BITCOLOR: { return "DDERR_NOT8BITCOLOR"; }
    case DDERR_NOTEXTUREHW: { return "DDERR_NOTEXTUREHW"; }
    case DDERR_NOVSYNCHW: { return "DDERR_NOVSYNCHW"; }
    case DDERR_NOZBUFFERHW: { return "DDERR_NOZBUFFERHW"; }
    case DDERR_NOZOVERLAYHW: { return "DDERR_NOZOVERLAYHW"; }
    case DDERR_OUTOFCAPS: { return "DDERR_OUTOFCAPS"; }
    case DDERR_OUTOFVIDEOMEMORY: { return "DDERR_OUTOFVIDEOMEMORY"; }
    case DDERR_OVERLAYCANTCLIP: { return "DDERR_OVERLAYCANTCLIP"; }
    case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: { return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE"; }
    case DDERR_PALETTEBUSY: { return "DDERR_PALETTEBUSY"; }
    case DDERR_COLORKEYNOTSET: { return "DDERR_COLORKEYNOTSET"; }
    case DDERR_SURFACEALREADYATTACHED: { return "DDERR_SURFACEALREADYATTACHED"; }
    case DDERR_SURFACEALREADYDEPENDENT: { return "DDERR_SURFACEALREADYDEPENDENT"; }
    case DDERR_SURFACEBUSY: { return "DDERR_SURFACEBUSY"; }
    case DDERR_CANTLOCKSURFACE: { return "DDERR_CANTLOCKSURFACE"; }
    case DDERR_SURFACEISOBSCURED: { return "DDERR_SURFACEISOBSCURED"; }
    case DDERR_SURFACELOST: { return "DDERR_SURFACELOST"; }
    case DDERR_SURFACENOTATTACHED: { return "DDERR_SURFACENOTATTACHED"; }
    case DDERR_TOOBIGHEIGHT: { return "DDERR_TOOBIGHEIGHT"; }
    case DDERR_TOOBIGSIZE: { return "DDERR_TOOBIGSIZE"; }
    case DDERR_TOOBIGWIDTH: { return "DDERR_TOOBIGWIDTH"; }
    case DDERR_UNSUPPORTEDFORMAT: { return "DDERR_UNSUPPORTEDFORMAT"; }
    case DDERR_UNSUPPORTEDMASK: { return "DDERR_UNSUPPORTEDMASK"; }
    case DDERR_INVALIDSTREAM: { return "DDERR_INVALIDSTREAM"; }
    case DDERR_VERTICALBLANKINPROGRESS: { return "DDERR_VERTICALBLANKINPROGRESS"; }
    case DDERR_WASSTILLDRAWING: { return "DDERR_WASSTILLDRAWING"; }
    case DDERR_DDSCAPSCOMPLEXREQUIRED: { return "DDERR_DDSCAPSCOMPLEXREQUIRED"; }
    case DDERR_XALIGN: { return "DDERR_XALIGN"; }
    case DDERR_INVALIDDIRECTDRAWGUID: { return "DDERR_INVALIDDIRECTDRAWGUID"; }
    case DDERR_DIRECTDRAWALREADYCREATED: { return "DDERR_DIRECTDRAWALREADYCREATED"; }
    case DDERR_NODIRECTDRAWHW: { return "DDERR_NODIRECTDRAWHW"; }
    case DDERR_PRIMARYSURFACEALREADYEXISTS: { return "DDERR_PRIMARYSURFACEALREADYEXISTS"; }
    case DDERR_NOEMULATION: { return "DDERR_NOEMULATION"; }
    case DDERR_REGIONTOOSMALL: { return "DDERR_REGIONTOOSMALL"; }
    case DDERR_CLIPPERISUSINGHWND: { return "DDERR_CLIPPERISUSINGHWND"; }
    case DDERR_NOCLIPPERATTACHED: { return "DDERR_NOCLIPPERATTACHED"; }
    case DDERR_NOHWND: { return "DDERR_NOHWND"; }
    case DDERR_HWNDSUBCLASSED: { return "DDERR_HWNDSUBCLASSED"; }
    case DDERR_HWNDALREADYSET: { return "DDERR_HWNDALREADYSET"; }
    case DDERR_NOPALETTEATTACHED: { return "DDERR_NOPALETTEATTACHED"; }
    case DDERR_NOPALETTEHW: { return "DDERR_NOPALETTEHW"; }
    case DDERR_BLTFASTCANTCLIP: { return "DDERR_BLTFASTCANTCLIP"; }
    case DDERR_NOBLTHW: { return "DDERR_NOBLTHW"; }
    case DDERR_NODDROPSHW: { return "DDERR_NODDROPSHW"; }
    case DDERR_OVERLAYNOTVISIBLE: { return "DDERR_OVERLAYNOTVISIBLE"; }
    case DDERR_NOOVERLAYDEST: { return "DDERR_NOOVERLAYDEST"; }
    case DDERR_INVALIDPOSITION: { return "DDERR_INVALIDPOSITION"; }
    case DDERR_NOTAOVERLAYSURFACE: { return "DDERR_NOTAOVERLAYSURFACE"; }
    case DDERR_EXCLUSIVEMODEALREADYSET: { return "DDERR_EXCLUSIVEMODEALREADYSET"; }
    case DDERR_NOTFLIPPABLE: { return "DDERR_NOTFLIPPABLE"; }
    case DDERR_CANTDUPLICATE: { return "DDERR_CANTDUPLICATE"; }
    case DDERR_NOTLOCKED: { return "DDERR_NOTLOCKED"; }
    case DDERR_CANTCREATEDC: { return "DDERR_CANTCREATEDC"; }
    case DDERR_NODC: { return "DDERR_NODC"; }
    case DDERR_WRONGMODE: { return "DDERR_WRONGMODE"; }
    case DDERR_IMPLICITLYCREATED: { return "DDERR_IMPLICITLYCREATED"; }
    case DDERR_NOTPALETTIZED: { return "DDERR_NOTPALETTIZED"; }
    case DDERR_UNSUPPORTEDMODE: { return "DDERR_UNSUPPORTEDMODE"; }
    case DDERR_NOMIPMAPHW: { return "DDERR_NOMIPMAPHW"; };
    case DDERR_INVALIDSURFACETYPE: { return "DDERR_INVALIDSURFACETYPE"; }
    case DDERR_NOOPTIMIZEHW: { return "DDERR_NOOPTIMIZEHW"; }
    case DDERR_NOTLOADED: { return "DDERR_NOTLOADED"; }
    case DDERR_NOFOCUSWINDOW: { return "DDERR_NOFOCUSWINDOW"; }
    case DDERR_NOTONMIPMAPSUBLEVEL: { return "DDERR_NOTONMIPMAPSUBLEVEL"; }
    case DDERR_DCALREADYCREATED: { return "DDERR_DCALREADYCREATED"; }
    case DDERR_NONONLOCALVIDMEM: { return "DDERR_NONONLOCALVIDMEM"; }
    case DDERR_CANTPAGELOCK: { return "DDERR_CANTPAGELOCK"; }
    case DDERR_UNRECOGNIZEDGUID: { return "DDERR_UNRECOGNIZEDGUID"; }
    case DDERR_NOTANOPTIMIZEDSURFACE: { return "DDERR_NOTANOPTIMIZEDSURFACE"; }
    case DDERR_ISOPTIMIZEDSURFACE: { return "DDERR_ISOPTIMIZEDSURFACE"; }
    case DDERR_CANTPAGEUNLOCK: { return "DDERR_CANTPAGEUNLOCK"; }
    case DDERR_NOTPAGELOCKED: { return "DDERR_NOTPAGELOCKED"; }
    case DDERR_MOREDATA: { return "DDERR_MOREDATA"; }
    case DDERR_EXPIRED: { return "DDERR_EXPIRED"; }
    case DDERR_TESTFINISHED: { return "DDERR_TESTFINISHED"; }
    case DDERR_NEWMODE: { return "DDERR_NEWMODE"; }
    case DDERR_D3DNOTINITIALIZED: { return "DDERR_D3DNOTINITIALIZED"; }
    case DDERR_VIDEONOTACTIVE: { return "DDERR_VIDEONOTACTIVE"; }
    case DDERR_NOMONITORINFORMATION: { return "DDERR_NOMONITORINFORMATION"; }
    case DDERR_NODRIVERSUPPORT: { return "DDERR_NODRIVERSUPPORT"; }
    case DDERR_DEVICEDOESNTOWNSURFACE: { return "DDERR_DEVICEDOESNTOWNSURFACE"; }
    }

    index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    sprintf(messages[index], "0x%08X", hr);

    return messages[index];
}

const char* log_level_to_string(const log_level level) {
    switch (level) {
    case LOG_LEVEL_ERROR: { return "ERROR"; }
    case LOG_LEVEL_WARNING: { return "WARNING"; }
    case LOG_LEVEL_INFO: { return "INFO"; }
    case LOG_LEVEL_TRACE: { return "TRACE"; }
    }

    return "UNKNOWN";
}

const char* rect_to_string(const RECT* rect) {
    if (rect == NULL) {
        return "NULL";
    }

    index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;

    sprintf(messages[index],
        "{ left = %d, top = %d, right= %d, bottom = %d }",
        rect->left, rect->top, rect->right, rect->bottom);

    return messages[index];
}

const char* flag_to_string(const u32 flags, u32 count, const flag* values) {
    u32 value = flags;
    const bool parens = value != 0;

    index = (index + 1) % CONVERTER_MAX_MESSAGE_COUNT;
    char* result = messages[index];

    sprintf(result, "0x%08X", value);

    if (parens) {
        strcat(result, " (");
    }

    bool exists = FALSE;

    for (u32 i = 0; i < count; i++) {
        if (value & values[i].value) {
            value &= ~values[i].value;

            if (exists) {
                strcat(result, " | ");
            }

            strcat(result, values[i].name);

            exists = TRUE;
        }
    }

    if (value != 0) {
        if (exists) {
            strcat(result, " | ");
        }

        char unknown[32];
        sprintf(unknown, "0x%08X", value);
        strcat(result, unknown);
    }

    if (parens) {
        strcat(result, ")");
    }

    return result;
}

const identifier ddid[DDID_COUNT] = {
    { &IID_IUnknown,                    "IID_IUnknown" },
    { &CLSID_DirectDraw,                "CLSID_DirectDraw" },
    { &CLSID_DirectDraw7,               "CLSID_DirectDraw7" },
    { &CLSID_DirectDrawClipper,         "CLSID_DirectDrawClipper" },
    { &CLSID_DirectDrawFactory,         "CLSID_DirectDrawFactory" },
    { &IID_IDDVideoPortContainer,       "IID_IDDVideoPortContainer" },
    { &IID_IDirect3DHALDevice,          "IID_IDirect3DHALDevice" },
    { &IID_IDirect3DRampDevice,         "IID_IDirect3DRampDevice" },
    { &IID_IDirect3DRGBDevice,          "IID_IDirect3DRGBDevice" },
    { &IID_IDirectDraw,                 "IID_IDirectDraw" },
    { &IID_IDirectDraw2,                "IID_IDirectDraw2" },
    { &IID_IDirectDraw3,                "IID_IDirectDraw3" },
    { &IID_IDirectDraw4,                "IID_IDirectDraw4" },
    { &IID_IDirectDraw7,                "IID_IDirectDraw7" },
    { &IID_IDirectDrawClipper,          "IID_IDirectDrawClipper" },
    { &IID_IDirectDrawColorControl,     "IID_IDirectDrawColorControl" },
    { &IID_IDirectDrawFactory,          "IID_IDirectDrawFactory" },
    { &IID_IDirectDrawGammaControl,     "IID_IDirectDrawGammaControl" },
    { &IID_IDirectDrawPalette,          "IID_IDirectDrawPalette" },
    { &IID_IDirectDrawSurface,          "IID_IDirectDrawSurface" },
    { &IID_IDirectDrawSurface2,         "IID_IDirectDrawSurface2" },
    { &IID_IDirectDrawSurface3,         "IID_IDirectDrawSurface3" },
    { &IID_IDirectDrawSurface4,         "IID_IDirectDrawSurface4" },
    { &IID_IDirectDrawSurface7,         "IID_IDirectDrawSurface7" },
    { &IID_IDirectDrawVideoPort,        "IID_IDirectDrawVideoPort" },
    { &IID_IDirectDrawVideoPortNotify,  "IID_IDirectDrawVideoPortNotify" }
};

const flag ddblts[DDBLT_FLAG_COUNT] = {
    { DDBLT_DONOTWAIT,                  "DDBLT_DONOTWAIT" },
    { DDBLT_DEPTHFILL,                  "DDBLT_DEPTHFILL" },
    { DDBLT_WAIT,                       "DDBLT_WAIT" },
    { DDBLT_ZBUFFERSRCOVERRIDE,         "DDBLT_ZBUFFERSRCOVERRIDE" },
    { DDBLT_ZBUFFERSRCCONSTOVERRIDE,    "DDBLT_ZBUFFERSRCCONSTOVERRIDE" },
    { DDBLT_ZBUFFERDESTOVERRIDE,        "DDBLT_ZBUFFERDESTOVERRIDE" },
    { DDBLT_ZBUFFERDESTCONSTOVERRIDE,   "DDBLT_ZBUFFERDESTCONSTOVERRIDE" },
    { DDBLT_ZBUFFER,                    "DDBLT_ZBUFFER" },
    { DDBLT_ROTATIONANGLE,              "DDBLT_ROTATIONANGLE" },
    { DDBLT_ROP,                        "DDBLT_ROP" },
    { DDBLT_KEYSRCOVERRIDE,             "DDBLT_KEYSRCOVERRIDE" },
    { DDBLT_KEYSRC,                     "DDBLT_KEYSRC" },
    { DDBLT_KEYDESTOVERRIDE,            "DDBLT_KEYDESTOVERRIDE" },
    { DDBLT_KEYDEST,                    "DDBLT_KEYDEST" },
    { DDBLT_DDROPS,                     "DDBLT_DDROPS" },
    { DDBLT_DDFX,                       "DDBLT_DDFX" },
    { DDBLT_COLORFILL,                  "DDBLT_COLORFILL" },
    { DDBLT_ASYNC,                      "DDBLT_ASYNC" },
    { DDBLT_ALPHASRCSURFACEOVERRIDE,    "DDBLT_ALPHASRCSURFACEOVERRIDE" },
    { DDBLT_ALPHASRCNEG,                "DDBLT_ALPHASRCNEG" },
    { DDBLT_ALPHASRCCONSTOVERRIDE,      "DDBLT_ALPHASRCCONSTOVERRIDE" },
    { DDBLT_ALPHASRC,                   "DDBLT_ALPHASRC" },
    { DDBLT_ALPHAEDGEBLEND,             "DDBLT_ALPHAEDGEBLEND" },
    { DDBLT_ALPHADESTSURFACEOVERRIDE,   "DDBLT_ALPHADESTSURFACEOVERRIDE" },
    { DDBLT_ALPHADESTNEG,               "DDBLT_ALPHADESTNEG" },
    { DDBLT_ALPHADESTCONSTOVERRIDE,     "DDBLT_ALPHADESTCONSTOVERRIDE" },
    { DDBLT_ALPHADEST,                  "DDBLT_ALPHADEST" }
};

const flag ddbltfasts[DDBLTFAST_FLAG_COUNT] = {
    { DDBLTFAST_DONOTWAIT,              "DDBLTFAST_DONOTWAIT" },
    { DDBLTFAST_WAIT,                   "DDBLTFAST_WAIT" },
    { DDBLTFAST_DESTCOLORKEY,           "DDBLTFAST_DESTCOLORKEY" },
    { DDBLTFAST_SRCCOLORKEY,            "DDBLTFAST_SRCCOLORKEY" },
    { DDBLTFAST_NOCOLORKEY,             "DDBLTFAST_NOCOLORKEY" }
};

const flag ddbltfxs[DDBLTFX_FLAG_COUNT] = {
    { DDBLTFX_ZBUFFERBASEDEST,          "DDBLTFX_ZBUFFERBASEDEST" },
    { DDBLTFX_ZBUFFERRANGE,             "DDBLTFX_ZBUFFERRANGE" },
    { DDBLTFX_ROTATE90,                 "DDBLTFX_ROTATE90" },
    { DDBLTFX_ROTATE270,                "DDBLTFX_ROTATE270" },
    { DDBLTFX_ROTATE180,                "DDBLTFX_ROTATE180" },
    { DDBLTFX_NOTEARING,                "DDBLTFX_NOTEARING" },
    { DDBLTFX_MIRRORUPDOWN,             "DDBLTFX_MIRRORUPDOWN" },
    { DDBLTFX_MIRRORLEFTRIGHT,          "DDBLTFX_MIRRORLEFTRIGHT" },
    { DDBLTFX_ARITHSTRETCHY,            "DDBLTFX_ARITHSTRETCHY" }
};

const flag ddcolors[DDCOLOR_FLAG_COUNT] = {
    { DDCOLOR_COLORENABLE,              "DDCOLOR_COLORENABLE"},
    { DDCOLOR_GAMMA,                    "DDCOLOR_GAMMA" },
    { DDCOLOR_SHARPNESS,                "DDCOLOR_SHARPNESS" },
    { DDCOLOR_SATURATION,               "DDCOLOR_SATURATION" },
    { DDCOLOR_HUE,                      "DDCOLOR_HUE" },
    { DDCOLOR_CONTRAST,                 "DDCOLOR_CONTRAST" },
    { DDCOLOR_BRIGHTNESS,               "DDCOLOR_BRIGHTNESS" }
};

const flag ddedms[DDEDM_FLAG_COUNT] = {
    { DDEDM_STANDARDVGAMODES,           "DDEDM_STANDARDVGAMODES" },
    { DDEDM_REFRESHRATES,               "DDEDM_REFRESHRATES" }
};

const flag ddenums[DDENUM_FLAG_COUNT] = {
    { DDENUM_NONDISPLAYDEVICES,         "DDENUM_NONDISPLAYDEVICES" },
    { DDENUM_DETACHEDSECONDARYDEVICES,  "DDENUM_DETACHEDSECONDARYDEVICES" },
    { DDENUM_ATTACHEDSECONDARYDEVICES,  "DDENUM_ATTACHEDSECONDARYDEVICES" }
};

const flag ddenumsurfaces[DDENUMSURFACE_FLAG_COUNT] = {
    { DDENUMSURFACES_DOESEXIST,         "DDENUMSURFACES_DOESEXIST" },
    { DDENUMSURFACES_CANBECREATED,      "DDENUMSURFACES_CANBECREATED" },
    { DDENUMSURFACES_NOMATCH,           "DDENUMSURFACES_NOMATCH" },
    { DDENUMSURFACES_MATCH,             "DDENUMSURFACES_MATCH" },
    { DDENUMSURFACES_ALL,               "DDENUMSURFACES_ALL" }
};

const flag ddflips[DDFLIP_FLAG_COUNT] = {
    { DDFLIP_DONOTWAIT,                 "DDFLIP_DONOTWAIT" },
    { DDFLIP_STEREO,                    "DDFLIP_STEREO" },
    { DDFLIP_INTERVAL4,                 "DDFLIP_INTERVAL4" },
    { DDFLIP_INTERVAL3,                 "DDFLIP_INTERVAL3" },
    { DDFLIP_INTERVAL2,                 "DDFLIP_INTERVAL2" },
    { DDFLIP_NOVSYNC,                   "DDFLIP_NOVSYNC" },
    { DDFLIP_ODD,                       "DDFLIP_ODD" },
    { DDFLIP_EVEN,                      "DDFLIP_EVEN" },
    { DDFLIP_WAIT,                      "DDFLIP_WAIT" }
};

const flag ddgbss[DDGBS_FLAG_COUNT] = {
    { DDGBS_ISBLTDONE,                  "DDGBS_ISBLTDONE" },
    { DDGBS_CANBLT,                     "DDGBS_CANBLT" }
};

static const flag ddgdis[DDGDI_FLAG_COUNT] = {
    { DDGDI_GETHOSTIDENTIFIER,          "DDGDI_GETHOSTIDENTIFIER" }
};

const flag ddlocks[DDLOCK_FLAG_COUNT] = {
    { DDLOCK_NODIRTYUPDATE,             "DDLOCK_NODIRTYUPDATE" },
    { DDLOCK_HASVOLUMETEXTUREBOXRECT,   "DDLOCK_HASVOLUMETEXTUREBOXRECT" },
    { DDLOCK_DONOTWAIT,                 "DDLOCK_DONOTWAIT" },
    { DDLOCK_OKTOSWAP,                  "DDLOCK_OKTOSWAP" },
    { DDLOCK_DISCARDCONTENTS,           "DDLOCK_DISCARDCONTENTS" },
    { DDLOCK_NOOVERWRITE,               "DDLOCK_NOOVERWRITE" },
    { DDLOCK_NOSYSLOCK,                 "DDLOCK_NOSYSLOCK" },
    { DDLOCK_WRITEONLY,                 "DDLOCK_WRITEONLY" },
    { DDLOCK_READONLY,                  "DDLOCK_READONLY" },
    { DDLOCK_EVENT,                     "DDLOCK_EVENT" },
    { DDLOCK_WAIT,                      "DDLOCK_WAIT" },
    { DDLOCK_SURFACEMEMORYPTR,          "DDLOCK_SURFACEMEMORYPTR" }
};

const flag ddpcapss[DDPCAPS_FLAG_COUNT] = {
    { DDPCAPS_ALPHA,                    "DDPCAPS_ALPHA" },
    { DDPCAPS_2BIT,                     "DDPCAPS_2BIT" },
    { DDPCAPS_1BIT,                     "DDPCAPS_1BIT" },
    { DDPCAPS_VSYNC,                    "DDPCAPS_VSYNC" },
    { DDPCAPS_ALLOW256,                 "DDPCAPS_ALLOW256" },
    { DDPCAPS_PRIMARYSURFACELEFT,       "DDPCAPS_PRIMARYSURFACELEFT" },
    { DDPCAPS_PRIMARYSURFACE,           "DDPCAPS_PRIMARYSURFACE" },
    { DDPCAPS_INITIALIZE,               "DDPCAPS_INITIALIZE" },
    { DDPCAPS_8BIT,                     "DDPCAPS_8BIT" },
    { DDPCAPS_8BITENTRIES,              "DDPCAPS_8BITENTRIES" },
    { DDPCAPS_4BIT,                     "DDPCAPS_4BIT" }
};

static const flag ddpfs[DDPF_FLAG_COUNT] = {
    { DDPF_BUMPDUDV,                    "DDPF_BUMPDUDV" },
    { DDPF_BUMPLUMINANCE,               "DDPF_BUMPLUMINANCE" },
    { DDPF_LUMINANCE,                   "DDPF_LUMINANCE" },
    { DDPF_ALPHAPREMULT,                "DDPF_ALPHAPREMULT" },
    { DDPF_STENCILBUFFER,               "DDPF_STENCILBUFFER" },
    { DDPF_ZPIXELS,                     "DDPF_ZPIXELS" },
    { DDPF_PALETTEINDEXED2,             "DDPF_PALETTEINDEXED2" },
    { DDPF_PALETTEINDEXED1,             "DDPF_PALETTEINDEXED1" },
    { DDPF_ZBUFFER,                     "DDPF_ZBUFFER" },
    { DDPF_YUV,                         "DDPF_YUV" },
    { DDPF_RGBTOYUV,                    "DDPF_RGBTOYUV" },
    { DDPF_COMPRESSED,                  "DDPF_COMPRESSED" },
    { DDPF_RGB,                         "DDPF_RGB" },
    { DDPF_PALETTEINDEXED8,             "DDPF_PALETTEINDEXED8" },
    { DDPF_PALETTEINDEXEDTO8,           "DDPF_PALETTEINDEXEDTO8" },
    { DDPF_PALETTEINDEXED4,             "DDPF_PALETTEINDEXED4" },
    { DDPF_FOURCC,                      "DDPF_FOURCC" },
    { DDPF_ALPHA,                       "DDPF_ALPHA" },
    { DDPF_ALPHAPIXELS,                 "DDPF_ALPHAPIXELS" }
};

const flag ddscapss[DDSCAPS_FLAG_COUNT] = {
    { DDSCAPS_OPTIMIZED,                "DDSCAPS_OPTIMIZED" },
    { DDSCAPS_STANDARDVGAMODE,          "DDSCAPS_STANDARDVGAMODE" },
    { DDSCAPS_NONLOCALVIDMEM,           "DDSCAPS_NONLOCALVIDMEM" },
    { DDSCAPS_LOCALVIDMEM,              "DDSCAPS_LOCALVIDMEM" },
    { DDSCAPS_VIDEOPORT,                "DDSCAPS_VIDEOPORT" },
    { DDSCAPS_ALLOCONLOAD,              "DDSCAPS_ALLOCONLOAD" },
    { DDSCAPS_MIPMAP,                   "DDSCAPS_MIPMAP" },
    { DDSCAPS_MODEX,                    "DDSCAPS_MODEX" },
    { DDSCAPS_HWCODEC,                  "DDSCAPS_HWCODEC" },
    { DDSCAPS_LIVEVIDEO,                "DDSCAPS_LIVEVIDEO" },
    { DDSCAPS_OWNDC,                    "DDSCAPS_OWNDC" },
    { DDSCAPS_ZBUFFER,                  "DDSCAPS_ZBUFFER" },
    { DDSCAPS_WRITEONLY,                "DDSCAPS_WRITEONLY" },
    { DDSCAPS_VISIBLE,                  "DDSCAPS_VISIBLE" },
    { DDSCAPS_VIDEOMEMORY,              "DDSCAPS_VIDEOMEMORY" },
    { DDSCAPS_3DDEVICE,                 "DDSCAPS_3DDEVICE" },
    { DDSCAPS_TEXTURE,                  "DDSCAPS_TEXTURE" },
    { DDSCAPS_SYSTEMMEMORY,             "DDSCAPS_SYSTEMMEMORY" },
    { DDSCAPS_PRIMARYSURFACELEFT,       "DDSCAPS_PRIMARYSURFACELEFT" },
    { DDSCAPS_PRIMARYSURFACE,           "DDSCAPS_PRIMARYSURFACE" },
    { DDSCAPS_PALETTE,                  "DDSCAPS_PALETTE" },
    { DDSCAPS_OVERLAY,                  "DDSCAPS_OVERLAY" },
    { DDSCAPS_OFFSCREENPLAIN,           "DDSCAPS_OFFSCREENPLAIN" },
    { DDSCAPS_FRONTBUFFER,              "DDSCAPS_FRONTBUFFER" },
    { DDSCAPS_FLIP,                     "DDSCAPS_FLIP" },
    { DDSCAPS_COMPLEX,                  "DDSCAPS_COMPLEX" },
    { DDSCAPS_BACKBUFFER,               "DDSCAPS_BACKBUFFER" },
    { DDSCAPS_ALPHA,                    "DDSCAPS_ALPHA" },
    { DDSCAPS_3D,                       "DDSCAPS_3D" }
};

const flag ddscaps2s[DDSCAPS2_FLAG_COUNT] = {
    { DDSCAPS2_ADDITIONALPRIMARY,       "DDSCAPS2_ADDITIONALPRIMARY" },
    { DDSCAPS2_EXTENDEDFORMATPRIMARY,   "DDSCAPS2_EXTENDEDFORMATPRIMARY" },
    { DDSCAPS2_ENABLEALPHACHANNEL,      "DDSCAPS2_ENABLEALPHACHANNEL" },
    { DDSCAPS2_DISCARDBACKBUFFER,       "DDSCAPS2_DISCARDBACKBUFFER" },
    { DDSCAPS2_DEINTERLACE,             "DDSCAPS2_DEINTERLACE" },
    { DDSCAPS2_NPATCHES,                "DDSCAPS2_NPATCHES" },
    { DDSCAPS2_RTPATCHES,               "DDSCAPS2_RTPATCHES" },
    { DDSCAPS2_POINTS,                  "DDSCAPS2_POINTS" },
    { DDSCAPS2_NOTUSERLOCKABLE,         "DDSCAPS2_NOTUSERLOCKABLE" },
    { DDSCAPS2_VOLUME,                  "DDSCAPS2_VOLUME" },
    { DDSCAPS2_STEREOSURFACELEFT,       "DDSCAPS2_STEREOSURFACELEFT" },
    { DDSCAPS2_DONOTPERSIST,            "DDSCAPS2_DONOTPERSIST" },
    { DDSCAPS2_D3DTEXTUREMANAGE,        "DDSCAPS2_D3DTEXTUREMANAGE" },
    { DDSCAPS2_MIPMAPSUBLEVEL,          "DDSCAPS2_MIPMAPSUBLEVEL" },
    { DDSCAPS2_CUBEMAP_NEGATIVEZ,       "DDSCAPS2_CUBEMAP_NEGATIVEZ" },
    { DDSCAPS2_CUBEMAP_POSITIVEZ,       "DDSCAPS2_CUBEMAP_POSITIVEZ" },
    { DDSCAPS2_CUBEMAP_NEGATIVEY,       "DDSCAPS2_CUBEMAP_NEGATIVEY" },
    { DDSCAPS2_CUBEMAP_POSITIVEY,       "DDSCAPS2_CUBEMAP_POSITIVEY" },
    { DDSCAPS2_CUBEMAP_NEGATIVEX,       "DDSCAPS2_CUBEMAP_NEGATIVEX" },
    { DDSCAPS2_CUBEMAP_POSITIVEX,       "DDSCAPS2_CUBEMAP_POSITIVEX" },
    { DDSCAPS2_CUBEMAP,                 "DDSCAPS2_CUBEMAP" },
    { DDSCAPS2_HINTANTIALIASING,        "DDSCAPS2_HINTANTIALIASING" },
    { DDSCAPS2_OPAQUE,                  "DDSCAPS2_OPAQUE" },
    { DDSCAPS2_TEXTUREMANAGE,           "DDSCAPS2_TEXTUREMANAGE" },
    { DDSCAPS2_HINTSTATIC,              "DDSCAPS2_HINTSTATIC" },
    { DDSCAPS2_HINTDYNAMIC,             "DDSCAPS2_HINTDYNAMIC" },
    { DDSCAPS2_HARDWAREDEINTERLACE,     "DDSCAPS2_HARDWAREDEINTERLACE" },
    { DDSCAPS2_PERSISTENTCONTENTS,      "DDSCAPS2_PERSISTENTCONTENTS" }
};

const flag ddscls[DDSCL_FLAG_COUNT] = {
    { DDSCL_FPUPRESERVE,                "DDSCL_FPUPRESERVE" },
    { DDSCL_FPUSETUP,                   "DDSCL_FPUSETUP" },
    { DDSCL_MULTITHREADED,              "DDSCL_MULTITHREADED" },
    { DDSCL_CREATEDEVICEWINDOW,         "DDSCL_CREATEDEVICEWINDOW" },
    { DDSCL_SETDEVICEWINDOW,            "DDSCL_SETDEVICEWINDOW" },
    { DDSCL_SETFOCUSWINDOW,             "DDSCL_SETFOCUSWINDOW" },
    { DDSCL_ALLOWMODEX,                 "DDSCL_ALLOWMODEX" },
    { DDSCL_EXCLUSIVE,                  "DDSCL_EXCLUSIVE" },
    { DDSCL_NORMAL,                     "DDSCL_NORMAL" },
    { DDSCL_NOWINDOWCHANGES,            "DDSCL_NOWINDOWCHANGES" },
    { DDSCL_ALLOWREBOOT,                "DDSCL_ALLOWREBOOT" },
    { DDSCL_FULLSCREEN,                 "DDSCL_FULLSCREEN" }
};

const flag ddsds[DDSD_FLAG_COUNT] = {
    { DDSD_DEPTH,                       "DDSD_DEPTH" },
    { DDSD_SRCVBHANDLE,                 "DDSD_SRCVBHANDLE" },
    { DDSD_FVF,                         "DDSD_FVF" },
    { DDSD_TEXTURESTAGE,                "DDSD_TEXTURESTAGE" },
    { DDSD_LINEARSIZE,                  "DDSD_LINEARSIZE" },
    { DDSD_REFRESHRATE,                 "DDSD_REFRESHRATE" },
    { DDSD_MIPMAPCOUNT,                 "DDSD_MIPMAPCOUNT" },
    { DDSD_CKSRCBLT,                    "DDSD_CKSRCBLT" },
    { DDSD_CKSRCOVERLAY,                "DDSD_CKSRCOVERLAY" },
    { DDSD_CKDESTBLT,                   "DDSD_CKDESTBLT" },
    { DDSD_CKDESTOVERLAY,               "DDSD_CKDESTOVERLAY" },
    { DDSD_PIXELFORMAT,                 "DDSD_PIXELFORMAT" },
    { DDSD_LPSURFACE,                   "DDSD_LPSURFACE" },
    { DDSD_ALPHABITDEPTH,               "DDSD_ALPHABITDEPTH" },
    { DDSD_ZBUFFERBITDEPTH,             "DDSD_ZBUFFERBITDEPTH" },
    { DDSD_BACKBUFFERCOUNT,             "DDSD_BACKBUFFERCOUNT" },
    { DDSD_PITCH,                       "DDSD_PITCH" },
    { DDSD_WIDTH,                       "DDSD_WIDTH" },
    { DDSD_HEIGHT,                      "DDSD_HEIGHT" },
    { DDSD_CAPS,                        "DDSD_CAPS" }
};

const flag ddsgrs[DDSGR_FLAG_COUNT] = {
    { DDSGR_CALIBRATE,                  "DDSGR_CALIBRATE" }
};

const flag ddwvbs[DDWAITVB_FLAG_COUNT] = {
    { DDWAITVB_BLOCKEND,                "DDWAITVB_BLOCKEND" },
    { DDWAITVB_BLOCKBEGINEVENT,         "DDWAITVB_BLOCKBEGINEVENT" },
    { DDWAITVB_BLOCKBEGIN,              "DDWAITVB_BLOCKBEGIN" }
};

const flag ddsdms[DDSDM_FLAG_COUNT] = {
    { DDSDM_STANDARDVGAMODE,            "DDSDM_STANDARDVGAMODE" }
};

const flag dms[DM_FLAG_COUNT] = {
    { DM_DISPLAYFREQUENCY,              "DM_DISPLAYFREQUENCY" },
    { DM_PELSHEIGHT,                    "DM_PELSHEIGHT" },
    { DM_PELSWIDTH,                     "DM_PELSWIDTH" },
    { DM_BITSPERPEL,                    "DM_BITSPERPEL" }
};
