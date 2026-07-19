#include "ddc.h"
#include "ddp.h"
#include "dds.h"
#include "idd.h"
#include "iddp.h"
#include "idds.h"

static HRESULT SUGARCALL idds_add_attached_surface1(idds*, LPDIRECTDRAWSURFACE);
static HRESULT SUGARCALL idds_add_overlay_dirty_rect(idds*, LPRECT);
static HRESULT SUGARCALL idds_blt1(idds*, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);
static HRESULT SUGARCALL idds_blt_batch(idds*, LPDDBLTBATCH, DWORD, DWORD);
static HRESULT SUGARCALL idds_blt_fast1(idds*, DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DWORD);
static HRESULT SUGARCALL idds_delete_attached_surface1(idds*, DWORD, LPDIRECTDRAWSURFACE);
static HRESULT SUGARCALL idds_enum_attached_surfaces(idds*, LPVOID, LPDDENUMSURFACESCALLBACK);
static HRESULT SUGARCALL idds_enum_overlay_z_orders(idds*, DWORD, LPVOID, LPDDENUMSURFACESCALLBACK);
static HRESULT SUGARCALL idds_flip1(idds*, LPDIRECTDRAWSURFACE, DWORD);
static HRESULT SUGARCALL idds_get_attached_surface1(idds*, LPDDSCAPS, LPDIRECTDRAWSURFACE*);
static HRESULT SUGARCALL idds_get_blt_status(idds*, DWORD);
static HRESULT SUGARCALL idds_get_caps1(idds*, LPDDSCAPS);
static HRESULT SUGARCALL idds_get_clipper(idds*, LPDIRECTDRAWCLIPPER*);
static HRESULT SUGARCALL idds_get_color_key(idds*, DWORD, LPDDCOLORKEY);
static HRESULT SUGARCALL idds_get_dc(idds*, HDC*);
static HRESULT SUGARCALL idds_get_flip_status(idds*, DWORD);
static HRESULT SUGARCALL idds_get_overlay_position(idds*, LPLONG, LPLONG);
static HRESULT SUGARCALL idds_get_palette(idds*, LPDIRECTDRAWPALETTE*);
static HRESULT SUGARCALL idds_get_pixel_format(idds*, LPDDPIXELFORMAT);
static HRESULT SUGARCALL idds_get_surface_desc1(idds*, LPDDSURFACEDESC);
static HRESULT SUGARCALL idds_initialize1(idds*, LPDIRECTDRAW, LPDDSURFACEDESC);
static HRESULT SUGARCALL idds_is_lost(idds*);
static HRESULT SUGARCALL idds_lock1(idds*, LPRECT, LPDDSURFACEDESC, DWORD, HANDLE);
static HRESULT SUGARCALL idds_release_dc(idds*, HDC);
static HRESULT SUGARCALL idds_restore(idds*);
static HRESULT SUGARCALL idds_set_clipper(idds*, LPDIRECTDRAWCLIPPER);
static HRESULT SUGARCALL idds_set_color_key(idds*, DWORD, LPDDCOLORKEY);
static HRESULT SUGARCALL idds_set_overlay_position(idds*, LONG, LONG);
static HRESULT SUGARCALL idds_set_palette(idds*, LPDIRECTDRAWPALETTE);
static HRESULT SUGARCALL idds_unlock(idds*, LPRECT);
static HRESULT SUGARCALL idds_update_overlay1(idds*, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDOVERLAYFX);
static HRESULT SUGARCALL idds_update_overlay_display(idds*, DWORD);
static HRESULT SUGARCALL idds_update_overlay_z_order1(idds*, DWORD, LPDIRECTDRAWSURFACE);

static HRESULT SUGARCALL idds_add_attached_surface2(idds*, LPDIRECTDRAWSURFACE2);
static HRESULT SUGARCALL idds_blt2(idds*, LPRECT, LPDIRECTDRAWSURFACE2, LPRECT, DWORD, LPDDBLTFX);
static HRESULT SUGARCALL idds_blt_fast2(idds*, DWORD, DWORD, LPDIRECTDRAWSURFACE2, LPRECT, DWORD);
static HRESULT SUGARCALL idds_delete_attached_surface2(idds*, DWORD, LPDIRECTDRAWSURFACE2);
static HRESULT SUGARCALL idds_flip2(idds*, LPDIRECTDRAWSURFACE2, DWORD);
static HRESULT SUGARCALL idds_get_attached_surface2(idds*, LPDDSCAPS, LPDIRECTDRAWSURFACE2*);
static HRESULT SUGARCALL idds_update_overlay2(idds*, LPRECT, LPDIRECTDRAWSURFACE2, LPRECT, DWORD, LPDDOVERLAYFX);
static HRESULT SUGARCALL idds_update_overlay_z_order2(idds*, DWORD, LPDIRECTDRAWSURFACE2);
static HRESULT SUGARCALL idds_get_dd_interface2(idds*, LPVOID*);
static HRESULT SUGARCALL idds_page_lock2(idds*, DWORD);
static HRESULT SUGARCALL idds_page_unlock2(idds*, DWORD);

static HRESULT SUGARCALL idds_add_attached_surface3(idds*, LPDIRECTDRAWSURFACE3);
static HRESULT SUGARCALL idds_blt3(idds*, LPRECT, LPDIRECTDRAWSURFACE3, LPRECT, DWORD, LPDDBLTFX);
static HRESULT SUGARCALL idds_blt_fast3(idds*, DWORD, DWORD, LPDIRECTDRAWSURFACE3, LPRECT, DWORD);
static HRESULT SUGARCALL idds_delete_attached_surface3(idds*, DWORD, LPDIRECTDRAWSURFACE3);
static HRESULT SUGARCALL idds_flip3(idds*, LPDIRECTDRAWSURFACE3, DWORD);
static HRESULT SUGARCALL idds_get_attached_surface3(idds*, LPDDSCAPS, LPDIRECTDRAWSURFACE3*);
static HRESULT SUGARCALL idds_update_overlay3(idds*, LPRECT, LPDIRECTDRAWSURFACE3, LPRECT, DWORD, LPDDOVERLAYFX);
static HRESULT SUGARCALL idds_update_overlay_z_order3(idds*, DWORD, LPDIRECTDRAWSURFACE3);
static HRESULT SUGARCALL idds_set_surface_desc3(idds*, LPDDSURFACEDESC, DWORD);

