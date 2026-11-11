/* NovaDock: Launcher icons: loading, scaling and reuse across reloads. */
#include "dock_internal.h"

/* Defined below, private to this file. */
static int icon_source(const struct NDDock *dock,const struct NDConfig *cfg, int index,char *path);
static ULONG icon_fingerprint(const struct NDDock *dock,const struct NDConfig *cfg, int index);
static int adopt_icon_source(struct NDDock *dock,struct NDIconStash *stash, const struct NDConfig *previous,int index);
static int icon_mask_pixel(const UBYTE *mask,int width,int x,int y);
static Object *open_datatype_picture(struct NDDock *dock,const char *path, struct BitMap **bitmap, struct BitMap **mask,int *width,int *height, struct ColorRegister **palette, int *num_colors);
static int build_datatype_icon(struct NDDock *dock,int index, int max_width,int max_height);
static void load_item_source(struct NDDock *dock,int i);
static void build_one_scaled_icon(struct NDDock *dock,int i,int max_width,int max_height);
static int render_icon(struct NDDock *dock,int i,int max_width,
                       int max_height,struct BitMap **out_bitmap,
                       struct BitMap **out_mask,int *out_width,int *out_height);
static void build_one_brush(struct NDDock *dock,int i,int max_width,int max_height);

/* Where item INDEX of CFG takes its artwork from.  Returns 0 for no
 * artwork, 1 with the resolved file in PATH, or 2 for the default tool icon
 * of a running-task tile.  Shared by loading and by the reload cache, so
 * both always agree on what "the same icon" means. */
static int icon_source(const struct NDDock *dock,const struct NDConfig *cfg,
                       int index,char *path)
{
    const struct NDItem *item=&cfg->items[index];
    const char *p=nd_item_text(cfg,item,ND_TEXT_ICON);
    size_t len;
    int dopus_icon=item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"DOPUS4");
    int music_icon=item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"MUSIC");
    int running_apps_icon=item->kind==ND_ITEM_PLUGIN &&
                          (!strcmp(item->id,"BITSKIM") || !strcmp(item->id,"RUNNINGAPPS") || !strcmp(item->id,"CLIPBOARD") || !strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW"));
    path[0]='\0';
    /* DOpus is deliberately drawn like a launcher. Use the icon beside its
     * configured executable, so changing Program path in the plugin
     * settings also changes the artwork without another icon setting. */
    if (dopus_icon) p=nd_option_get(cfg,"DOPUS4","PROGRAM");
    else if (music_icon || running_apps_icon) p=item->command;
    if (item->transient==ND_TRANSIENT_TASK) return 2;
    if (((item->kind != ND_ITEM_LAUNCH && item->kind != ND_ITEM_DRAWER &&
          item->kind != ND_ITEM_GROUP) &&
         !dopus_icon && !music_icon && !running_apps_icon) || !p || !*p)
        return 0;
    if (strchr(p, ':')) {
        strncpy(path, p, ND_ICON_PATH_LEN - 1);
        path[ND_ICON_PATH_LEN - 1] = '\0';
    } else {
        len = strlen(dock->directory);
        if (!len || len + strlen(p) + 2 >= ND_ICON_PATH_LEN) return 0;
        strcpy(path, dock->directory);
        if (path[len - 1] != ':' && path[len - 1] != '/') strcat(path, "/");
        strcat(path, p);
    }
    return 1;
}
/* Everything that decides which object load_icons() produces for an item:
 * its source, and the kind/volume flags that pick the default icon. */
static ULONG icon_fingerprint(const struct NDDock *dock,const struct NDConfig *cfg,
                              int index)
{
    static char path[ND_ICON_PATH_LEN];
    const struct NDItem *item=&cfg->items[index];
    const char *c;
    ULONG h=2166136261UL;
    int source=icon_source(dock,cfg,index,path);
    int volume=item->kind==ND_ITEM_DRAWER && item->command[0] &&
               item->command[strlen(item->command)-1]==':';
    h=(h^(ULONG)source)*16777619UL;
    h=(h^(ULONG)item->kind)*16777619UL;
    h=(h^(ULONG)volume)*16777619UL;
    h=(h^(ULONG)(item->kind==ND_ITEM_PLUGIN &&
                 (!strcmp(item->id,"MUSIC") || !strcmp(item->id,"DOPUS4"))))*16777619UL;
    for (c=path;*c;++c) h=(h^(UBYTE)*c)*16777619UL;
    return h;
}
/* Screen-independent artwork kept across a reload.  DiskObjects and decoded
 * IFF brushes are pure file data; only the pen-mapped bitmaps built from
 * them (and DataTypes pictures, which decode straight to the screen) must
 * be rebuilt.  Re-reading every .info from disk was most of a reload's
 * time on a 68000 - long enough for Preferences to report a timeout. */
