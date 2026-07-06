#include "idd.h"
#include "idds.h"
#include "dds.h"

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
    if (manager == NULL || riid == NULL || object == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (!IsEqualGUID(&IID_IUnknown, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface2, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface3, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface4, riid)
        && !IsEqualGUID(&IID_IDirectDrawSurface7, riid)) {
        return E_NOINTERFACE;
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
        CopyMemory(&instance->id, riid, sizeof(GUID));
        instance->refs = 1;

        *object = instance;
    }

    return hr;
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
        return DDERR_INVALIDPARAMS;
    }

    return dds_query_interface(self->instance, riid, object);
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
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_add_overlay_dirty_rect(idds* self, LPRECT lpRect) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt1(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt_batch(idds* self, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt_fast1(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_delete_attached_surface1(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_enum_attached_surfaces(idds* self, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_enum_overlay_z_orders(idds* self, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_flip1(idds* self, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    return dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags);
}

HRESULT SUGARCALL idds_get_attached_surface1(idds* self, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;

    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(DDSCAPS2));
    caps.dwCaps = lpDDSCaps->dwCaps;

    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, &caps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface, lplpDDAttachedSurface);
    }

    return hr;
}

HRESULT SUGARCALL idds_get_blt_status(idds* self, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_caps1(idds* self, LPDDSCAPS lpDDSCaps) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_clipper(idds* self, LPDIRECTDRAWCLIPPER* lplpDDClipper) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_color_key(idds* self, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_dc(idds* self, HDC* lphDC) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lphDC == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return dds_get_dc(self->instance, lphDC);
}

HRESULT SUGARCALL idds_get_flip_status(idds* self, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_overlay_position(idds* self, LPLONG lplX, LPLONG lplY) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_palette(idds* self, LPDIRECTDRAWPALETTE* lplpDDPalette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lplpDDPalette == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return dds_get_palette(self->instance, (iddp**)lplpDDPalette);
}

HRESULT SUGARCALL idds_get_pixel_format(idds* self, LPDDPIXELFORMAT lpDDPixelFormat) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_surface_desc1(idds* self, LPDDSURFACEDESC lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSurfaceDesc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) {
        return DDERR_INVALIDPARAMS;
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

    return hr;
}

HRESULT SUGARCALL idds_initialize1(idds* self, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDD == NULL || lpDDSurfaceDesc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    DDSURFACEDESC2 desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC2));
    CopyMemory(&desc, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
    desc.dwSize = sizeof(DDSURFACEDESC2);

    return dds_initialize(self->instance, ((idd*)lpDD)->instance, &desc);
}

HRESULT SUGARCALL idds_is_lost(idds* self) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_lock1(idds* self, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_release_dc(idds* self, HDC hDC) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (hDC == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    return dds_release_dc(self->instance, hDC);
}

HRESULT SUGARCALL idds_restore(idds* self) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    return DD_OK;
}

HRESULT SUGARCALL idds_set_clipper(idds* self, LPDIRECTDRAWCLIPPER lpDDClipper) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_set_color_key(idds* self, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_set_overlay_position(idds* self, LONG lX, LONG lY) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_set_palette(idds* self, LPDIRECTDRAWPALETTE lpDDPalette) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    return dds_set_palette(self->instance, (iddp*)lpDDPalette);
}

HRESULT SUGARCALL idds_unlock(idds* self, LPRECT lpRect) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_update_overlay1(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_update_overlay_display(idds* self, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_update_overlay_z_order1(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_add_attached_surface2(idds* self, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt2(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt_fast2(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_delete_attached_surface2(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_flip2(idds* self, LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    return dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags);
}

HRESULT SUGARCALL idds_get_attached_surface2(idds* self, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;

    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(DDSCAPS2));
    caps.dwCaps = lpDDSCaps->dwCaps;

    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, &caps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface2, lplpDDAttachedSurface);
    }

    return hr;
}

HRESULT SUGARCALL idds_update_overlay2(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_update_overlay_z_order2(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_dd_interface2(idds* self, LPVOID* lplpDD) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_page_lock2(idds* self, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_page_unlock2(idds* self, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_add_attached_surface3(idds* self, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt3(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt_fast3(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_delete_attached_surface3(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_flip3(idds* self, LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    return dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags);
}

HRESULT SUGARCALL idds_get_attached_surface3(idds* self, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;

    DDSCAPS2 caps;
    ZeroMemory(&caps, sizeof(DDSCAPS2));
    caps.dwCaps = lpDDSCaps->dwCaps;

    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, &caps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface3, lplpDDAttachedSurface);
    }

    return hr;
}

HRESULT SUGARCALL idds_update_overlay3(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_update_overlay_z_order3(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_set_surface_desc3(idds* self, LPDDSURFACEDESC lpDDSD, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_add_attached_surface4(idds* self, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt4(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt_fast4(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_delete_attached_surface4(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_flip4(idds* self, LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    return dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags);
}

HRESULT SUGARCALL idds_get_attached_surface4(idds* self, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE4* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, lpDDSCaps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface4, lplpDDAttachedSurface);
    }

    return hr;
}

HRESULT SUGARCALL idds_get_caps4(idds* self, LPDDSCAPS2 lpDDSCaps) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_surface_desc4(idds* self, LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSurfaceDesc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC2)) {
        return DDERR_INVALIDPARAMS;
    }

    return dds_get_surface_desc(self->instance, lpDDSurfaceDesc);
}

HRESULT SUGARCALL idds_initialize4(idds* self, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDD == NULL || lpDDSurfaceDesc == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    DDSURFACEDESC2 desc;
    CopyMemory(&desc, lpDDSurfaceDesc, sizeof(DDSURFACEDESC2));
    
    return dds_initialize(self->instance, ((idd*)lpDD)->instance, &desc);
}

HRESULT SUGARCALL idds_lock4(idds* self, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_update_overlay4(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_update_overlay_z_order4(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_set_surface_desc4(idds* self, LPDDSURFACEDESC2 lpDDSD, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_set_private_data4(idds* self, REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_private_data4(idds* self, REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_free_private_data4(idds* self, REFGUID guidTag) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_uniqueness_value4(idds* self, LPDWORD lpValue) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_change_uniqueness_value4(idds* self) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_add_attached_surface7(idds* self, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt7(idds* self, LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_blt_fast7(idds* self, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_delete_attached_surface7(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_flip7(idds* self, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    return dds_flip(self->instance,
        lpDDSurfaceTargetOverride == NULL ? NULL : ((idds*)lpDDSurfaceTargetOverride)->instance, dwFlags);
}

HRESULT SUGARCALL idds_get_attached_surface7(idds* self, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7* lplpDDAttachedSurface) {
    if (self == NULL) {
        return DDERR_INVALIDOBJECT;
    }

    if (lpDDSCaps == NULL || lplpDDAttachedSurface == NULL) {
        return DDERR_INVALIDPARAMS;
    }

    HRESULT hr = DD_OK;
    dds* instance = NULL;
    if (SUCCEEDED(hr = dds_get_attached_surface(self->instance, lpDDSCaps, &instance))) {
        hr = dds_query_interface(instance, &IID_IDirectDrawSurface7, lplpDDAttachedSurface);
    }

    return hr;
}

HRESULT SUGARCALL idds_update_overlay7(idds* self, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_update_overlay_z_order7(idds* self, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_set_priority7(idds* self, DWORD dwPriority) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_priority7(idds* self, LPDWORD lpdwPriority) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_set_lod7(idds* self, DWORD dwMaxLOD) {
    return DDERR_UNSUPPORTED;
}

HRESULT SUGARCALL idds_get_lod7(idds* self, LPDWORD lpdwMaxLOD) {
    return DDERR_UNSUPPORTED;
}