static HRESULT SUGARCALL idds_add_attached_surface4(idds*, LPDIRECTDRAWSURFACE4);
static HRESULT SUGARCALL idds_blt4(idds*, LPRECT, LPDIRECTDRAWSURFACE4, LPRECT, DWORD, LPDDBLTFX);
static HRESULT SUGARCALL idds_blt_fast4(idds*, DWORD, DWORD, LPDIRECTDRAWSURFACE4, LPRECT, DWORD);
static HRESULT SUGARCALL idds_delete_attached_surface4(idds*, DWORD, LPDIRECTDRAWSURFACE4);
static HRESULT SUGARCALL idds_flip4(idds*, LPDIRECTDRAWSURFACE4, DWORD);
static HRESULT SUGARCALL idds_get_attached_surface4(idds*, LPDDSCAPS2, LPDIRECTDRAWSURFACE4*);
static HRESULT SUGARCALL idds_get_caps4(idds*, LPDDSCAPS2);
static HRESULT SUGARCALL idds_get_surface_desc4(idds*, LPDDSURFACEDESC2);
static HRESULT SUGARCALL idds_initialize4(idds*, LPDIRECTDRAW, LPDDSURFACEDESC2);
static HRESULT SUGARCALL idds_lock4(idds*, LPRECT, LPDDSURFACEDESC2, DWORD, HANDLE);
static HRESULT SUGARCALL idds_update_overlay4(idds*, LPRECT, LPDIRECTDRAWSURFACE4, LPRECT, DWORD, LPDDOVERLAYFX);
static HRESULT SUGARCALL idds_update_overlay_z_order4(idds*, DWORD, LPDIRECTDRAWSURFACE4);
static HRESULT SUGARCALL idds_set_surface_desc4(idds*, LPDDSURFACEDESC2, DWORD);
static HRESULT SUGARCALL idds_set_private_data4(idds*, REFGUID, LPVOID, DWORD, DWORD);
static HRESULT SUGARCALL idds_get_private_data4(idds*, REFGUID, LPVOID, LPDWORD);
static HRESULT SUGARCALL idds_free_private_data4(idds*, REFGUID);
static HRESULT SUGARCALL idds_get_uniqueness_value4(idds*, LPDWORD);
static HRESULT SUGARCALL idds_change_uniqueness_value4(idds*);

static HRESULT SUGARCALL idds_add_attached_surface7(idds*, LPDIRECTDRAWSURFACE7);
static HRESULT SUGARCALL idds_blt7(idds*, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDBLTFX);
static HRESULT SUGARCALL idds_blt_fast7(idds*, DWORD, DWORD, LPDIRECTDRAWSURFACE7, LPRECT, DWORD);
static HRESULT SUGARCALL idds_delete_attached_surface7(idds*, DWORD, LPDIRECTDRAWSURFACE7);
static HRESULT SUGARCALL idds_flip7(idds*, LPDIRECTDRAWSURFACE7, DWORD);
static HRESULT SUGARCALL idds_get_attached_surface7(idds*, LPDDSCAPS2, LPDIRECTDRAWSURFACE7*);
static HRESULT SUGARCALL idds_update_overlay7(idds*, LPRECT, LPDIRECTDRAWSURFACE7, LPRECT, DWORD, LPDDOVERLAYFX);
static HRESULT SUGARCALL idds_update_overlay_z_order7(idds*, DWORD, LPDIRECTDRAWSURFACE7);
static HRESULT SUGARCALL idds_set_priority7(idds*, DWORD);
static HRESULT SUGARCALL idds_get_priority7(idds*, LPDWORD);
static HRESULT SUGARCALL idds_set_lod7(idds*, DWORD);
static HRESULT SUGARCALL idds_get_lod7(idds*, LPDWORD);

typedef struct iddsu_vft {
    IDDSQUERYINTERFACE          QueryInterface;
    IDDSADDREF                  AddRef;
    IDDSRELEASE                 Release;
} iddsu_vft;

const static iddsu_vft iddsu_self = {
    idds_query_interface,
    idds_add_ref,
    idds_remove_ref
};

typedef struct idds1_vft {
    IDDSQUERYINTERFACE          QueryInterface;
    IDDSADDREF                  AddRef;
    IDDSRELEASE                 Release;
    IDDSADDATTACHEDSURFACE1     AddAttachedSurface;
    IDDSADDOVERLAYDIRTYRECT     AddOverlayDirtyRect;
    IDDSBLT1                    Blt;
    IDDSBLTBATCH                BltBatch;
    IDDSBLTFAST1                BltFast;
    IDDSDELETEATTACHEDSURFACE1  DeleteAttachedSurface;
    IDDSENUMATTACHEDSURFACES    EnumAttachedSurfaces;
    IDDSENUMOVERLAYZORDERS      EnumOverlayZOrders;
    IDDSFLIP1                   Flip;
    IDDSGETATTACHEDSURFACE1     GetAttachedSurface;
    IDDSGETBLTSTATUS            GetBltStatus;
    IDDSGETCAPS1                GetCaps;
    IDDSGETCLIPPER              GetClipper;
    IDDSGETCOLORKEY             GetColorKey;
    IDDSGETDC                   GetDC;
    IDDSGETFLIPSTATUS           GetFlipStatus;
    IDDSGETOVERLAYPOSITION      GetOverlayPosition;
    IDDSGETPALETTE              GetPalette;
    IDDSGETPIXELFORMAT          GetPixelFormat;
    IDDSGETSURFACEDESC1         GetSurfaceDesc;
    IDDSINITIALIZE1             Initialize;
    IDDSISLOST                  IsLost;
    IDDSLOCK1                   Lock;
    IDDSRELEASEDC               ReleaseDC;
    IDDSRESTORE                 Restore;
    IDDSSETCLIPPER              SetClipper;
    IDDSSETCOLORKEY             SetColorKey;
    IDDSSETOVERLAYPOSITION      SetOverlayPosition;
    IDDSSETPALETTE              SetPalette;
    IDDSUNLOCK                  Unlock;
    IDDSUPDATEOVERLAY1          UpdateOverlay;
    IDDSUPDATEOVERLAYDISPLAY    UpdateOverlayDisplay;
    IDDSUPDATEOVERLAYZORDER1    UpdateOverlayZOrder;
} idds1_vft;

const static idds1_vft idds1_self = {
    idds_query_interface,
    idds_add_ref,
    idds_remove_ref,
    idds_add_attached_surface1,
    idds_add_overlay_dirty_rect,
    idds_blt1,
    idds_blt_batch,
    idds_blt_fast1,
    idds_delete_attached_surface1,
    idds_enum_attached_surfaces,
    idds_enum_overlay_z_orders,
    idds_flip1,
    idds_get_attached_surface1,
    idds_get_blt_status,
    idds_get_caps1,
    idds_get_clipper,
    idds_get_color_key,
    idds_get_dc,
    idds_get_flip_status,
    idds_get_overlay_position,
    idds_get_palette,
    idds_get_pixel_format,
    idds_get_surface_desc1,
    idds_initialize1,
    idds_is_lost,
    idds_lock1,
    idds_release_dc,
    idds_restore,
    idds_set_clipper,
    idds_set_color_key,
    idds_set_overlay_position,
    idds_set_palette,
    idds_unlock,
    idds_update_overlay1,
    idds_update_overlay_display,
    idds_update_overlay_z_order1
};