void stash_icon_sources(struct NDDock *dock,struct NDIconStash *stash)
{
    int i;
    memset(stash,0,sizeof(*stash));
    for (i=0;i<dock->conf.count && i<ND_MAX_ITEMS;++i) {
        /* DataTypes items carry only a placeholder default icon. */
        if (dock->datatype_icon_paths[i][0]) continue;
        if (!dock->icons[i] && !dock->brushes[i]) continue;
        stash->icons[i]=dock->icons[i];dock->icons[i]=NULL;
        stash->brushes[i]=dock->brushes[i];dock->brushes[i]=NULL;
        stash->fingerprint[i]=icon_fingerprint(dock,&dock->conf,i);
        stash->usable[i]=1;
    }
}
/* Same Workbench screen and tile dimensions: move ownership of rendered
 * artwork too. The stash is heap allocated, not placed on the 68k stack. */
void stash_live_icon_sources(struct NDDock *dock,struct NDIconStash *stash)
{
    int i;
    stash_icon_sources(dock,stash);
    stash->live=1;
    for (i=0;i<dock->conf.count;++i) {
        if (dock->datatype_icon_paths[i][0]) {
            stash->icons[i]=dock->icons[i];dock->icons[i]=NULL;
            stash->fingerprint[i]=icon_fingerprint(dock,&dock->conf,i);
            stash->usable[i]=1;
        }
        stash->scaled[i]=dock->scaled_icon_bitmaps[i];
        stash->scaled_masks[i]=dock->scaled_icon_masks[i];
        stash->brush_maps[i]=dock->brush_bitmaps[i];
        stash->brush_masks[i]=dock->brush_masks[i];
        stash->widths[i]=dock->scaled_icon_width[i];
        stash->heights[i]=dock->scaled_icon_height[i];
        strcpy(stash->datatype_paths[i],dock->datatype_icon_paths[i]);
        dock->scaled_icon_bitmaps[i]=NULL;dock->scaled_icon_masks[i]=NULL;
        dock->brush_bitmaps[i]=NULL;dock->brush_masks[i]=NULL;
        dock->scaled_icon_width[i]=0;dock->scaled_icon_height[i]=0;
        dock->datatype_icon_paths[i][0]=0;
    }
}
static int adopt_icon_source(struct NDDock *dock,struct NDIconStash *stash,
                             const struct NDConfig *previous,int index)
{
    static char now_path[ND_ICON_PATH_LEN],then_path[ND_ICON_PATH_LEN];
    ULONG fingerprint;
    int j,source;
    if (!stash || !previous) return 0;
    fingerprint=icon_fingerprint(dock,&dock->conf,index);
    source=icon_source(dock,&dock->conf,index,now_path);
    for (j=0;j<previous->count && j<ND_MAX_ITEMS;++j) {
        if (!stash->usable[j] || stash->fingerprint[j]!=fingerprint) continue;
        /* Confirm a fingerprint match on the actual source. */
        if (icon_source(dock,previous,j,then_path)!=source ||
            strcmp(now_path,then_path) ||
            previous->items[j].kind!=dock->conf.items[index].kind) continue;
        dock->icons[index]=stash->icons[j];
        dock->brushes[index]=stash->brushes[j];
        if (stash->live) {
            dock->scaled_icon_bitmaps[index]=stash->scaled[j];stash->scaled[j]=NULL;
            dock->scaled_icon_masks[index]=stash->scaled_masks[j];stash->scaled_masks[j]=NULL;
            dock->brush_bitmaps[index]=stash->brush_maps[j];stash->brush_maps[j]=NULL;
            dock->brush_masks[index]=stash->brush_masks[j];stash->brush_masks[j]=NULL;
            dock->scaled_icon_width[index]=stash->widths[j];
            dock->scaled_icon_height[index]=stash->heights[j];
            strcpy(dock->datatype_icon_paths[index],stash->datatype_paths[j]);
        }
        stash->icons[j]=NULL;stash->brushes[j]=NULL;stash->usable[j]=0;
        return 1;
    }
    return 0;
}
void free_icon_stash(struct NDIconStash *stash)
{
    int i;
    if (GfxBase) WaitBlit();
    for (i=0;i<ND_MAX_ITEMS;++i) {
        if (stash->scaled[i]) FreeBitMap(stash->scaled[i]);
        if (stash->scaled_masks[i]) FreeBitMap(stash->scaled_masks[i]);
        if (stash->brush_maps[i]) FreeBitMap(stash->brush_maps[i]);
        if (stash->brush_masks[i]) FreeBitMap(stash->brush_masks[i]);
        stash->scaled[i]=NULL;stash->scaled_masks[i]=NULL;
        stash->brush_maps[i]=NULL;stash->brush_masks[i]=NULL;
        if (stash->icons[i] && IconBase) FreeDiskObject(stash->icons[i]);
        if (stash->brushes[i]) FreeVec(stash->brushes[i]);
        stash->icons[i]=NULL;stash->brushes[i]=NULL;stash->usable[i]=0;
    }
}
/* Read the artwork source of item I (icon, ILBM brush or DataTypes path).
 * The caller quietens requesters. */
