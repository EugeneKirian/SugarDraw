#pragma once

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define NOMINMAX
#include <windows.h>

#define DIRECTDRAW_VERSION 0x0700
#include <ddraw.h>
#include <dvp.h>

#define SUGARCALL __stdcall

#define RELEASE_NONE                            0x00000000
#define RELEASE_NOTIFY                          0x00000001

#define DDENUM_NONE								0x00000000L
#define DDENUM_VALID							(DDENUM_ATTACHEDSECONDARYDEVICES \
                                                    | DDENUM_DETACHEDSECONDARYDEVICES | DDENUM_NONDISPLAYDEVICES)

#define DDGDI_NONE                              0x00000000L

#define DDSCL_NONE								0x00000000L
#define DDSCL_DONTHOOKHWND                      0x00000020L
#define DDSCL_VALID								(DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT | DDSCL_NOWINDOWCHANGES \
                                                    | DDSCL_NORMAL | DDSCL_EXCLUSIVE | DDSCL_DONTHOOKHWND \
                                                    | DDSCL_ALLOWMODEX | DDSCL_SETFOCUSWINDOW | DDSCL_SETDEVICEWINDOW \
                                                    | DDSCL_CREATEDEVICEWINDOW | DDSCL_MULTITHREADED | DDSCL_FPUSETUP | DDSCL_FPUPRESERVE)

#define DDSDM_NONE								0x00000000L
#define DDSDM_VALID								(DDSDM_STANDARDVGAMODE)

#define DDSD_NONE								0x00000000L
#define DDSD_VALID								(DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_BACKBUFFERCOUNT \
                                                    | DDSD_ALPHABITDEPTH | DDSD_LPSURFACE | DDSD_PIXELFORMAT | DDSD_CKDESTOVERLAY \
                                                    | DDSD_CKDESTBLT | DDSD_CKSRCOVERLAY | DDSD_CKSRCBLT | DDSD_MIPMAPCOUNT | DDSD_REFRESHRATE \
                                                    | DDSD_LINEARSIZE | DDSD_TEXTURESTAGE | DDSD_FVF | DDSD_SRCVBHANDLE | DDSD_DEPTH)

#define DDSCAPS_NONE							0x00000000L
#define DDSCAPS_3D								0x00000001L
#define	DDSCAPS_DATAEXCHANGE					(DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY)
#define DDSCAPS_SHAREDZBUFFER                   0x01000000L
#define DDSCAPS_SHAREDBACKBUFFER                0x02000000L

#define DDSCAPS_VALID							(DDSCAPS_3D | DDSCAPS_ALPHA | DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP \
                                                    | DDSCAPS_FRONTBUFFER | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_OVERLAY | DDSCAPS_PALETTE \
                                                    | DDSCAPS_PRIMARYSURFACE | DDSCAPS_RESERVED3 | DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE \
                                                    | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_VISIBLE | DDSCAPS_WRITEONLY \
                                                    | DDSCAPS_ZBUFFER | DDSCAPS_OWNDC | DDSCAPS_LIVEVIDEO | DDSCAPS_HWCODEC | DDSCAPS_MODEX \
                                                    | DDSCAPS_MIPMAP | DDSCAPS_RESERVED2 | DDSCAPS_SHAREDZBUFFER | DDSCAPS_SHAREDBACKBUFFER \
                                                    | DDSCAPS_ALLOCONLOAD | DDSCAPS_VIDEOPORT | DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM \
                                                    | DDSCAPS_STANDARDVGAMODE | DDSCAPS_OPTIMIZED)

#define DDSCAPS2_NONE							0x00000000L
#define DDSCAPS2_VALID                          0x3FFFFFFEL /* TODO */

#define DDSCAPS3_NONE							0x00000000L
#define DDSCAPS3_VALID                          DDSCAPS3_MULTISAMPLE_MASK /* TODO */

#define DDSCAPS4_NONE							0x00000000L
#define DDSCAPS4_VALID                          0x00000000L /* TODO */

#define PALETTE_MAX_ENTRY_COUNT                 256

#define DDPFLAGS_NONE							0x0000000L

#define DDPCAPS_NONE							0x0000000L
#define DDPCAPS_VALID							(DDPCAPS_4BIT | DDPCAPS_8BITENTRIES | DDPCAPS_8BIT | DDPCAPS_INITIALIZE \
                                                    | DDPCAPS_PRIMARYSURFACE | DDPCAPS_PRIMARYSURFACELEFT | DDPCAPS_ALLOW256 \
                                                    | DDPCAPS_VSYNC | DDPCAPS_1BIT | DDPCAPS_2BIT | DDPCAPS_ALPHA)


#define DDBLTFAST_NONE                          DDBLTFAST_NOCOLORKEY
#define DDBLTFAST_VALID                         (DDBLTFAST_NOCOLORKEY | DDBLTFAST_SRCCOLORKEY | DDBLTFAST_DESTCOLORKEY \
                                                    | DDBLTFAST_WAIT | DDBLTFAST_DONOTWAIT)

#define DDCKEY_NONE                             0x00000000L
#define DDCKEY_VALID		                    (DDCKEY_COLORSPACE | DDCKEY_DESTBLT	| DDCKEY_DESTOVERLAY | DDCKEY_SRCBLT | DDCKEY_SRCOVERLAY)

#define DDLOCK_NONE                             DDLOCK_SURFACEMEMORYPTR
#define DDLOCK_VALID                            (DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_EVENT | DDLOCK_EVENT \
                                                    | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK | DDLOCK_NOOVERWRITE | DDLOCK_DISCARDCONTENTS \
                                                    | DDLOCK_OKTOSWAP | DDLOCK_DONOTWAIT | DDLOCK_HASVOLUMETEXTUREBOXRECT | DDLOCK_NODIRTYUPDATE)

typedef int bool;

typedef int s32;
typedef unsigned int u32;

typedef float f32;
typedef double f64;

typedef char s8;
typedef unsigned char u8;

const extern GUID SUGARDRAW_DEVICE_GUID;

#define SUGARDRAW_DEVICE_FILE		            "sugar.dll"
#define SUGARDRAW_DEVICE_FILE_LENGTH            32

#define SUGARDRAW_DEVICE_NAME		            "SugarDraw Device"
#define SUGARDRAW_DEVICE_NAME_LENGTH            32

#define SUGARDRAW_DEVICE_DESCRIPTION            "SugarDraw Device Driver"
#define SUGARDRAW_DEVICE_DESCRIPTION_LENGTH     32

#define CompareMemory(a, b, size)               (memcmp(a, b, size) == 0)