typedef struct idds2_vft {
    IDDSQUERYINTERFACE          QueryInterface;
    IDDSADDREF                  AddRef;
    IDDSRELEASE                 Release;
    IDDSADDATTACHEDSURFACE2     AddAttachedSurface;
    IDDSADDOVERLAYDIRTYRECT     AddOverlayDirtyRect;
    IDDSBLT2                    Blt;
    IDDSBLTBATCH                BltBatch;
    IDDSBLTFAST2                BltFast;
    IDDSDELETEATTACHEDSURFACE2  DeleteAttachedSurface;
    IDDSENUMATTACHEDSURFACES    EnumAttachedSurfaces;
    IDDSENUMOVERLAYZORDERS      EnumOverlayZOrders;
    IDDSFLIP2                   Flip;
    IDDSGETATTACHEDSURFACE2     GetAttachedSurface;
    IDDSGETBLTSTATUS            GetBltStatus;
    IDDSGETCAPS1                GetCaps;
    IDDSGETCLIPPER              GetClipper;
    IDDSGETCOLORKEY             GetColorKey;
    IDDSGETDC                   GetDC;
    IDDSGETFLIPSTATUS           GetFlipStatus;
    IDDSGETOVERLAYPOSITION      GetOverlayPosition;
    IDDSGETPALETTE              GetPalette;
    IDDSGETPIXELFORMAT          GetPixelFormat;
    IDDSGETSURFACEDESC1         GetSurfaceDesc;
    IDDSINITIALIZE1             Initialize;
    IDDSISLOST                  IsLost;
    IDDSLOCK1                   Lock;
    IDDSRELEASEDC               ReleaseDC;
    IDDSRESTORE                 Restore;
    IDDSSETCLIPPER              SetClipper;
    IDDSSETCOLORKEY             SetColorKey;
    IDDSSETOVERLAYPOSITION      SetOverlayPosition;
    IDDSSETPALETTE              SetPalette;
    IDDSUNLOCK                  Unlock;
    IDDSUPDATEOVERLAY2          UpdateOverlay;
    IDDSUPDATEOVERLAYDISPLAY    UpdateOverlayDisplay;
    IDDSUPDATEOVERLAYZORDER2    UpdateOverlayZOrder;
    IDDSGETDDINTERFACE2         GetDDInterface;
    IDDSPAGELOCK2               PageLock;
    IDDSPAGEUNLOCK2             PageUnlock;
} idds2_vft;

const static idds2_vft idds2_self = {
    idds_query_interface,
    idds_add_ref,
    idds_remove_ref,
    idds_add_attached_surface2,
    idds_add_overlay_dirty_rect,
    idds_blt2,
    idds_blt_batch,
    idds_blt_fast2,
    idds_delete_attached_surface2,
    idds_enum_attached_surfaces,
    idds_enum_overlay_z_orders,
    idds_flip2,
    idds_get_attached_surface2,
    idds_get_blt_status,
    idds_get_caps1,
    idds_get_clipper,
    idds_get_color_key,
    idds_get_dc,
    idds_get_flip_status,
    idds_get_overlay_position,
    idds_get_palette,
    idds_get_pixel_format,
    idds_get_surface_desc1,
    idds_initialize1,
    idds_is_lost,
    idds_lock1,
    idds_release_dc,
    idds_restore,
    idds_set_clipper,
    idds_set_color_key,
    idds_set_overlay_position,
    idds_set_palette,
    idds_unlock,
    idds_update_overlay2,
    idds_update_overlay_display,
    idds_update_overlay_z_order2,
    idds_get_dd_interface2,
    idds_page_lock2,
    idds_page_unlock2
};

typedef struct idds3_vft {
    IDDSQUERYINTERFACE          QueryInterface;
    IDDSADDREF                  AddRef;
    IDDSRELEASE                 Release;
    IDDSADDATTACHEDSURFACE7     AddAttachedSurface;
    IDDSADDOVERLAYDIRTYRECT     AddOverlayDirtyRect;
    IDDSBLT7                    Blt;
    IDDSBLTBATCH                BltBatch;
    IDDSBLTFAST7                BltFast;
    IDDSDELETEATTACHEDSURFACE7  DeleteAttachedSurface;
    IDDSENUMATTACHEDSURFACES    EnumAttachedSurfaces;
    IDDSENUMOVERLAYZORDERS      EnumOverlayZOrders;
    IDDSFLIP7                   Flip;
    IDDSGETATTACHEDSURFACE7     GetAttachedSurface;
    IDDSGETBLTSTATUS            GetBltStatus;
    IDDSGETCAPS1                GetCaps;
    IDDSGETCLIPPER              GetClipper;
    IDDSGETCOLORKEY             GetColorKey;
    IDDSGETDC                   GetDC;
    IDDSGETFLIPSTATUS           GetFlipStatus;
    IDDSGETOVERLAYPOSITION      GetOverlayPosition;
    IDDSGETPALETTE              GetPalette;
    IDDSGETPIXELFORMAT          GetPixelFormat;
    IDDSGETSURFACEDESC1         GetSurfaceDesc;
    IDDSINITIALIZE1             Initialize;
    IDDSISLOST                  IsLost;
    IDDSLOCK1                   Lock;
    IDDSRELEASEDC               ReleaseDC;
    IDDSRESTORE                 Restore;
    IDDSSETCLIPPER              SetClipper;
    IDDSSETCOLORKEY             SetColorKey;
    IDDSSETOVERLAYPOSITION      SetOverlayPosition;
    IDDSSETPALETTE              SetPalette;
    IDDSUNLOCK                  Unlock;
    IDDSUPDATEOVERLAY7          UpdateOverlay;
    IDDSUPDATEOVERLAYDISPLAY    UpdateOverlayDisplay;
    IDDSUPDATEOVERLAYZORDER7    UpdateOverlayZOrder;
    IDDSGETDDINTERFACE2         GetDDInterface;
    IDDSPAGELOCK2               PageLock;
    IDDSPAGEUNLOCK2             PageUnlock;
    IDDSSETSURFACEDESC3         SetSurfaceDesc;
} idds3_vft;