static void load_item_source(struct NDDock *dock,int i)
{
    static char path[ND_ICON_PATH_LEN];
    int source;
    source=icon_source(dock,&dock->conf,i,path);
    if (source) ++dock->diagnostic_icons_loaded;
    if (source==2) {
        /* The program's own icon cannot be located safely for another
         * running process; use Workbench's default tool icon. */
        if (IconBase) dock->icons[i]=GetDefDiskObject(WBTOOL);
        return;
    }
    if (!source) return;
    if (has_iff_extension(path)) {
        dock->brushes[i] = (struct NDILBM *)AllocVec(sizeof(struct NDILBM),
                                         MEMF_PUBLIC | MEMF_CLEAR);
        if (dock->brushes[i] && !nd_ilbm_load(path, dock->brushes[i])) {
            FreeVec(dock->brushes[i]);
            dock->brushes[i] = NULL;
        }
    } else {
        int is_volume = dock->conf.items[i].kind == ND_ITEM_DRAWER &&
                        dock->conf.items[i].command[0] &&
                        dock->conf.items[i].command[
                          strlen(dock->conf.items[i].command)-1] == ':';
        int picture_path = !has_info_extension(path) &&
                           has_filename_extension(path);
        if (picture_path && DataTypesBase) {
            strncpy(dock->datatype_icon_paths[i],path,
                    sizeof(dock->datatype_icon_paths[i])-1);
            dock->datatype_icon_paths[i][
                sizeof(dock->datatype_icon_paths[i])-1]='\0';
        }
        if (!IconBase || picture_path) {
            /* Keep a Workbench default as a graceful fallback when the
             * named datatype is not installed on this machine. */
            if (picture_path && IconBase)
                dock->icons[i]=GetDefDiskObject(
                    (dock->conf.items[i].kind==ND_ITEM_DRAWER ||
                     dock->conf.items[i].kind==ND_ITEM_GROUP) ?
                    WBDRAWER : WBTOOL);
            return;
        }
        if (has_info_extension(path)) path[strlen(path) - 5] = '\0';
        /* GetDiskObjectNew() mirrors Workbench for objects without a
         * private .info by returning the proper system default icon.  A
         * volume root is special: its icon base is <volume>:Disk, which
         * is not itself a filesystem object, so request WBDISK directly
         * when Disk.info is absent. */
        if (IconBase->lib_Version >= 44) {
            struct TagItem load_tags[3];
            load_tags[0].ti_Tag = ICONGETA_GetPaletteMappedIcon;
            /* Keep the ColorIcon palette private and translate it into
             * the existing Workbench palette below.  Asking icon.library
             * for a palette-mapped icon retains pens until FreeDiskObject
             * and can starve applications which open their own screens. */
            load_tags[0].ti_Data = FALSE;
            load_tags[1].ti_Tag = ICONGETA_GenerateImageMasks;
            load_tags[1].ti_Data = TRUE;
            load_tags[2].ti_Tag = TAG_DONE;
            load_tags[2].ti_Data = 0;
            dock->icons[i] = GetIconTagList(path,load_tags);
        } else if (is_volume) {
            dock->icons[i] = GetDiskObject(path);
        } else if (IconBase->lib_Version >= 37) {
            dock->icons[i] = GetDiskObjectNew(path);
        } else {
            dock->icons[i] = GetDiskObject(path);
        }
        if (is_volume) {
            if (!dock->icons[i]) dock->icons[i] = GetDefDiskObject(WBDISK);
        } else if (!dock->icons[i])
            dock->icons[i] = GetDefDiskObject(
                (dock->conf.items[i].kind == ND_ITEM_DRAWER ||
                 dock->conf.items[i].kind == ND_ITEM_GROUP) ?
                WBDRAWER : WBTOOL);
    }
}
void load_icons_reusing(struct NDDock *dock,struct NDIconStash *stash,
                               const struct NDConfig *previous)
{
    int i;
    APTR requesters=quiet_requesters();
    dock->diagnostic_icons_reused=0;dock->diagnostic_icons_loaded=0;
    for (i = 0; i < dock->conf.count; ++i) {
        /* DataTypes and icon.library can be slow on a baseline 68k.  Keep
         * resident plugins acknowledged while a live reload rebuilds the
         * artwork, otherwise their bounded IPC wait expires and they exit. */
        if (dock->ipc) process_ipc(dock,0);
        if (adopt_icon_source(dock,stash,previous,i)) {
            ++dock->diagnostic_icons_reused;
            continue;
        }
        load_item_source(dock,i);
    }
    if (dock->ipc) process_ipc(dock,0);
    restore_requesters(requesters);
    /* Everything is loaded now, including any startup leftovers. */
    dock->icons_pending=0;
}
void load_icons(struct NDDock *dock)
{
    load_icons_reusing(dock,NULL,NULL);
}
static int icon_mask_pixel(const UBYTE *mask,int width,int x,int y)
{
    int row_bytes;
    if (!mask || width < 1 || x < 0 || y < 0) return 0;
    row_bytes = ((width + 15) >> 4) << 1;
    return (mask[y * row_bytes + (x >> 3)] & (0x80U >> (x & 7))) != 0;
}
int planar_icon_pixel(const struct Image *image,int x,int y)
{
    int bit,source_plane,row_words,plane_words,pen;
    UWORD word;
    if (!image || !image->ImageData || x < 0 || x >= image->Width ||
        y < 0 || y >= image->Height) return 0;
    row_words = (image->Width + 15) >> 4;
    plane_words = row_words * image->Height;
    source_plane = 0;
    pen = image->PlaneOnOff & ~image->PlanePick;
    for (bit=0; bit<8; ++bit) {
        if (!(image->PlanePick & (1U << bit))) continue;
        if (source_plane < image->Depth) {
            word = image->ImageData[source_plane * plane_words +
                                    y * row_words + (x >> 4)];
            if (word & (0x8000U >> (x & 15))) pen |= 1U << bit;
        }
        ++source_plane;
    }
    return pen;
}
/* Ask picture.datatype to decode artwork without remapping it to Workbench.
 * Remapping inside picture.datatype reserves shared palette pens and can
 * corrupt applications (notably DOpus 4) which open a shared-pen screen while
 * NovaDock is running. We translate the source palette with FindColor later.
 * The returned bitmap/palette remain owned by the datatype object; the mask is
 * copied because PDTA_MaskPlane is only a plane pointer. */
