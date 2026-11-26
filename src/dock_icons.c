/* NovaDock: Launcher icons: loading, scaling and reuse across reloads. */
#include "dock_internal.h"
#include <graphics/scale.h>
#include "nd_artwork.h"
#include "nd_task.h"

/* Defined below, private to this file. */
static int icon_source(const struct NDDock *dock,const struct NDConfig *cfg, int index,char *path);
static ULONG icon_fingerprint(const struct NDDock *dock,const struct NDConfig *cfg, int index);
static int adopt_icon_source(struct NDDock *dock,struct NDIconStash *stash, const struct NDConfig *previous,int index);
static int icon_mask_pixel(const UBYTE *mask,int width,int x,int y);
static struct {struct NDTask task;int index;ULONG fingerprint;} icon_jobs[2];
static UBYTE icon_load_state[ND_MAX_ITEMS];
static void close_icon_jobs(void)
{int i;for(i=0;i<2;++i)nd_task_close(&icon_jobs[i].task);memset(icon_load_state,0,sizeof(icon_load_state));}
unsigned int active_icon_jobs(void){return (icon_jobs[0].task.port!=NULL)+(icon_jobs[1].task.port!=NULL);}
unsigned long icon_job_signals(void){return nd_task_signals(&icon_jobs[0].task)|nd_task_signals(&icon_jobs[1].task);}
static int build_datatype_icon(struct NDDock *dock,int index, int max_width,int max_height);
static void release_plugin_artwork(struct NDDock *dock)
{
 int i,j,k;for(i=0;i<ND_MAX_ITEMS;++i)if(dock->plugin_artwork[i]){
  for(j=i+1;j<ND_MAX_ITEMS;++j)if(dock->plugin_artwork[j]==dock->plugin_artwork[i])dock->plugin_artwork[j]=NULL;
  if(dock->screen)for(k=1;k<16;++k)if(dock->plugin_artwork[i]->owned_pens[k])
   ReleasePen(dock->screen->ViewPort.ColorMap,dock->plugin_artwork[i]->mapped[k]);
  nd_artwork_close(dock->plugin_artwork[i]);free(dock->plugin_artwork[i]);dock->plugin_artwork[i]=NULL;
 }
}
static void map_plugin_palette(struct NDDock *dock,struct NDArtwork *a)
{
 int k;
 if(!dock->screen || !a || !a->data || a->palette_ready)return;
 for(k=1;k<16;++k){
  ULONG red=(ULONG)a->data[8+k*3]*0x01010101UL,green=(ULONG)a->data[9+k*3]*0x01010101UL,blue=(ULONG)a->data[10+k*3]*0x01010101UL;
  LONG mapped=ObtainBestPenA(dock->screen->ViewPort.ColorMap,red,green,blue,NULL);
  if(mapped>=0)a->owned_pens[k]=1;
  else mapped=FindColor(dock->screen->ViewPort.ColorMap,red,green,blue,-1);
  a->mapped[k]=mapped>=0 ? (ULONG)mapped:0;
 }
 a->palette_ready=1;
}
void map_plugin_artwork(struct NDDock *dock)
{
 int i;for(i=0;i<ND_MAX_ITEMS;++i)map_plugin_palette(dock,dock->plugin_artwork[i]);
}
void unmap_plugin_artwork(struct NDDock *dock)
{
 int i,k;struct NDArtwork *a;
 if(!dock->screen)return;
 for(i=0;i<ND_MAX_ITEMS;++i){
  a=dock->plugin_artwork[i];if(!a || !a->palette_ready)continue;
  for(k=1;k<16;++k){if(a->owned_pens[k])ReleasePen(dock->screen->ViewPort.ColorMap,a->mapped[k]);a->owned_pens[k]=0;a->mapped[k]=0;}
  a->palette_ready=0;
 }
}
void load_plugin_artwork(struct NDDock *dock,int index)
{
 const struct NDItem *item=&dock->conf.items[index];struct NDArtwork *a;char path[ND_ARTWORK_PATH];int j;
 if(item->kind!=ND_ITEM_PLUGIN)return;
 if(dock->plugin_artwork[index]){map_plugin_palette(dock,dock->plugin_artwork[index]);return;}
 if(!strcmp(item->id,"MUSIC") && !item->media_capabilities && !dock->plugin_art_key[index])dock->plugin_art_key[index]=nd_artwork_key("music");
 if(!dock->plugin_art_key[index] && strncmp(item->caption,"ART:",4))return;
 if(!nd_artwork_path(dock->directory,item->command,path,sizeof(path)))return;
 for(j=0;j<ND_MAX_ITEMS;++j)if(dock->plugin_artwork[j]&&!strcmp(dock->plugin_artwork[j]->path,path)){dock->plugin_artwork[index]=dock->plugin_artwork[j];map_plugin_palette(dock,dock->plugin_artwork[index]);return;}
 a=(struct NDArtwork *)calloc(1,sizeof(*a));if(!a)return;dock->plugin_artwork[index]=a;
 if(nd_artwork_load(a,path))map_plugin_palette(dock,a);
}
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
    int games_icon=item->kind==ND_ITEM_PLUGIN && (nd_collection_plugin_path(item->command) || nd_actions_plugin_path(item->command) || nd_shelf_plugin_path(item->command));
    int running_apps_icon=item->kind==ND_ITEM_PLUGIN &&
                          (!strcmp(item->id,"SCREENSWITCHER") || !strcmp(item->id,"BITSKIM") || !strcmp(item->id,"RUNNINGAPPS") || !strcmp(item->id,"CLIPBOARD") || !strcmp(item->id,"NET") || !strcmp(item->id,"ROADSHOW"));
    path[0]='\0';
    if(item->kind==ND_ITEM_PLUGIN && !strcmp(item->id,"AMINET"))
        return nd_collection_art_path(dock->directory,"Aminet",NULL,path,ND_ICON_PATH_LEN) ? 1:0;
    /* DOpus is deliberately drawn like a launcher. Use the icon beside its
     * configured executable, so changing Program path in the plugin
     * settings also changes the artwork without another icon setting. */
    if (dopus_icon) p=nd_option_get(cfg,"DOPUS4","PROGRAM");
    else if (games_icon) {
        APTR old=quiet_requesters();
        int result=nd_collection_art_path(dock->directory,nd_option_get(cfg,item->id,"ICON"),
                            nd_option_get(cfg,item->id,"ICONFILE"),path,ND_ICON_PATH_LEN);
        restore_requesters(old);return result ? 1:0;
    }
    else if (music_icon || running_apps_icon) p=item->command;
    if (item->transient==ND_TRANSIENT_TASK) return 2;
    if (((item->kind != ND_ITEM_LAUNCH && item->kind != ND_ITEM_DRAWER &&
          item->kind != ND_ITEM_GROUP) &&
         !dopus_icon && !music_icon && !running_apps_icon && !games_icon) || !p || !*p)
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

        if (!dock->icons[i] && !dock->brushes[i] && !dock->picture_sources[i]) continue;
        stash->icons[i]=dock->icons[i];dock->icons[i]=NULL;
        stash->brushes[i]=dock->brushes[i];dock->brushes[i]=NULL;
        stash->pictures[i]=dock->picture_sources[i];dock->picture_sources[i]=NULL;
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
        dock->picture_sources[index]=stash->pictures[j];stash->pictures[j]=NULL;icon_load_state[index]=2;
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
        if(stash->pictures[i])FreeVec(stash->pictures[i]);stash->pictures[i]=NULL;
        stash->icons[i]=NULL;stash->brushes[i]=NULL;stash->usable[i]=0;
    }
}
/* Read the artwork source of item I (icon, ILBM brush or DataTypes path).
 * The caller quietens requesters. */