const static idds3_vft idds3_self = {
    idds_query_interface,
    idds_add_ref,
    idds_remove_ref,
    idds_add_attached_surface7,
    idds_add_overlay_dirty_rect,
    idds_blt7,
    idds_blt_batch,
    idds_blt_fast7,
    idds_delete_attached_surface7,
    idds_enum_attached_surfaces,
    idds_enum_overlay_z_orders,
    idds_flip7,
    idds_get_attached_surface7,
    idds_get_blt_status,
    idds_get_caps1,
    idds_get_clipper,
    idds_get_color_key,
    idds_get_dc,
    idds_get_flip_status,
    idds_get_overlay_position,
    idds_get_palette,
    idds_get_pixel_format,
    idds_get_surface_desc1,
    idds_initialize1,
    idds_is_lost,
    idds_lock1,
    idds_release_dc,
    idds_restore,
    idds_set_clipper,
    idds_set_color_key,
    idds_set_overlay_position,
    idds_set_palette,
    idds_unlock,
    idds_update_overlay7,
    idds_update_overlay_display,
    idds_update_overlay_z_order7,
    idds_get_dd_interface2,
    idds_page_lock2,
    idds_page_unlock2,
    idds_set_surface_desc3
};

typedef struct idds4_vft {
    IDDSQUERYINTERFACE          QueryInterface;
    IDDSADDREF                  AddRef;
    IDDSRELEASE                 Release;
    IDDSADDATTACHEDSURFACE4     AddAttachedSurface;
    IDDSADDOVERLAYDIRTYRECT     AddOverlayDirtyRect;
    IDDSBLT4                    Blt;
    IDDSBLTBATCH                BltBatch;
    IDDSBLTFAST4                BltFast;
    IDDSDELETEATTACHEDSURFACE4  DeleteAttachedSurface;
    IDDSENUMATTACHEDSURFACES    EnumAttachedSurfaces;
    IDDSENUMOVERLAYZORDERS      EnumOverlayZOrders;
    IDDSFLIP4                   Flip;
    IDDSGETATTACHEDSURFACE4     GetAttachedSurface;
    IDDSGETBLTSTATUS            GetBltStatus;
    IDDSGETCAPS4                GetCaps;
    IDDSGETCLIPPER              GetClipper;
    IDDSGETCOLORKEY             GetColorKey;
    IDDSGETDC                   GetDC;
    IDDSGETFLIPSTATUS           GetFlipStatus;
    IDDSGETOVERLAYPOSITION      GetOverlayPosition;
    IDDSGETPALETTE              GetPalette;
    IDDSGETPIXELFORMAT          GetPixelFormat;
    IDDSGETSURFACEDESC4         GetSurfaceDesc;
    IDDSINITIALIZE4             Initialize;
    IDDSISLOST                  IsLost;
    IDDSLOCK4                   Lock;
    IDDSRELEASEDC               ReleaseDC;
    IDDSRESTORE                 Restore;
    IDDSSETCLIPPER              SetClipper;
    IDDSSETCOLORKEY             SetColorKey;
    IDDSSETOVERLAYPOSITION      SetOverlayPosition;
    IDDSSETPALETTE              SetPalette;
    IDDSUNLOCK                  Unlock;
    IDDSUPDATEOVERLAY4          UpdateOverlay;
    IDDSUPDATEOVERLAYDISPLAY    UpdateOverlayDisplay;
    IDDSUPDATEOVERLAYZORDER4    UpdateOverlayZOrder;
    IDDSGETDDINTERFACE2         GetDDInterface;
    IDDSPAGELOCK2               PageLock;
    IDDSPAGEUNLOCK2             PageUnlock;
    IDDSSETSURFACEDESC4         SetSurfaceDesc;
    IDDSSETPRIVATEDATA4         SetPrivateData;
    IDDSGETPRIVATEDATA4         GetPrivateData;
    IDDSFREEPRIVATEDATA4        FreePrivateData;
    IDDSGETUNIQUENESSVALUE4     GetUniquenessValue;
    IDDSCHANGEUNIQUENESSVALUE4  ChangeUniquenessValue;
} idds4_vft;

const static idds4_vft idds4_self = {
    idds_query_interface,
    idds_add_ref,
    idds_remove_ref,
    idds_add_attached_surface4,
    idds_add_overlay_dirty_rect,
    idds_blt4,
    idds_blt_batch,
    idds_blt_fast4,
    idds_delete_attached_surface4,
    idds_enum_attached_surfaces,
    idds_enum_overlay_z_orders,
    idds_flip4,
    idds_get_attached_surface4,
    idds_get_blt_status,
    idds_get_caps4,
    idds_get_clipper,
    idds_get_color_key,
    idds_get_dc,
    idds_get_flip_status,
    idds_get_overlay_position,
    idds_get_palette,
    idds_get_pixel_format,
    idds_get_surface_desc4,
    idds_initialize4,
    idds_is_lost,
    idds_lock4,
    idds_release_dc,
    idds_restore,
    idds_set_clipper,
    idds_set_color_key,
    idds_set_overlay_position,
    idds_set_palette,
    idds_unlock,
    idds_update_overlay4,
    idds_update_overlay_display,
    idds_update_overlay_z_order4,
    idds_get_dd_interface2,
    idds_page_lock2,
    idds_page_unlock2,
    idds_set_surface_desc4,
    idds_set_private_data4,
    idds_get_private_data4,
    idds_free_private_data4,
    idds_get_uniqueness_value4,
    idds_change_uniqueness_value4
};