static Object *open_datatype_picture(struct NDDock *dock,const char *path,
                                     struct BitMap **bitmap,
                                     struct BitMap **mask,int *width,int *height,
                                     struct ColorRegister **palette,
                                     int *num_colors)
{
    Object *object;
    struct BitMapHeader *header=NULL;
    struct BitMap *dest=NULL,*mask_copy=NULL,source_mask;
    struct ColorRegister *registers=NULL;
    PLANEPTR mask_plane=NULL;
    ULONG colors=0;
    if (bitmap) *bitmap=NULL;
    if (mask) *mask=NULL;
    if (width) *width=0;
    if (height) *height=0;
    if (palette) *palette=NULL;
    if (num_colors) *num_colors=0;
    if (!dock || !dock->screen || !DataTypesBase || !path || !*path) return NULL;
    object=NewDTObject((STRPTR)path,
        DTA_SourceType,DTST_FILE,DTA_GroupID,GID_PICTURE,
        PDTA_Remap,FALSE,TAG_DONE);
    if (!object) return NULL;
    if (!DoDTMethod(object,NULL,NULL,DTM_PROCLAYOUT,NULL,TRUE) ||
        !GetDTAttrs(object,PDTA_BitMapHeader,(ULONG)&header,
                    PDTA_BitMap,(ULONG)&dest,
                    PDTA_ColorRegisters,(ULONG)&registers,
                    PDTA_NumColors,(ULONG)&colors,
                    PDTA_MaskPlane,(ULONG)&mask_plane,TAG_DONE) ||
        !header || !dest || !registers || colors<1 || colors>256 ||
        dest->Depth>8 || header->bmh_Width<1 || header->bmh_Height<1) {
        DisposeDTObject(object);
        return NULL;
    }
    if (mask_plane) {
        mask_copy=AllocBitMap(header->bmh_Width,header->bmh_Height,
                             1,BMF_CLEAR,NULL);
        if (mask_copy) {
            InitBitMap(&source_mask,1,header->bmh_Width,header->bmh_Height);
            source_mask.Planes[0]=mask_plane;
            BltBitMap(&source_mask,0,0,mask_copy,0,0,
                      header->bmh_Width,header->bmh_Height,0xC0,1,NULL);
            WaitBlit();
        }
    }
    if (bitmap) *bitmap=dest;
    if (mask) *mask=mask_copy;
    else if (mask_copy) FreeBitMap(mask_copy);
    if (width) *width=header->bmh_Width;
    if (height) *height=header->bmh_Height;
    if (palette) *palette=registers;
    if (num_colors) *num_colors=(int)colors;
    return object;
}
static int build_datatype_icon(struct NDDock *dock,int index,
                               int max_width,int max_height)
{
    Object *object;
    struct BitMap *source=NULL,*source_mask=NULL,*dest=NULL,*dest_mask=NULL;
    struct ColorRegister *palette=NULL;
    struct RastPort source_rp,source_mask_rp,dest_rp,dest_mask_rp;
    LONG mapped[256];
    int width,height,num_colors,fitted_width,fitted_height,x,y,sx,sy,opaque;
    int max_pen,j;
    LONG colour;
    if (!dock->datatype_icon_paths[index][0]) return 0;
    object=open_datatype_picture(dock,dock->datatype_icon_paths[index],
                                 &source,&source_mask,&width,&height,
                                 &palette,&num_colors);
    if (!object) return 0;
    nd_icon_fit(width,height,max_width,max_height,
                &fitted_width,&fitted_height);
    dest=AllocBitMap(fitted_width,fitted_height,
        dock->screen->RastPort.BitMap->Depth,BMF_CLEAR,
        dock->screen->RastPort.BitMap);
    dest_mask=AllocBitMap(fitted_width,fitted_height,1,BMF_CLEAR,NULL);
    if (!dest || !dest_mask) goto done;
    InitRastPort(&source_rp);source_rp.BitMap=source;
    InitRastPort(&dest_rp);dest_rp.BitMap=dest;
    InitRastPort(&dest_mask_rp);dest_mask_rp.BitMap=dest_mask;
    if (source_mask) {
        InitRastPort(&source_mask_rp);source_mask_rp.BitMap=source_mask;
    }
    max_pen = dock->screen->RastPort.BitMap->Depth >= 8 ? 255 :
              (1 << dock->screen->RastPort.BitMap->Depth) - 1;
    for (j=0;j<256;++j) mapped[j]=-1;
    for (j=0;j<num_colors;++j)
        mapped[j]=FindColor(dock->screen->ViewPort.ColorMap,
            RGBBYTE(palette[j].red),RGBBYTE(palette[j].green),
            RGBBYTE(palette[j].blue),max_pen);
    SetAPen(&dest_mask_rp,1);
    for (y=0;y<fitted_height;++y) {
        sy=(y*height)/fitted_height;
        for (x=0;x<fitted_width;++x) {
            sx=(x*width)/fitted_width;
            opaque=!source_mask || ReadPixel(&source_mask_rp,sx,sy)>0;
            if (!opaque) continue;
            colour=ReadPixel(&source_rp,sx,sy);
            if (colour<0 || colour>=num_colors || mapped[colour]<0) continue;
            SetAPen(&dest_rp,(ULONG)mapped[colour]);
            WritePixel(&dest_rp,x,y);
            WritePixel(&dest_mask_rp,x,y);
        }
    }
    dock->scaled_icon_bitmaps[index]=dest;
    dock->scaled_icon_masks[index]=dest_mask;
    dock->scaled_icon_width[index]=fitted_width;
    dock->scaled_icon_height[index]=fitted_height;
    dest=NULL;dest_mask=NULL;
done:
    if (dest) FreeBitMap(dest);
    if (dest_mask) FreeBitMap(dest_mask);
    if (source_mask) FreeBitMap(source_mask);
    DisposeDTObject(object);
    return dock->scaled_icon_bitmaps[index]!=NULL;
}
/* Decode the normal icon into a cached bitmap fitted to the common
 * artwork area. Hover never allocates or substitutes artwork. */