static void load_item_source(struct NDDock *dock,int i)
{
 char path[ND_ICON_PATH_LEN],program[512],guard[64];struct NDIOJob request;const struct NDItem *item=&dock->conf.items[i];int slot,source;
 if(icon_load_state[i])return;
 load_plugin_artwork(dock,i);source=icon_source(dock,&dock->conf,i,path);
 if(!source){icon_load_state[i]=2;return;}
 for(slot=0;slot<2;++slot)if(!icon_jobs[slot].task.port){int busy;sprintf(guard,"NovaDock.Icon.%d",slot);Forbid();busy=FindPort(guard)!=NULL;Permit();if(!busy)break;}if(slot==2)return;
 memset(&request,0,sizeof(request));
 request.fallback=item->kind==ND_ITEM_DRAWER && item->command[0] && item->command[strlen(item->command)-1]==':' ? WBDISK:(item->kind==ND_ITEM_DRAWER || item->kind==ND_ITEM_GROUP) ? WBDRAWER:WBTOOL;
 if(source!=2)strcpy(request.path,path);
 request.iff=has_iff_extension(path);request.picture=!request.iff && !has_info_extension(path) && has_filename_extension(path);
 request.preferences=item->kind==ND_ITEM_LAUNCH && !strcmp(item->command,"NovaDockPrefs") && !strcmp(ITEM_TEXT(item,ND_TEXT_ICON),"NovaDockPrefs");
 strcpy(program,dock->directory);if(!AddPart(program,"SupportLog",sizeof(program)))return;sprintf(guard,"NovaDock.Icon.%d",slot);
 if(!nd_task_start(&icon_jobs[slot].task,program,guard,ND_IO_ICON,&request,sizeof(request)))return;
 icon_jobs[slot].task.dispose=nd_io_dispose;icon_jobs[slot].index=i;icon_jobs[slot].fingerprint=icon_fingerprint(dock,&dock->conf,i);icon_load_state[i]=1;++dock->diagnostic_icons_loaded;
}
void process_icon_jobs(struct NDDock *dock)
{
 int slot,max_width,max_height;
 for(slot=0;slot<2;++slot){int active=icon_jobs[slot].task.port!=NULL;struct NDTaskPacket *m=nd_task_poll(&icon_jobs[slot].task);int i=icon_jobs[slot].index;
  if(!m){if(active && !icon_jobs[slot].task.port && i>=0 && i<ND_MAX_ITEMS)icon_load_state[i]=2;continue;}
  if(m->bytes==sizeof(struct NDIOJob) && i>=0 && i<dock->conf.count && icon_jobs[slot].fingerprint==icon_fingerprint(dock,&dock->conf,i)){
   struct NDIOJob *result=(struct NDIOJob *)m->data;
   dock->icons[i]=result->icon;result->icon=NULL;dock->brushes[i]=result->brush;result->brush=NULL;dock->picture_sources[i]=result->decoded;result->decoded=NULL;icon_load_state[i]=2;
   if(dock->window && dock->screen){nd_icon_art_box(dock->tile_width,dock->tile_height,dock->conf.item_size==ND_ITEM_SIZE_TINY,&max_width,&max_height);remap_item_brush_colors(dock,i);build_one_scaled_icon(dock,i,max_width,max_height);build_one_brush(dock,i,max_width,max_height);mark_tile_dirty(dock,i);}
  }
  nd_io_dispose(m->data,m->bytes);FreeVec(m);
 }
}