typedef struct idds7_vft {
    IDDSQUERYINTERFACE          QueryInterface;
    IDDSADDREF                  AddRef;
    IDDSRELEASE                 Release;
    IDDSADDATTACHEDSURFACE7     AddAttachedSurface;
    IDDSADDOVERLAYDIRTYRECT     AddOverlayDirtyRect;
    IDDSBLT7                    Blt;
    IDDSBLTBATCH                BltBatch;
    IDDSBLTFAST7                BltFast;
    IDDSDELETEATTACHEDSURFACE7  DeleteAttachedSurface;
    IDDSENUMATTACHEDSURFACES    EnumAttachedSurfaces;
    IDDSENUMOVERLAYZORDERS      EnumOverlayZOrders;
    IDDSFLIP7                   Flip;
    IDDSGETATTACHEDSURFACE7     GetAttachedSurface;
    IDDSGETBLTSTATUS            GetBltStatus;
    IDDSGETCAPS4                GetCaps;
    IDDSGETCLIPPER              GetClipper;
    IDDSGETCOLORKEY             GetColorKey;
    IDDSGETDC                   GetDC;
    IDDSGETFLIPSTATUS           GetFlipStatus;
    IDDSGETOVERLAYPOSITION      GetOverlayPosition;
    IDDSGETPALETTE              GetPalette;
    IDDSGETPIXELFORMAT          GetPixelFormat;
    IDDSGETSURFACEDESC4         GetSurfaceDesc;
    IDDSINITIALIZE4             Initialize;
    IDDSISLOST                  IsLost;
    IDDSLOCK4                   Lock;
    IDDSRELEASEDC               ReleaseDC;
    IDDSRESTORE                 Restore;
    IDDSSETCLIPPER              SetClipper;
    IDDSSETCOLORKEY             SetColorKey;
    IDDSSETOVERLAYPOSITION      SetOverlayPosition;
    IDDSSETPALETTE              SetPalette;
    IDDSUNLOCK                  Unlock;
    IDDSUPDATEOVERLAY7          UpdateOverlay;
    IDDSUPDATEOVERLAYDISPLAY    UpdateOverlayDisplay;
    IDDSUPDATEOVERLAYZORDER7    UpdateOverlayZOrder;
    IDDSGETDDINTERFACE2         GetDDInterface;
    IDDSPAGELOCK2               PageLock;
    IDDSPAGEUNLOCK2             PageUnlock;
    IDDSSETSURFACEDESC4         SetSurfaceDesc;
    IDDSSETPRIVATEDATA4         SetPrivateData;
    IDDSGETPRIVATEDATA4         GetPrivateData;
    IDDSFREEPRIVATEDATA4        FreePrivateData;
    IDDSGETUNIQUENESSVALUE4     GetUniquenessValue;
    IDDSCHANGEUNIQUENESSVALUE4  ChangeUniquenessValue;
    IDDSSETPRIORITY7            SetPriority;
    IDDSGETPRIORITY7            GetPriority;
    IDDSSETLOD7                 SetLOD;
    IDDSGETLOD7                 GetLOD;
} idds7_vft;

const static idds7_vft idds7_self = {
    idds_query_interface,
    idds_add_ref,
    idds_remove_ref,
    idds_add_attached_surface7,
    idds_add_overlay_dirty_rect,
    idds_blt7,
    idds_blt_batch,
    idds_blt_fast7,
    idds_delete_attached_surface7,
    idds_enum_attached_surfaces,
    idds_enum_overlay_z_orders,
    idds_flip7,
    idds_get_attached_surface7,
    idds_get_blt_status,
    idds_get_caps4,
    idds_get_clipper,
    idds_get_color_key,
    idds_get_dc,
    idds_get_flip_status,
    idds_get_overlay_position,
    idds_get_palette,
    idds_get_pixel_format,
    idds_get_surface_desc4,
    idds_initialize4,
    idds_is_lost,
    idds_lock4,
    idds_release_dc,
    idds_restore,
    idds_set_clipper,
    idds_set_color_key,
    idds_set_overlay_position,
    idds_set_palette,
    idds_unlock,
    idds_update_overlay7,
    idds_update_overlay_display,
    idds_update_overlay_z_order7,
    idds_get_dd_interface2,
    idds_page_lock2,
    idds_page_unlock2,
    idds_set_surface_desc4,
    idds_set_private_data4,
    idds_get_private_data4,
    idds_free_private_data4,
    idds_get_uniqueness_value4,
    idds_change_uniqueness_value4,
    idds_set_priority7,
    idds_get_priority7,
    idds_set_lod7,
    idds_get_lod7
};

HRESULT SUGARCALL idds_create(sugar* manager, const GUID* riid, idds** object) {
    if (manager == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (riid == NULL || object == NULL) {
        LOGLEAVE(manager->logger, DDERR_INVALIDPARAMS);
    }

    if (!IsEqualGUID(&IID_IUnknown, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface2, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface3, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface4, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface7, riid)) {
        LOGLEAVE(manager->logger, E_NOINTERFACE);
    }

    HRESULT hr = DD_OK;
    idds* instance = NULL;
    if (SUCCEEDED(hr = allocator_allocate(manager->allocator, MEM_TAG_IDIRECTDRAWSURFACE, sizeof(idds), &instance))) {
        if (IsEqualGUID(&IID_IUnknown, riid)) {
            instance->self = &iddsu_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurface, riid)) {
            instance->self = &idds1_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurface2, riid)) {
            instance->self = &idds2_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurface3, riid)) {
            instance->self = &idds3_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurface4, riid)) {
            instance->self = &idds4_self;
        }
        else if (IsEqualGUID(&IID_IDirectDrawSurface7, riid)) {
            instance->self = &idds7_self;
        }

        instance->manager = manager;
        instance->logger = manager->logger;
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    LOGLEAVESELF(manager->logger, hr, instance);
}

void SUGARCALL idds_release(idds* self) {
    if (self != NULL) {
        allocator_free(self->manager->allocator, self);
    }
}

HRESULT SUGARCALL idds_query_interface(idds* self, const GUID* riid, void** object) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (riid == NULL || object == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_query_interface(self->instance, riid, object));
}

ULONG SUGARCALL idds_add_ref(idds* self) {
    if (self == NULL) {
        return 0;
    }

    return InterlockedIncrement(&self->refs);
}

ULONG SUGARCALL idds_remove_ref(idds* self) {
    if (self == NULL || self->refs == 0) {
        return 0;
    }

    s32 result = InterlockedDecrement(&self->refs);

    if ((result = max(result, 0)) == 0) {
        self->refs = 0;

        if (self->instance != NULL) {
            dds_remove_ref(self->instance, self);
        }

        idds_release(self);
    }

    return result;
}

HRESULT SUGARCALL idds_add_attached_surface1(idds* self, LPDIRECTDRAWSURFACE lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpDDSAttachedSurface);

    if (lpDDSAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_add_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_add_overlay_dirty_rect(idds* self, LPRECT lpRect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s", rect_to_string(lpRect));

    if (lpRect == NULL) {
        LEAVE(DDERR_INVALIDPARAMS); // TODO can rect be NULL?
    }

    LEAVE(dds_add_overlay_dirty_rect(self->instance, lpRect));
}

HRESULT SUGARCALL idds_blt1(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO proper implementation

    LEAVE(dds_blt(self->instance, lpDestRect,
        ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwFlags, lpDDBltFx));
}

HRESULT SUGARCALL idds_blt_batch(idds* self, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO proper implementation

    LEAVE(dds_blt_batch(self->instance, lpDDBltBatch, dwCount, dwFlags));
}

HRESULT SUGARCALL idds_blt_fast1(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSrcSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_blt_fast(self->instance, dwX, dwY,
        ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwTrans));
}

HRESULT SUGARCALL idds_delete_attached_surface1(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_delete_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_enum_attached_surfaces(idds* self, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%p", lpContext, lpEnumSurfacesCallback);

    // TODO

    return dds_enum_attached_surfaces(self->instance);
}

HRESULT SUGARCALL idds_enum_overlay_z_orders(idds* self, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO

    LEAVE(dds_enum_overlay_z_orders(self->instance));
}

HRESULT SUGARCALL idds_flip1(idds* self, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags));
}