static int render_icon(struct NDDock *dock,int i,int max_width,
                       int max_height,struct BitMap **out_bitmap,
                       struct BitMap **out_mask,int *out_width,int *out_height)
{
    struct BitMap *dest = NULL, *dest_mask = NULL;
    struct RastPort dest_rp, mask_rp;
    struct TagItem info_tags[9];
    struct Image *planar;
    struct ColorRegister *palette = NULL;
    LONG palette_mapped = 0, icon_width = 0, icon_height = 0;
    LONG palette_size = 0, transparent = -1;
    PLANEPTR image_mask = NULL;
    UBYTE *image_data = NULL;
    UBYTE *scaled_pens = NULL, *scaled_opaque = NULL;
    unsigned long *mask_queue = NULL; ULONG pixel_count;
    LONG mapped[256];
    int width, height, fitted_width, fitted_height, x, y, source_x;
    int source_y,source_pen,opaque,max_pen,j;
    int ok = 0;
    if (!IconBase || IconBase->lib_Version<44 || !dock->icons[i]) return 0;
    width = height = 0;
    info_tags[0].ti_Tag = ICONCTRLA_IsPaletteMapped;
    info_tags[0].ti_Data = (ULONG)&palette_mapped;
    info_tags[1].ti_Tag = ICONCTRLA_GetWidth;
    info_tags[1].ti_Data = (ULONG)&icon_width;
    info_tags[2].ti_Tag = ICONCTRLA_GetHeight;
    info_tags[2].ti_Data = (ULONG)&icon_height;
    info_tags[3].ti_Tag = ICONCTRLA_GetImageData1;
    info_tags[3].ti_Data = (ULONG)&image_data;
    info_tags[4].ti_Tag = ICONCTRLA_GetTransparentColor1;
    info_tags[4].ti_Data = (ULONG)&transparent;
    info_tags[5].ti_Tag = ICONCTRLA_GetImageMask1;
    info_tags[5].ti_Data = (ULONG)&image_mask;
    info_tags[6].ti_Tag = ICONCTRLA_GetPalette1;
    info_tags[6].ti_Data = (ULONG)&palette;
    info_tags[7].ti_Tag = ICONCTRLA_GetPaletteSize1;
    info_tags[7].ti_Data = (ULONG)&palette_size;
    info_tags[8].ti_Tag = TAG_DONE;
    info_tags[8].ti_Data = 0;
    IconControlA(dock->icons[i],info_tags);
    planar=(struct Image *)dock->icons[i]->do_Gadget.GadgetRender;
    /* Raw ColorIcon image data is preferable here.  It carries the
     * original palette without causing icon.library to reserve pens. */
    if (image_data && palette && palette_size > 0 &&
        icon_width > 0 && icon_height > 0) {
        palette_mapped = 1;
        width = (int)icon_width;
        height = (int)icon_height;
    } else if (planar && planar->ImageData && planar->Width > 0 &&
               planar->Height > 0 && planar->Depth > 0) {
        palette_mapped = 0;
        width = planar->Width;
        height = planar->Height;
    } else return 0;
    nd_icon_fit(width,height,max_width,max_height,&fitted_width,&fitted_height);
    dest = AllocBitMap(fitted_width,fitted_height,
        dock->screen->RastPort.BitMap->Depth,BMF_CLEAR,
        dock->screen->RastPort.BitMap);
    dest_mask = AllocBitMap(fitted_width,fitted_height,1,BMF_CLEAR,NULL);
    if (!dest || !dest_mask) goto icon_done;
    InitRastPort(&dest_rp);
    dest_rp.BitMap = dest;
    InitRastPort(&mask_rp);
    mask_rp.BitMap = dest_mask;
    SetAPen(&mask_rp,1);
    pixel_count=(ULONG)fitted_width*(ULONG)fitted_height;
    scaled_pens=(UBYTE *)alloc_data(pixel_count);
    scaled_opaque=(UBYTE *)alloc_data(pixel_count);
    mask_queue=(unsigned long *)alloc_data(pixel_count*sizeof(unsigned long));
    if (!scaled_pens || !scaled_opaque || !mask_queue) goto icon_done;
    max_pen = dock->screen->RastPort.BitMap->Depth >= 8 ? 255 :
              (1 << dock->screen->RastPort.BitMap->Depth) - 1;
    for (j=0; j<256; ++j) {
        mapped[j] = -1;
    }
    if (palette_mapped) {
        if (palette_size > 256) palette_size = 256;
        for (j=0; j<palette_size; ++j)
            mapped[j] = FindColor(dock->screen->ViewPort.ColorMap,
                RGBBYTE(palette[j].red),RGBBYTE(palette[j].green),
                RGBBYTE(palette[j].blue),max_pen);
    }
    /* First build a small, nearest-neighbour pen map.  Classic planar
     * icons often fake transparency with a two-pen checkerboard matching
     * their Workbench drawer.  A generated icon.library mask may still
     * mark that entire rectangle opaque. */
    for (y=0; y<fitted_height; ++y) {
        source_y = (y * height) / fitted_height;
        for (x=0; x<fitted_width; ++x) {
            source_x = (x * width) / fitted_width;
            if (palette_mapped) {
                source_pen = image_data[source_y * width + source_x];
                if (image_mask)
                    opaque = icon_mask_pixel((const UBYTE *)image_mask,
                                             width,source_x,source_y);
                else opaque = transparent<0 || source_pen!=transparent;
                if (source_pen >= palette_size || mapped[source_pen] < 0)
                    opaque = 0;
            } else {
                source_pen = planar_icon_pixel(planar,source_x,source_y);
                if (image_mask)
                    opaque=icon_mask_pixel((const UBYTE *)image_mask,
                                           width,source_x,source_y);
                else opaque=1;
            }
            scaled_pens[(ULONG)y*(ULONG)fitted_width+(ULONG)x]=
                (UBYTE)(source_pen&255);
            scaled_opaque[(ULONG)y*(ULONG)fitted_width+(ULONG)x]=
                opaque ? 1 : 0;
        }
    }
    if (!palette_mapped || (!image_mask && transparent<0))
        (void)nd_icon_remove_border_background(scaled_pens,scaled_opaque,
            fitted_width,fitted_height,mask_queue,pixel_count);
    for (y=0;y<fitted_height;++y) {
        for (x=0;x<fitted_width;++x) {
            ULONG pixel=(ULONG)y*(ULONG)fitted_width+(ULONG)x;
            if (scaled_opaque[pixel]) {
                source_pen=scaled_pens[pixel];
                if (palette_mapped) source_pen=(int)mapped[source_pen];
                SetAPen(&dest_rp,(ULONG)source_pen);
                WritePixel(&dest_rp,x,y);
                WritePixel(&mask_rp,x,y);
            }
        }
    }
    *out_bitmap = dest;
    *out_mask = dest_mask;
    *out_width = fitted_width;
    *out_height = fitted_height;
    dest = NULL;
    dest_mask = NULL;
    ok = 1;
icon_done:
    if (scaled_pens) FreeVec(scaled_pens);
    if (scaled_opaque) FreeVec(scaled_opaque);
    if (mask_queue) FreeVec(mask_queue);
    if (dest) FreeBitMap(dest);
    if (dest_mask) FreeBitMap(dest_mask);
    return ok;
}
static void build_one_scaled_icon(struct NDDock *dock,int i,
                                  int max_width,int max_height)
{
    struct BitMap *bitmap,*mask;
    int width,height;
    if (dock->scaled_icon_bitmaps[i]) return;
    if (build_datatype_icon(dock,i,max_width,max_height)) return;
    if (!render_icon(dock,i,max_width,max_height,
                     &bitmap,&mask,&width,&height)) return;
    dock->scaled_icon_bitmaps[i]=bitmap;
    dock->scaled_icon_masks[i]=mask;
    dock->scaled_icon_width[i]=width;
    dock->scaled_icon_height[i]=height;
}
/* Decode Workbench icons once into a compact screen bitmap.  V44
 * ColorIcons use their generated image mask; classic planar icons are decoded
 * as well, so large legacy application icons do not disappear from the dock. */