void load_icons_reusing(struct NDDock *dock,struct NDIconStash *stash,
                               const struct NDConfig *previous)
{
    int i;
    APTR requesters=quiet_requesters();
    if(previous){ULONG keys[ND_MAX_ITEMS];UWORD states[ND_MAX_ITEMS];int j;
        memcpy(keys,dock->plugin_art_key,sizeof(keys));memcpy(states,dock->plugin_art_state,sizeof(states));
        memset(dock->plugin_art_key,0,sizeof(keys));memset(dock->plugin_art_state,0,sizeof(states));
        for(i=0;i<dock->conf.count;++i)for(j=0;j<previous->count;++j)if(!strcmp(dock->conf.items[i].id,previous->items[j].id)){dock->plugin_art_key[i]=keys[j];dock->plugin_art_state[i]=states[j];break;}
    }
    release_plugin_artwork(dock);
    dock->diagnostic_icons_reused=0;dock->diagnostic_icons_loaded=0;
    for (i = 0; i < dock->conf.count; ++i) {
        /* DataTypes and icon.library can be slow on a baseline 68k.  Keep
         * resident plugins acknowledged while a live reload rebuilds the
         * artwork, otherwise their bounded IPC wait expires and they exit. */
        if (dock->ipc) process_ipc(dock,0);
        load_plugin_artwork(dock,i);
        if (adopt_icon_source(dock,stash,previous,i)) {
            ++dock->diagnostic_icons_reused;
            continue;
        }
        load_item_source(dock,i);
    }
    if (dock->ipc) process_ipc(dock,0);
    restore_requesters(requesters);
    /* Reused sources are ready; new sources finish without blocking reload. */
    dock->icons_pending=1;dock->icon_next=0;
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
static int build_datatype_icon(struct NDDock *dock,int index,int max_width,int max_height)
{
 struct NDIconPicture *p=dock->picture_sources[index];struct BitMap *dest,*mask;struct RastPort rp,mrp;LONG mapped[256];int w,h,x,y,sx,sy,c,max_pen;
 if(!p || !dock->screen || !p->width || !p->height || p->width>96 || p->height>96 || !p->colors || p->colors>256)return 0;
 nd_icon_fit(p->width,p->height,max_width,max_height,&w,&h);dest=AllocBitMap(w,h,dock->screen->RastPort.BitMap->Depth,BMF_CLEAR,dock->screen->RastPort.BitMap);mask=AllocBitMap(w,h,1,BMF_CLEAR,NULL);
 if(!dest || !mask){if(dest)FreeBitMap(dest);if(mask)FreeBitMap(mask);return 0;}
 InitRastPort(&rp);InitRastPort(&mrp);rp.BitMap=dest;mrp.BitMap=mask;SetAPen(&mrp,1);max_pen=dock->screen->RastPort.BitMap->Depth>=8 ? 255:(1<<dock->screen->RastPort.BitMap->Depth)-1;
 for(c=0;c<p->colors;++c)mapped[c]=FindColor(dock->screen->ViewPort.ColorMap,RGBBYTE(p->rgb[c][0]),RGBBYTE(p->rgb[c][1]),RGBBYTE(p->rgb[c][2]),max_pen);
 for(y=0;y<h;++y)for(x=0;x<w;++x){unsigned int at;sy=y*p->height/h;sx=x*p->width/w;at=sy*p->width+sx;c=p->pixels[at];if(p->opaque[at] && c<p->colors && mapped[c]>=0){SetAPen(&rp,mapped[c]);WritePixel(&rp,x,y);WritePixel(&mrp,x,y);}}
 dock->scaled_icon_bitmaps[index]=dest;dock->scaled_icon_masks[index]=mask;dock->scaled_icon_width[index]=w;dock->scaled_icon_height[index]=h;return 1;
}
/* Decode the normal icon into a cached bitmap fitted to the common
 * artwork area. Hover can magnify that same bitmap and mask. */
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
    int ok = 0, remap_classic = 0;
    if (!IconBase || !dock->icons[i]) return 0;
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
    if (IconBase->lib_Version>=44) IconControlA(dock->icons[i],info_tags);
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
    else if (planar->Depth==3 && planar->PlanePick==7 &&
             !(planar->PlaneOnOff & ~7)) {
        /* Eight-colour classic icons have no embedded palette. MagicWB
         * supplies the convention; never interpret its indices as screen pens. */
        for (j=0;j<8;++j) {
            unsigned char rgb[3];
            nd_icon_magicwb_rgb(j,rgb);
            mapped[j]=FindColor(dock->screen->ViewPort.ColorMap,
                RGBBYTE(rgb[0]),RGBBYTE(rgb[1]),RGBBYTE(rgb[2]),max_pen);
        }
        remap_classic=1;
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
                if (palette_mapped || remap_classic) source_pen=(int)mapped[source_pen];
                if(source_pen<0)continue;
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
                dock->brush_pens[i][color] < 0) continue;
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
static int scale_hover_bitmap(struct BitMap *source,struct BitMap *dest,
                              int width,int height,int zoom_width,int zoom_height)
{
    struct BitScaleArgs scale;
    memset(&scale,0,sizeof(scale));
    scale.bsa_SrcWidth=scale.bsa_XSrcFactor=width;
    scale.bsa_SrcHeight=scale.bsa_YSrcFactor=height;
    scale.bsa_XDestFactor=zoom_width;
    scale.bsa_YDestFactor=zoom_height;
    scale.bsa_SrcBitMap=source;
    scale.bsa_DestBitMap=dest;
    BitMapScale(&scale);
    return scale.bsa_DestWidth==zoom_width && scale.bsa_DestHeight==zoom_height;
}

/* A bounded LRU retains neighbouring icons and their recent sizes. A shared
 * scratch pair forced every neighbour to be scaled again on every paint. */
static ULONG hover_bitmap_bytes(int width,int height,int depth)
{
    return (ULONG)((width+15)/16)*2UL*(ULONG)height*(ULONG)depth;
}
ULONG hover_cache_bytes(struct NDDock *dock)
{
    ULONG total=0;int i,depth=dock->screen->RastPort.BitMap->Depth;
    for(i=0;i<ND_HOVER_CACHE_SLOTS;++i){
        if(dock->hover_icons[i].bitmap)total+=hover_bitmap_bytes(ND_HOVER_ICON_LIMIT,ND_HOVER_ICON_LIMIT,depth);
        if(dock->hover_icons[i].mask)total+=hover_bitmap_bytes(ND_HOVER_ICON_LIMIT,ND_HOVER_ICON_LIMIT,1);
    }
    for(i=0;i<ND_MAX_ITEMS;++i){
        struct NDHoverWidget *c=&dock->hover_widgets[i];
        if(c->bitmap)total+=hover_bitmap_bytes(c->capacity_width,c->capacity_height,depth);
        if(c->mask)total+=hover_bitmap_bytes(c->capacity_width,c->capacity_height,1);
    }
    return total;
}
int hover_cache_reserve(struct NDDock *dock,ULONG bytes,int widget,int icon)
{
    ULONG budget,used,oldest;int i,chosen,kind;
    budget=(AvailMem(MEMF_ANY)+hover_cache_bytes(dock))/8UL;
    if(budget<131072UL)budget=131072UL;if(budget>1048576UL)budget=1048576UL;
    if(dock->conf.hover_cache_kb>=128 && dock->conf.hover_cache_kb<=8192)budget=(ULONG)dock->conf.hover_cache_kb*1024UL;
    dock->hover_cache_budget=budget;
    for(;;){
        used=hover_cache_bytes(dock);if(used<=budget && bytes<=budget-used)return 1;
        chosen=-1;kind=0;oldest=~0UL;
        for(i=0;i<ND_HOVER_CACHE_SLOTS;++i)if(i!=icon && (dock->hover_icons[i].bitmap||dock->hover_icons[i].mask) && dock->hover_icons[i].used<=oldest){chosen=i;kind=0;oldest=dock->hover_icons[i].used;}
        for(i=0;i<ND_MAX_ITEMS;++i)if(i!=widget && (dock->hover_widgets[i].bitmap||dock->hover_widgets[i].mask) && dock->hover_widgets[i].used<=oldest){chosen=i;kind=1;oldest=dock->hover_widgets[i].used;}
        if(chosen<0)return 0;
        WaitBlit();
        if(kind){struct NDHoverWidget *c=&dock->hover_widgets[chosen];if(c->bitmap)FreeBitMap(c->bitmap);if(c->mask)FreeBitMap(c->mask);memset(c,0,sizeof(*c));}
        else{struct NDHoverScale *c=&dock->hover_icons[chosen];if(c->bitmap)FreeBitMap(c->bitmap);if(c->mask)FreeBitMap(c->mask);memset(c,0,sizeof(*c));}
        ++dock->hover_cache_evictions;
    }
}
int magnify_icon(struct NDDock *dock,int index,struct BitMap **bitmap,struct BitMap **mask,
                 int *width,int *height,int zoom_width,int zoom_height)
{
    struct NDHoverScale *cache=NULL;
    ULONG serial;
    int i,slot=0;
    /* A live plugin can change from a wide widget to compact artwork while
     * its previous animation is still shrinking. Never scale past storage. */
    if(index<0 || index>=ND_MAX_ITEMS || zoom_width<1 || zoom_height<1 ||
       zoom_width>ND_HOVER_ICON_LIMIT || zoom_height>ND_HOVER_ICON_LIMIT ||
       !bitmap || !mask || !*bitmap || !*mask || *width<1 || *height<1)return 0;
    serial=dock->hover_banks[index].serial;
    for(i=0;i<ND_HOVER_CACHE_SLOTS;++i){
        struct NDHoverScale *c=&dock->hover_icons[i];
        if(c->valid && c->index==index && c->source==*bitmap &&
           c->source_mask==*mask && c->serial==serial &&
           c->width==zoom_width && c->height==zoom_height &&
           c->source_width==*width && c->source_height==*height){cache=c;break;}
        if(!c->bitmap || c->used<dock->hover_icons[slot].used)slot=i;
    }
    if(!cache){
        cache=&dock->hover_icons[slot];
        cache->valid=0;
        if(!hover_cache_reserve(dock,
            (!cache->bitmap ? hover_bitmap_bytes(ND_HOVER_ICON_LIMIT,ND_HOVER_ICON_LIMIT,(*bitmap)->Depth):0)+
            (!cache->mask ? hover_bitmap_bytes(ND_HOVER_ICON_LIMIT,ND_HOVER_ICON_LIMIT,1):0),-1,slot))return 0;
        if(!cache->bitmap)
            cache->bitmap=AllocBitMap(ND_HOVER_ICON_LIMIT,ND_HOVER_ICON_LIMIT,
                (*bitmap)->Depth,BMF_CLEAR,dock->screen->RastPort.BitMap);
        if(!cache->mask)
            cache->mask=AllocBitMap(ND_HOVER_ICON_LIMIT,ND_HOVER_ICON_LIMIT,1,BMF_CLEAR,NULL);
        if(!cache->bitmap || !cache->mask)return 0;
        WaitBlit();
        cache->valid=scale_hover_bitmap(*bitmap,cache->bitmap,*width,*height,zoom_width,zoom_height) &&
                     scale_hover_bitmap(*mask,cache->mask,*width,*height,zoom_width,zoom_height);
        cache->source=*bitmap;cache->source_mask=*mask;cache->index=index;
        cache->width=zoom_width;cache->height=zoom_height;
        cache->source_width=*width;cache->source_height=*height;cache->serial=serial;
    }
    if(!cache->valid)return 0;
    cache->used=++dock->hover_cache_clock;
    *bitmap=cache->bitmap;*mask=cache->mask;*width=zoom_width;*height=zoom_height;
    return 1;
}

void release_hover_graphics(struct NDDock *dock)
{
    int i;
    if(GfxBase)WaitBlit();
    for(i=0;i<ND_HOVER_CACHE_SLOTS;++i){
        if(dock->hover_icons[i].bitmap)FreeBitMap(dock->hover_icons[i].bitmap);
        if(dock->hover_icons[i].mask)FreeBitMap(dock->hover_icons[i].mask);
    }
    memset(dock->hover_icons,0,sizeof(dock->hover_icons));
    for(i=0;i<ND_MAX_ITEMS;++i){
        if(dock->hover_banks[i].bitmap)FreeBitMap(dock->hover_banks[i].bitmap);
        if(dock->hover_banks[i].mask)FreeBitMap(dock->hover_banks[i].mask);
        if(dock->hover_widgets[i].bitmap)FreeBitMap(dock->hover_widgets[i].bitmap);
        if(dock->hover_widgets[i].mask)FreeBitMap(dock->hover_widgets[i].mask);
    }
    memset(dock->hover_banks,0,sizeof(dock->hover_banks));
    if(dock->hover_shelf)FreeBitMap(dock->hover_shelf);
    if(dock->hover_frame)FreeBitMap(dock->hover_frame);
    dock->hover_frame=NULL;dock->hover_frame_width=dock->hover_frame_height=0;
    if(dock->hover_widget_mask)FreeBitMap(dock->hover_widget_mask);
    memset(dock->hover_widgets,0,sizeof(dock->hover_widgets));
    dock->hover_shelf=NULL;dock->hover_widget_mask=NULL;
    dock->hover_shelf_valid=0;dock->hover_shelf_serial=0;dock->painting_mask=0;
    memset(dock->hover_label_width,0,sizeof(dock->hover_label_width));
    dock->hover_prev_valid=0;dock->hover_anim_valid=0;dock->hover_animating=0;
    dock->hover_shelf_width=dock->hover_shelf_height=0;
    dock->hover_cache_clock=0;
}

void release_icons(struct NDDock *dock)
{
    int i;
    close_icon_jobs();
    release_plugin_artwork(dock);
    if (GfxBase) WaitBlit();
    release_hover_graphics(dock);
    for (i = 0; i < ND_MAX_ITEMS; ++i) {
        release_thumbnail(dock,i);
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
        if(dock->picture_sources[i])FreeVec(dock->picture_sources[i]);dock->picture_sources[i]=NULL;
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
        i=dock->icon_next;
        if (dock->ipc) process_ipc(dock,0);
        if (!dock->icons[i] && !dock->brushes[i] && !dock->picture_sources[i])load_item_source(dock,i);
        if(!icon_load_state[i])break;
        ++dock->icon_next;
        remap_item_brush_colors(dock,i);
        build_one_scaled_icon(dock,i,max_width,max_height);
        build_one_brush(dock,i,max_width,max_height);
        mark_tile_dirty(dock,i);
        /* At most BUDGET tiles or about 40 ms, whichever comes first. */
        if (++done>=budget || diagnostic_ticks()-started>=2UL) break;
    }
    restore_requesters(requesters);
    if (dock->icon_next>=dock->conf.count || dock->icon_next>=ND_MAX_ITEMS)
        {dock->icons_pending=0;for(i=0;i<dock->conf.count;++i)if(icon_load_state[i]==1)dock->icons_pending=1;}
    return dock->icons_pending;
}

/* Explicit retry: keep settings and resident plugins; replace only artwork. */
void reload_artwork(struct NDDock *dock)
{
 close_tooltip(dock);close_folder_popup(dock);release_icons(dock);load_icons(dock);
 draw_all(dock);show_feedback(dock,"Reloading artwork...",0);
}