HRESULT SUGARCALL idds_get_attached_surface1(idds* self, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;

    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(DDSCAPS2));
    CopyMemory(&caps, lpDDSCaps, sizeof(DDSCAPS));

    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, &caps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface, lplpDDAttachedSurface);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_get_blt_status(idds* self, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    LEAVE(dds_get_blt_status(self->instance, dwFlags));
}

HRESULT SUGARCALL idds_get_caps1(idds* self, LPDDSCAPS lpDDSCaps) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;

    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(DDSCAPS2));

    if (SUCCEEDED(hr = dds_get_caps(self->instance, &caps))) {
        CopyMemory(lpDDSCaps, &caps, sizeof(DDSCAPS));
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_get_clipper(idds* self, LPDIRECTDRAWCLIPPER* lplpDDClipper) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lplpDDClipper);

    if (lplpDDClipper == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;

    iddcconn connector;
    ZeroMemory(&connector, sizeof(iddcconn));

    if (SUCCEEDED(hr = dds_get_clipper(self->instance, &connector))) {
        hr = ddc_query_interface(connector.instance, &connector.id, lplpDDClipper);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_get_color_key(idds* self, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if ((dwFlags & ~DDCKEY_VALID) || lpDDColorKey == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_get_color_key(self->instance, dwFlags, lpDDColorKey));
}

HRESULT SUGARCALL idds_get_dc(idds* self, HDC* lphDC) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lphDC);

    if (lphDC == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_get_dc(self->instance, lphDC));
}

HRESULT SUGARCALL idds_get_flip_status(idds* self, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO check dwFlags

    LEAVE(dds_get_flip_status(self->instance, dwFlags));
}

HRESULT SUGARCALL idds_get_overlay_position(idds* self, LPLONG lplX, LPLONG lplY) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p, 0x%p", lplX, lplY);

    if (lplX == NULL || lplY == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_get_overlay_position(self->instance, lplX, lplY));
}

HRESULT SUGARCALL idds_get_palette(idds* self, LPDIRECTDRAWPALETTE* lplpDDPalette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lplpDDPalette);

    if (lplpDDPalette == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;

    iddpconn connector;
    ZeroMemory(&connector, sizeof(iddpconn));

    if (SUCCEEDED(hr = dds_get_palette(self->instance, &connector))) {
        hr = ddp_query_interface(connector.instance, &connector.id, lplpDDPalette);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_get_pixel_format(idds* self, LPDDPIXELFORMAT lpDDPixelFormat) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDPixelFormat == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDPixelFormat->dwSize != sizeof(DDPIXELFORMAT)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;

    DDPIXELFORMAT format;
    ZeroMemory(&format, sizeof(DDPIXELFORMAT));
    format.dwSize = sizeof(DDPIXELFORMAT);

    if (SUCCEEDED(hr = dds_get_pixel_format(self->instance, lpDDPixelFormat))) {
        CopyMemory(lpDDPixelFormat, &format, sizeof(DDPIXELFORMAT));
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_get_surface_desc1(idds* self, LPDDSURFACEDESC lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSurfaceDesc == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
    desc.dwSize = sizeof(DDSURFACEDESC2);
    desc.dwFlags = lpDDSurfaceDesc->dwFlags;

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = dds_get_surface_desc(self->instance, &desc))) {
        CopyMemory(lpDDSurfaceDesc, &desc, sizeof(DDSURFACEDESC));
        lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_initialize1(idds* self, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDD == NULL || lpDDSurfaceDesc == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
    CopyMemory(&desc, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
    desc.dwSize = sizeof(DDSURFACEDESC2);

    LEAVE(dds_initialize(self->instance, ((idd*)lpDD)->instance, &desc));
}

HRESULT SUGARCALL idds_is_lost(idds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_is_lost(self->instance));
}

HRESULT SUGARCALL idds_lock1(idds* self, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s, %s, %s, 0x%08X", rect_to_string(lpDestRect),
        ddsurfacedesc1_to_string(lpDDSurfaceDesc), ddlock_to_string(dwFlags), hEvent);

    if (lpDDSurfaceDesc == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (dwFlags & ~DDLOCK_VALID) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    // TODO Early versions suport event, see documentation.
    // For now assume DirectDraw 6+ behavior...
    if ((dwFlags & DDLOCK_EVENT) || hEvent != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
    CopyMemory(&desc, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
    desc.dwSize = sizeof(DDSURFACEDESC2);

    if (SUCCEEDED(hr = dds_lock(self->instance, lpDestRect, &desc, dwFlags))) {
        CopyMemory(lpDDSurfaceDesc, &desc, sizeof(DDSURFACEDESC));
        lpDDSurfaceDesc->dwSize = sizeof(DDSURFACEDESC);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_release_dc(idds* self, HDC hDC) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", hDC);

    if (hDC == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_release_dc(self->instance, hDC));
}

HRESULT SUGARCALL idds_restore(idds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_restore(self->instance));
}

HRESULT SUGARCALL idds_set_clipper(idds* self, LPDIRECTDRAWCLIPPER lpDDClipper) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpDDClipper);

    iddcconn connector;
    ZeroMemory(&connector, sizeof(iddcconn));

    if (lpDDClipper != NULL) {
        iddc* clipper = (iddc*)lpDDClipper;
        connector.instance = clipper->instance;
        CopyMemory(&connector.id, &clipper->id, sizeof(GUID));
    }

    LEAVE(dds_set_clipper(self->instance, &connector));
}

HRESULT SUGARCALL idds_set_color_key(idds* self, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (dwFlags & ~DDCKEY_VALID) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    return dds_set_color_key(self->instance, dwFlags, lpDDColorKey);
}

HRESULT SUGARCALL idds_set_overlay_position(idds* self, LONG lX, LONG lY) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%d, %d", lX, lY);

    LEAVE(dds_set_overlay_position(self->instance, lX, lY));
}

HRESULT SUGARCALL idds_set_palette(idds* self, LPDIRECTDRAWPALETTE lpDDPalette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("0x%p", lpDDPalette);

    iddpconn connector;
    ZeroMemory(&connector, sizeof(iddpconn));

    if (lpDDPalette != NULL) {
        iddp* palette = (iddp*)lpDDPalette;
        connector.instance = palette->instance;
        CopyMemory(&connector.id, &palette->id, sizeof(GUID));
    }

    LEAVE(dds_set_palette(self->instance, &connector));
}

HRESULT SUGARCALL idds_unlock(idds* self, LPRECT lpRect) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    ENTER("%s", rect_to_string(lpRect));

    LEAVE(dds_unlock(self->instance, lpRect));
}

HRESULT SUGARCALL idds_update_overlay1(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDDestSurface == NULL || lpDestRect == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags == DDOVER_NONE) || (dwFlags & ~DDOVER_VALID)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_HIDE) && (dwFlags & DDOVER_SHOW)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_DDFX) && lpDDOverlayFx == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDOverlayFx != NULL) {
        if (lpDDOverlayFx->dwSize != sizeof(DDOVERLAYFX)) {
            LEAVE(DDERR_INVALIDPARAMS);
        }
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDDestSurface != NULL) {
        idds* surface = (idds*)lpDDDestSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay(self->instance, &self->id,
        lpSrcRect, &connector, lpDestRect, dwFlags, lpDDOverlayFx));
}