void build_scaled_icon_bitmaps(struct NDDock *dock)
{
    int i, max_width, max_height;
    if (!dock->screen || !dock->window) return;
    /* Shared, aspect-preserving artwork limits for applications and plugins. */
    nd_icon_art_box(dock->tile_width,dock->tile_height,
                     dock->conf.item_size==ND_ITEM_SIZE_TINY,&max_width,&max_height);
    for (i=0; i<dock->conf.count; ++i) {
        if (dock->ipc) process_ipc(dock,0);
        build_one_scaled_icon(dock,i,max_width,max_height);
    }
    if (dock->ipc) process_ipc(dock,0);
}
/* Convert item I's ILBM brush into a screen bitmap and mask. */
static void build_one_brush(struct NDDock *dock,int i,int max_width,
                            int max_height)
{
    struct RastPort image_rp, mask_rp;
    struct NDILBM *brush;
    int x,y,sx,sy,color,last_color,fitted_width,fitted_height;
    brush = dock->brushes[i];
    if (!brush || dock->brush_bitmaps[i] || dock->brush_masks[i]) return;
    nd_icon_fit(brush->width,brush->height,max_width,max_height,
                &fitted_width,&fitted_height);
    if (fitted_width<1 || fitted_height<1) return;
    dock->brush_bitmaps[i] = AllocBitMap(fitted_width,fitted_height,
        dock->screen->RastPort.BitMap->Depth, BMF_CLEAR,
        dock->screen->RastPort.BitMap);
    dock->brush_masks[i] = AllocBitMap(fitted_width,fitted_height,
                                       1, BMF_CLEAR, NULL);
    if (!dock->brush_bitmaps[i] || !dock->brush_masks[i]) {
        if (dock->brush_bitmaps[i]) FreeBitMap(dock->brush_bitmaps[i]);
        if (dock->brush_masks[i]) FreeBitMap(dock->brush_masks[i]);
        dock->brush_bitmaps[i] = NULL;
        dock->brush_masks[i] = NULL;
        return;
    }
    InitRastPort(&image_rp);
    InitRastPort(&mask_rp);
    image_rp.BitMap = dock->brush_bitmaps[i];
    mask_rp.BitMap = dock->brush_masks[i];
    SetAPen(&mask_rp, 1);
    last_color = -1;
    for (y=0;y<fitted_height;++y) {
        sy=(y*brush->height)/fitted_height;
        for (x=0;x<fitted_width;++x) {
            sx=(x*brush->width)/fitted_width;
            color=brush->pixels[sy*ND_ICON_MAX_DIM+sx];
            if (color == brush->transparent ||
                dock->brush_pens[i][color] < 0) return;
            if (color != last_color) {
                SetAPen(&image_rp, (ULONG)dock->brush_pens[i][color]);
                last_color = color;
            }
            WritePixel(&image_rp, x, y);
            WritePixel(&mask_rp, x, y);
        }
    }
    dock->scaled_icon_width[i]=fitted_width;
    dock->scaled_icon_height[i]=fitted_height;
}
void build_brush_bitmaps(struct NDDock *dock)
{
    int i,max_width,max_height;
    if (!dock->screen) return;
    nd_icon_art_box(dock->tile_width,dock->tile_height,
                     dock->conf.item_size==ND_ITEM_SIZE_TINY,&max_width,&max_height);
    for (i = 0; i < dock->conf.count; ++i) {
        if (dock->ipc) process_ipc(dock,0);
        build_one_brush(dock,i,max_width,max_height);
    }
    if (dock->ipc) process_ipc(dock,0);
}
void release_icons(struct NDDock *dock)
{
    int i;
    if (GfxBase) WaitBlit();
    for (i = 0; i < ND_MAX_ITEMS; ++i) {
        if (dock->scaled_icon_bitmaps[i])
            FreeBitMap(dock->scaled_icon_bitmaps[i]);
        dock->scaled_icon_bitmaps[i] = NULL;
        if (dock->scaled_icon_masks[i]) FreeBitMap(dock->scaled_icon_masks[i]);
        dock->scaled_icon_masks[i] = NULL;
        dock->scaled_icon_width[i] = 0;
        dock->scaled_icon_height[i] = 0;
        dock->datatype_icon_paths[i][0]='\0';
        if (dock->brush_bitmaps[i]) FreeBitMap(dock->brush_bitmaps[i]);
        dock->brush_bitmaps[i] = NULL;
        if (dock->brush_masks[i]) FreeBitMap(dock->brush_masks[i]);
        dock->brush_masks[i] = NULL;
        if (dock->icons[i] && IconBase) FreeDiskObject(dock->icons[i]);
        dock->icons[i] = NULL;
        if (dock->brushes[i]) FreeVec(dock->brushes[i]);
        dock->brushes[i] = NULL;
    }
}
/* Startup: load and convert the artwork of a few tiles at a time, so the
 * window opens immediately on a slow machine and icons appear as they are
 * read.  Returns 1 while more remain.  A reload or Workbench reopen loads
 * everything at once and clears ICONS_PENDING (load_icons_reusing). */