HRESULT SUGARCALL idds_update_overlay_display(idds* self, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    LEAVE(dds_update_overlay_display(self->instance));
}

HRESULT SUGARCALL idds_update_overlay_z_order1(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSReference != NULL) {
        idds* surface = (idds*)lpDDSReference;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay_z_order(self->instance, dwFlags, &connector));
}

HRESULT SUGARCALL idds_add_attached_surface2(idds* self, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_add_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_blt2(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO proper implementation

    LEAVE(dds_blt(self->instance, lpDestRect, ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwFlags, lpDDBltFx));
}

HRESULT SUGARCALL idds_blt_fast2(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSrcSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_blt_fast(self->instance, dwX, dwY, ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwTrans));
}

HRESULT SUGARCALL idds_delete_attached_surface2(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_delete_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_flip2(idds* self, LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags));
}

HRESULT SUGARCALL idds_get_attached_surface2(idds* self, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;

    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(DDSCAPS2));
    CopyMemory(&caps, lpDDSCaps, sizeof(DDSCAPS));

    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, &caps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface2, lplpDDAttachedSurface);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_update_overlay2(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDDestSurface == NULL || lpDestRect == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags == DDOVER_NONE) || (dwFlags & ~DDOVER_VALID)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_HIDE) && (dwFlags & DDOVER_SHOW)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_DDFX) && lpDDOverlayFx == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDOverlayFx != NULL) {
        if (lpDDOverlayFx->dwSize != sizeof(DDOVERLAYFX)) {
            LEAVE(DDERR_INVALIDPARAMS);
        }
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDDestSurface != NULL) {
        idds* surface = (idds*)lpDDDestSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay(self->instance, &self->id,
        lpSrcRect, &connector, lpDestRect, dwFlags, lpDDOverlayFx));
}

HRESULT SUGARCALL idds_update_overlay_z_order2(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSReference != NULL) {
        idds* surface = (idds*)lpDDSReference;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay_z_order(self->instance, dwFlags, &connector));
}

HRESULT SUGARCALL idds_get_dd_interface2(idds* self, LPVOID* lplpDD) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lplpDD == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_get_dd_interface(self->instance, lplpDD));
}

HRESULT SUGARCALL idds_page_lock2(idds* self, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    LEAVE(dds_page_lock(self->instance));
}

HRESULT SUGARCALL idds_page_unlock2(idds* self, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    LEAVE(dds_page_unlock(self->instance));
}

HRESULT SUGARCALL idds_add_attached_surface3(idds* self, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_add_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_blt3(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO proper implementation

    LEAVE(dds_blt(self->instance, lpDestRect, ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwFlags, lpDDBltFx));
}

HRESULT SUGARCALL idds_blt_fast3(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSrcSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_blt_fast(self->instance, dwX, dwY, ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwTrans));
}

HRESULT SUGARCALL idds_delete_attached_surface3(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_delete_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_flip3(idds* self, LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags));
}

HRESULT SUGARCALL idds_get_attached_surface3(idds* self, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;

    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(DDSCAPS2));
    CopyMemory(&caps, lpDDSCaps, sizeof(DDSCAPS));

    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, &caps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface3, lplpDDAttachedSurface);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_update_overlay3(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDDestSurface == NULL || lpDestRect == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags == DDOVER_NONE) || (dwFlags & ~DDOVER_VALID)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_HIDE) && (dwFlags & DDOVER_SHOW)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_DDFX) && lpDDOverlayFx == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDOverlayFx != NULL) {
        if (lpDDOverlayFx->dwSize != sizeof(DDOVERLAYFX)) {
            LEAVE(DDERR_INVALIDPARAMS);
        }
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDDestSurface != NULL) {
        idds* surface = (idds*)lpDDDestSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay(self->instance, &self->id,
        lpSrcRect, &connector, lpDestRect, dwFlags, lpDDOverlayFx));
}

HRESULT SUGARCALL idds_update_overlay_z_order3(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSReference != NULL) {
        idds* surface = (idds*)lpDDSReference;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay_z_order(self->instance, dwFlags, &connector));
}

HRESULT SUGARCALL idds_set_surface_desc3(idds* self, LPDDSURFACEDESC lpDDSD, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSD == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSD->dwSize != sizeof(DDSURFACEDESC)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    // TODO verify dwFlags

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
    CopyMemory(&desc, lpDDSD, sizeof(DDSURFACEDESC));
    desc.dwSize = sizeof(DDSURFACEDESC2);

    LEAVE(dds_set_surface_desc(self->instance, &desc));
}

HRESULT SUGARCALL idds_add_attached_surface4(idds* self, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_add_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_blt4(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO proper implementation

    LEAVE(dds_blt(self->instance, lpDestRect, ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwFlags, lpDDBltFx));
}

HRESULT SUGARCALL idds_blt_fast4(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSrcSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_blt_fast(self->instance, dwX, dwY, ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwTrans));
}

HRESULT SUGARCALL idds_delete_attached_surface4(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_delete_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_flip4(idds* self, LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags));
}

HRESULT SUGARCALL idds_get_attached_surface4(idds* self, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE4* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;
    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, lpDDSCaps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface4, lplpDDAttachedSurface);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_get_caps4(idds* self, LPDDSCAPS2 lpDDSCaps) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;

    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(DDSCAPS2));

    if (SUCCEEDED(hr = dds_get_caps(self->instance, &caps))) {
        CopyMemory(lpDDSCaps, &caps, sizeof(DDSCAPS2));
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_get_surface_desc4(idds* self, LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSurfaceDesc == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC2)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_get_surface_desc(self->instance, lpDDSurfaceDesc));
}

HRESULT SUGARCALL idds_initialize4(idds* self, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDD == NULL || lpDDSurfaceDesc == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    DDSURFACEDESC2 desc;
    CopyMemory(&desc, lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));

    LEAVE(dds_initialize(self->instance, ((idd*)lpDD)->instance, &desc));
}

HRESULT SUGARCALL idds_lock4(idds* self, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSurfaceDesc == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC2)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (dwFlags & ~DDLOCK_VALID) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDLOCK_EVENT) || hEvent != NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    // In IDirectDrawSurface7, the default behavior of this method is to wait
    // for the accelator to finish. Therefore, under default conditions,
    // this method never returns DDERR_WASSTILLDRAWING.
    // If you want to see the error codes and not wait until
    // the blit operation succeeds, use the DDLOCK_DONOTWAIT flag.
    if (IsEqualGUID(&IID_IDirectDrawSurface7, &self->id)) {
        if ((!dwFlags & DDLOCK_DONOTWAIT)) {
            dwFlags |= DDLOCK_WAIT;
        }
    }

    DDSURFACEDESC2 desc;
    CopyMemory(&desc, lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));

    HRESULT hr = DD_OK;
    if (SUCCEEDED(hr = dds_lock(self->instance, lpDestRect, &desc, dwFlags))) {
        CopyMemory(lpDDSurfaceDesc, &desc, sizeof(DDSURFACEDESC2));
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_update_overlay4(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDDestSurface == NULL || lpDestRect == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags == DDOVER_NONE) || (dwFlags & ~DDOVER_VALID)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_HIDE) && (dwFlags & DDOVER_SHOW)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_DDFX) && lpDDOverlayFx == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDOverlayFx != NULL) {
        if (lpDDOverlayFx->dwSize != sizeof(DDOVERLAYFX)) {
            LEAVE(DDERR_INVALIDPARAMS);
        }
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDDestSurface != NULL) {
        idds* surface = (idds*)lpDDDestSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay(self->instance, &self->id,
        lpSrcRect, &connector, lpDestRect, dwFlags, lpDDOverlayFx));
}

HRESULT SUGARCALL idds_update_overlay_z_order4(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSReference != NULL) {
        idds* surface = (idds*)lpDDSReference;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay_z_order(self->instance, dwFlags, &connector));
}

HRESULT SUGARCALL idds_set_surface_desc4(idds* self, LPDDSURFACEDESC2 lpDDSD, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSD == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDSD->dwSize != sizeof(DDSURFACEDESC)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    // TODO verify dwFlags

    DDSURFACEDESC2 desc;
    CopyMemory(&desc, lpDDSD, sizeof(DDSURFACEDESC2));

    LEAVE(dds_set_surface_desc(self->instance, &desc));
}

HRESULT SUGARCALL idds_set_private_data4(idds* self, REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (guidTag == NULL || lpData == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    // TODO validate cbSize
    // TODO validate dwFlags

    LEAVE(dds_set_private_data(self->instance, guidTag, lpData, cbSize, dwFlags));
}

HRESULT SUGARCALL idds_get_private_data4(idds* self, REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (guidTag == NULL || lpcbBufferSize == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_get_private_data(self->instance, guidTag, lpBuffer, lpcbBufferSize));
}

HRESULT SUGARCALL idds_free_private_data4(idds* self, REFGUID guidTag) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (guidTag == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_free_private_data(self->instance, guidTag));
}

HRESULT SUGARCALL idds_get_uniqueness_value4(idds* self, LPDWORD lpValue) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpValue == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_get_uniqueness_value(self->instance, lpValue));
}

HRESULT SUGARCALL idds_change_uniqueness_value4(idds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_change_uniqueness_value(self->instance));
}

HRESULT SUGARCALL idds_add_attached_surface7(idds* self, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_add_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_blt7(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO proper implementation

    LEAVE(dds_blt(self->instance, lpDestRect,
        lpDDSrcSurface == NULL ? NULL : ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwFlags, lpDDBltFx));
}

HRESULT SUGARCALL idds_blt_fast7(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSrcSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_blt_fast(self->instance, dwX, dwY, ((idds*)lpDDSrcSurface)->instance, lpSrcRect, dwTrans));
}

HRESULT SUGARCALL idds_delete_attached_surface7(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSAttachedSurface != NULL) {
        idds* surface = (idds*)lpDDSAttachedSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_delete_attached_surface(self->instance, &connector));
}

HRESULT SUGARCALL idds_flip7(idds* self, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags));
}

HRESULT SUGARCALL idds_get_attached_surface7(idds* self, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    HRESULT hr = DD_OK;
    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, lpDDSCaps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface7, lplpDDAttachedSurface);
    }

    LEAVE(hr);
}

HRESULT SUGARCALL idds_update_overlay7(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDDestSurface == NULL || lpDestRect == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags == DDOVER_NONE) || (dwFlags & ~DDOVER_VALID)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_HIDE) && (dwFlags & DDOVER_SHOW)) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if ((dwFlags & DDOVER_DDFX) && lpDDOverlayFx == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    if (lpDDOverlayFx != NULL) {
        if (lpDDOverlayFx->dwSize != sizeof(DDOVERLAYFX)) {
            LEAVE(DDERR_INVALIDPARAMS);
        }
    }

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDDestSurface != NULL) {
        idds* surface = (idds*)lpDDDestSurface;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay(self->instance, &self->id,
        lpSrcRect, &connector, lpDestRect, dwFlags, lpDDOverlayFx));
}

HRESULT SUGARCALL idds_update_overlay_z_order7(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    // TODO validate dwFlags

    iddsconn connector;
    ZeroMemory(&connector, sizeof(iddsconn));

    if (lpDDSReference != NULL) {
        idds* surface = (idds*)lpDDSReference;
        connector.instance = surface->instance;
        CopyMemory(&connector.id, &surface->id, sizeof(GUID));
    }

    LEAVE(dds_update_overlay_z_order(self->instance, dwFlags, &connector));
}

HRESULT SUGARCALL idds_set_priority7(idds* self, DWORD dwPriority) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_set_priority(self->instance, dwPriority));
}

HRESULT SUGARCALL idds_get_priority7(idds* self, LPDWORD lpdwPriority) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpdwPriority == NULL) {
        LEAVE(DDERR_INVALIDPARAMS);
    }

    LEAVE(dds_get_priority(self->instance, lpdwPriority));
}

HRESULT SUGARCALL idds_set_lod7(idds* self, DWORD dwMaxLOD) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    LEAVE(dds_set_lod(self->instance, dwMaxLOD));
}

HRESULT SUGARCALL idds_get_lod7(idds* self, LPDWORD lpdwMaxLOD) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpdwMaxLOD == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    LEAVE(dds_get_lod(self->instance, lpdwMaxLOD));
}