int load_pending_icons(struct NDDock *dock,int budget)
{
    ULONG started=diagnostic_ticks();
    int i,done=0,max_width,max_height,tiny;
    APTR requesters;
    if (!dock->icons_pending) return 0;
    if (!dock->window || !dock->screen) return 1;
    tiny=dock->conf.item_size==ND_ITEM_SIZE_TINY;
    nd_icon_art_box(dock->tile_width,dock->tile_height,tiny,&max_width,&max_height);
    requesters=quiet_requesters();
    while (dock->icon_next<dock->conf.count && dock->icon_next<ND_MAX_ITEMS) {
        i=dock->icon_next++;
        if (dock->ipc) process_ipc(dock,0);
        if (!dock->icons[i] && !dock->brushes[i]) load_item_source(dock,i);
        remap_item_brush_colors(dock,i);
        build_one_scaled_icon(dock,i,max_width,max_height);
        build_one_brush(dock,i,max_width,max_height);
        mark_tile_dirty(dock,i);
        /* At most BUDGET tiles or about 40 ms, whichever comes first. */
        if (++done>=budget || diagnostic_ticks()-started>=2UL) break;
    }
    restore_requesters(requesters);
    if (dock->icon_next>=dock->conf.count || dock->icon_next>=ND_MAX_ITEMS)
        dock->icons_pending=0;
    return dock->icons_pending;
}
