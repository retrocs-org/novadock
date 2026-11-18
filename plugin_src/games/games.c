/* NovaDock WHDLoad games: a dock tile that lists the WHDLoad installs in a
 * games drawer and starts them the way Workbench does.
 *
 * A game is a drawer containing a *.slave file anywhere below the chosen
 * root. Indexing runs in bounded batches while the window remains usable. Its
 * icon is <drawer>.info or <slave name>.info. Starting it loads this tile's
 * configured WHDLoad executable and runs it as a Workbench process with
 * the icon as argument, so WHDLoad sees the icon's tool types exactly as on
 * a double-click - on AmigaOS 3.0 and later, without workbench.library 44.
 *
 * Each independently identified tile caches its index on disk and has its own
 * LABEL and recursive DRAWER
 * (initially unset). Cached counts load after the tile connects; cold scans
 * run in bounded batches.  AmigaOS 3.0+, any 68000; C89. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_games_scan.h"
#include "nd_games_cache.h"
#include "nd_games_filter.h"
#include "nd_music_formats.h"
#include "nd_whdload_status.h"

#ifdef ND_MUSIC_LIBRARY
#define COLLECTION_MUSIC 1
#define COLLECTION_NAME "MUSIC"
#define COLLECTION_CACHE "MusicLibrary"
const char nd_version[]=ND_PLUGIN_VER_STRING(MUSICLIB,"MusicLibrary");
#else
#define COLLECTION_MUSIC 0
#define COLLECTION_NAME "WHDLoad"
#define COLLECTION_CACHE "WHDLoad"
const char nd_version[]=ND_PLUGIN_VER_STRING(WHDLOAD,"Games");
#endif
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct Library *IconBase=NULL;

#define GAMES_PATH ND_GAMES_SCAN_PATH
#define GAMES_MAX 10000
#define GAMES_ROWS 14
#define G_LIST 1
#define G_PLAY 2
#define G_RESCAN 3
#define G_FILTER 4
#define G_CLEAR 5

struct Game {
    struct Node node;      /* ln_Name = name */
    struct Node view_node;
    char *project;         /* icon path without .info */
    char name[1];
};

static struct List games;
static int game_count=-1;  /* -1: not scanned yet */
static struct MsgPort *wb_reply;
static struct NDGamesScan scanner;
static char tile_label[32]=COLLECTION_NAME;
static unsigned long music_formats;
static char music_player[GAMES_PATH];
static char whdload_command[GAMES_PATH]="WHDLoad";
static int root_valid,root_status=ND_WHD_FOLDER_UNSET;
static char cache_root[GAMES_PATH],cache_file[100];
static int cache_used=0,cache_warning=0;
static int running=0;      /* games started and not yet finished */

/* ---- small helpers ------------------------------------------------------- */

static APTR quiet(void)
{
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR old=self->pr_WindowPtr;
    self->pr_WindowPtr=(APTR)-1L;
    return old;
}

static void unquiet(APTR old)
{
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=old;
}

static int lower(int c)
{
    return c>='A' && c<='Z' ? c-'A'+'a' : c;
}

static int compare_names(const char *a,const char *b)
{
    while (*a && lower((unsigned char)*a)==lower((unsigned char)*b)) {++a;++b;}
    return lower((unsigned char)*a)-lower((unsigned char)*b);
}

static int exists(const char *path)
{
    APTR old=quiet();
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    unquiet(old);
    if (!lock) return 0;
    UnLock(lock);
    return 1;
}

static int join(char *out,int size,const char *dir,const char *name)
{
    if (strlen(dir)+strlen(name)+2>(size_t)size) return 0;
    strcpy(out,dir);
    return AddPart((STRPTR)out,(STRPTR)name,size)!=0;
}

/* ---- the game list ----------------------------------------------------- */

static void free_games(void)
{
    struct Node *node;
    while ((node=RemHead(&games))!=NULL) FreeVec(node);
    game_count=-1;
}

static void add_game(const char *name,const char *project)
{
    struct Game *game;
    struct Node *after;
    if (game_count>=GAMES_MAX) return;
    game=(struct Game *)AllocVec(sizeof(*game)+strlen(name)+strlen(project)+1,
                                 MEMF_PUBLIC|MEMF_CLEAR);
    if (!game) return;
    strcpy(game->name,name);
    game->project=game->name+strlen(name)+1;
    strcpy(game->project,project);
    game->node.ln_Name=game->name;game->view_node.ln_Name=game->name;
    /* Keep the list sorted by name. */
    for (after=games.lh_TailPred;after->ln_Pred;after=after->ln_Pred)
        if (compare_names(after->ln_Name,name)<=0) break;
    Insert(&games,&game->node,after->ln_Pred ? after : NULL);
    ++game_count;
}

struct GameDirectory {BPTR lock;struct FileInfoBlock *fib;char path[GAMES_PATH];};
static void *scan_open(void *context,const char *path)
{
    struct GameDirectory *dir;
    dir=(struct GameDirectory *)AllocVec(sizeof(*dir),MEMF_PUBLIC|MEMF_CLEAR);
    if(!dir)return NULL;
    dir->fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    dir->lock=Lock((STRPTR)path,ACCESS_READ);
    if(!dir->fib || !dir->lock || !Examine(dir->lock,dir->fib) || dir->fib->fib_DirEntryType<=0){
        if(dir->lock)UnLock(dir->lock);if(dir->fib)FreeDosObject(DOS_FIB,dir->fib);FreeVec(dir);return NULL;
    }
    strcpy(dir->path,path);return dir;
}
static int scan_next(void *context,void *handle,char *name,unsigned int size)
{
    struct GameDirectory *dir=(struct GameDirectory *)handle;
    if(ExNext(dir->lock,dir->fib)){
        /* Do not follow directory/soft links: they can lead outside the root
         * or back into an ancestor indefinitely. Real drawers are recursive. */
        if(dir->fib->fib_DirEntryType!=ST_USERDIR && dir->fib->fib_DirEntryType>=0)return 3;
        strncpy(name,dir->fib->fib_FileName,size-1);name[size-1]=0;
        if(COLLECTION_MUSIC && dir->fib->fib_DirEntryType<0){
            if(nd_music_file_matches(name,music_formats)){
                char project[GAMES_PATH];int before=game_count;
                if(!join(project,sizeof(project),dir->path,name))return -1;
                add_game(name,project);if(game_count==before || game_count>=GAMES_MAX)return -1;
            }
            return 3;
        }
        return dir->fib->fib_DirEntryType==ST_USERDIR ? 2:1;
    }
    return IoErr()==ERROR_NO_MORE_ENTRIES ? 0:-1;
}
static void scan_close(void *context,void *handle)
{
    struct GameDirectory *dir=(struct GameDirectory *)handle;
    UnLock(dir->lock);FreeDosObject(DOS_FIB,dir->fib);FreeVec(dir);
}
static int scan_found(void *context,const char *dir,const char *name,const char *slave_name)
{
    char slave[108],icon[GAMES_PATH+6],project[GAMES_PATH];int found=0,before=game_count;
    strcpy(slave,slave_name);slave[strlen(slave)-6]=0;
    if(join(project,sizeof(project),dir,name)){sprintf(icon,"%s.info",project);found=exists(icon);}
    if(!found && join(project,sizeof(project),dir,slave)){sprintf(icon,"%s.info",project);found=exists(icon);}
    if(!found && strlen(dir)<sizeof(project)){strcpy(project,dir);sprintf(icon,"%s.info",project);found=exists(icon);}
    if(found){add_game(name,project);if(game_count==before || game_count>=GAMES_MAX)return 0;}
    return 1;
}
static const struct NDGamesScanOps scan_ops={scan_open,scan_next,scan_close,scan_found};
static void read_label(const char *id)
{nd_plugin_option_string(id,"LABEL",COLLECTION_NAME,tile_label,sizeof(tile_label));if(!tile_label[0])strcpy(tile_label,COLLECTION_NAME);}
static int apply_music_options(void *context,const char *key,const char *value)
{
    int format=nd_music_format_index(key);
    if(format>=0){if(!strcmp(value,"YES"))music_formats|=nd_music_formats[format].bit;else if(!strcmp(value,"NO"))music_formats&=~nd_music_formats[format].bit;else return 0;}
    else if(!strcmp(key,"PLAYER") && *value){if(strlen(value)>=sizeof(music_player))return 0;strcpy(music_player,value);}
    return 1;
}
static unsigned long cache_options(void)
{return COLLECTION_MUSIC ? 0x80000000UL|music_formats:0;}
static void cache_settings(const char *id)
{
    const char *p;
    cache_file[0]=0;
    for(p=id;*p;++p)if(!((*p>='A' && *p<='Z') || (*p>='a' && *p<='z') ||
        (*p>='0' && *p<='9') || *p=='_' || *p=='-'))return;
    if(!*id || strlen(id)>19)return;
    sprintf(cache_file,"ENVARC:NovaDock/Cache/" COLLECTION_CACHE "/%s.idx",id);
    nd_plugin_option_string(id,"DRAWER","",cache_root,sizeof(cache_root));
    read_label(id);
    if(COLLECTION_MUSIC){
        music_formats=nd_music_default_mask();
        nd_plugin_option_string("MUSIC","DEFAULT_APP","",music_player,sizeof(music_player));
        (void)nd_plugin_options_effective(id,apply_music_options,NULL);
    }else{
        nd_plugin_option_string(id,"COMMAND","WHDLoad",whdload_command,sizeof(whdload_command));
        if(!whdload_command[0])strcpy(whdload_command,"WHDLoad");
    }
}
static void check_root(const char *id)
{
    APTR old;BPTR lock;struct FileInfoBlock *fib;LONG error;
    cache_settings(id);root_valid=0;root_status=ND_WHD_FOLDER_UNSET;
    if(!cache_root[0])return;
    old=quiet();lock=Lock(cache_root,ACCESS_READ);
    if(!lock){
        error=IoErr();
        root_status=(error==ERROR_OBJECT_NOT_FOUND || error==ERROR_DIR_NOT_FOUND) ? ND_WHD_FOLDER_MISSING:
                    error==ERROR_DEVICE_NOT_MOUNTED ? ND_WHD_FOLDER_UNAVAILABLE:
                    error==ERROR_NO_FREE_STORE ? ND_WHD_FOLDER_MEMORY:ND_WHD_FOLDER_UNREADABLE;
    }else{
        fib=AllocDosObject(DOS_FIB,NULL);
        if(!fib)root_status=ND_WHD_FOLDER_MEMORY;
        else{
            if(!Examine(lock,fib))root_status=ND_WHD_FOLDER_UNREADABLE;
            else if(fib->fib_DirEntryType<=0)root_status=ND_WHD_FOLDER_NOT_DRAWER;
            else{root_valid=1;root_status=ND_WHD_FOLDER_OK;}
            FreeDosObject(DOS_FIB,fib);
        }
        UnLock(lock);
    }
    unquiet(old);
}
static int cache_add(void *context,const char *name,const char *project)
{int before=game_count;add_game(name,project);return game_count>before;}
static int cache_next(void *context,char *name,unsigned int name_size,char *project,unsigned int project_size)
{
    struct Node **cursor=(struct Node **)context;struct Game *game;
    if(!(*cursor)->ln_Succ)return 0;
    game=(struct Game *)*cursor;*cursor=(*cursor)->ln_Succ;
    if(strlen(game->name)>=name_size || strlen(game->project)>=project_size)return 0;
    strcpy(name,game->name);strcpy(project,game->project);return 1;
}
static void save_cache(void)
{
    static const char *const drawers[]={"ENVARC:NovaDock","ENVARC:NovaDock/Cache","ENVARC:NovaDock/Cache/" COLLECTION_CACHE};
    struct Node *cursor=games.lh_Head;BPTR lock;APTR old;int i;
    if(!cache_file[0])return;
    old=quiet();
    for(i=0;i<3;++i){lock=CreateDir((STRPTR)drawers[i]);if(lock)UnLock(lock);}
    cache_warning=!nd_games_cache_write_options(cache_file,cache_root,cache_options(),(unsigned long)game_count,cache_next,&cursor);
    unquiet(old);
}
static void rescan(const char *id)
{
    APTR old;
    nd_games_scan_cancel(&scanner);free_games();game_count=0;
    check_root(id);cache_used=cache_warning=0;
    old=quiet();if(!COLLECTION_MUSIC || music_formats)nd_games_scan_begin(&scanner,&scan_ops,NULL,cache_root);unquiet(old);
    if(!cache_root[0])scanner.errors=1;
    if(!scanner.head && !scanner.errors)save_cache();
}

static void load_index(const char *id)
{
    APTR old;
    cache_settings(id);cache_used=cache_warning=0;game_count=0;
    nd_games_scan_cancel(&scanner);scanner.errors=0;
    old=quiet();cache_used=root_valid && nd_games_cache_read_options(cache_file,cache_root,cache_options(),cache_add,NULL);unquiet(old);
    if(!cache_used){free_games();rescan(id);}
}

/* ---- starting a game as Workbench does ----------------------------------- */

static void finished_games(void)
{
    struct WBStartup *startup;
    int i;
    if (!wb_reply) return;
    while ((startup=(struct WBStartup *)GetMsg(wb_reply))!=NULL) {
        UnLoadSeg(startup->sm_Segment);
        for (i=0;i<startup->sm_NumArgs;++i) {
            if (startup->sm_ArgList[i].wa_Lock) UnLock(startup->sm_ArgList[i].wa_Lock);
            if (startup->sm_ArgList[i].wa_Name) FreeVec(startup->sm_ArgList[i].wa_Name);
        }
        FreeVec(startup);
        --running;
    }
}

static char *copy_name(const char *name)
{
    char *copy=(char *)AllocVec(strlen(name)+1,MEMF_PUBLIC);
    if (copy) strcpy(copy,name);
    return copy;
}

/* A configured tool: an absolute path, beside the game, or in the
 * usual command drawers. */
static int find_tool(const char *tool,const char *game_dir,char *out,int size)
{
    static const char *const places[]={"C:","SYS:C","SYS:Utilities",
                                       "SYS:System","SYS:Tools",NULL};
    int i;
    if (strchr(tool,':')) {
        if ((int)strlen(tool)>=size) return 0;
        strcpy(out,tool);
        return exists(out);
    }
    if (join(out,size,game_dir,tool) && exists(out)) return 1;
    for (i=0;places[i];++i)
        if (join(out,size,places[i],tool) && exists(out)) return 1;
    return 0;
}

/* 0 on success, otherwise a reason for the user. */
static const char *start_game(const char *project)
{
    struct DiskObject *icon=NULL;
    struct WBStartup *startup=NULL;
    struct WBArg *args;
    struct Process *process;
    char game_dir[GAMES_PATH],tool[GAMES_PATH];
    BPTR segment=0,home=0,current=0;
    LONG stack;
    const char *problem=NULL;
    if (strlen(project)>=sizeof(game_dir)) return "The path is too long.";
    strcpy(game_dir,project);
    *PathPart((STRPTR)game_dir)='\0';
    icon=GetDiskObject((STRPTR)project);
    if(COLLECTION_MUSIC){
        const char *player=music_player[0] ? music_player:
            (icon && icon->do_DefaultTool ? (char *)icon->do_DefaultTool:"");
        if(!*player)problem="Choose a music player in Edit settings or in the Music plugin.";
        else if(!find_tool(player,game_dir,tool,sizeof(tool)))problem="The configured music player was not found.";
    }else{
        if(!icon)return "The game has no icon.";
        if(icon->do_Type!=WBPROJECT)
            problem="The game's icon is not a Workbench project icon.";
        else if(!find_tool(whdload_command,game_dir,tool,sizeof(tool)))
            problem="The configured WHDLoad command was not found. Check Edit settings.";
    }
    if(problem){if(icon)FreeDiskObject(icon);return problem;}
    if (!(segment=LoadSeg((STRPTR)tool)))
        problem="The selected program could not be loaded.";
    else if (!(startup=(struct WBStartup *)AllocVec(sizeof(*startup)+2*sizeof(struct WBArg),
                                                    MEMF_PUBLIC|MEMF_CLEAR)))
        problem="Not enough memory.";
    if (problem) {
        if (segment) UnLoadSeg(segment);
        if(icon)FreeDiskObject(icon);
        return problem;
    }
    stack=COLLECTION_MUSIC ? 32768:(icon->do_StackSize<8192 ? 8192 : icon->do_StackSize);
    args=(struct WBArg *)(startup+1);
    /* Argument 0 is the program, argument 1 the game's icon. */
    {
        BPTR tool_lock;
        APTR old=quiet();
        tool_lock=Lock((STRPTR)tool,ACCESS_READ);
        if (tool_lock) {args[0].wa_Lock=ParentDir(tool_lock);UnLock(tool_lock);}
        args[1].wa_Lock=Lock((STRPTR)game_dir,ACCESS_READ);
        unquiet(old);
    }
    args[0].wa_Name=(BYTE *)copy_name(FilePart((STRPTR)tool));
    args[1].wa_Name=(BYTE *)copy_name(FilePart((STRPTR)project));
    if (args[0].wa_Lock) home=DupLock(args[0].wa_Lock);
    if (args[1].wa_Lock) current=DupLock(args[1].wa_Lock);
    process=args[0].wa_Lock && args[1].wa_Lock && args[0].wa_Name && args[1].wa_Name ?
        CreateNewProcTags(NP_Seglist,segment,NP_FreeSeglist,FALSE,
                          NP_StackSize,stack,NP_Name,(ULONG)args[0].wa_Name,
                          NP_HomeDir,home,NP_CurrentDir,current,TAG_DONE) : NULL;
    if(icon)FreeDiskObject(icon);
    if (!process) {
        int i;
        if (home) UnLock(home);
        if (current) UnLock(current);
        for (i=0;i<2;++i) {
            if (args[i].wa_Lock) UnLock(args[i].wa_Lock);
            if (args[i].wa_Name) FreeVec(args[i].wa_Name);
        }
        UnLoadSeg(segment);
        FreeVec(startup);
        return "The selected item could not be started.";
    }
    /* The new process owns HOME and CURRENT; the startup message comes back
     * here when the game ends (finished_games frees everything else). */
    startup->sm_Message.mn_Node.ln_Type=NT_MESSAGE;
    startup->sm_Message.mn_ReplyPort=wb_reply;
    startup->sm_Message.mn_Length=sizeof(*startup);
    startup->sm_Process=&process->pr_MsgPort;
    startup->sm_Segment=segment;
    startup->sm_NumArgs=2;
    startup->sm_ToolWindow=NULL;
    startup->sm_ArgList=args;
    PutMsg(&process->pr_MsgPort,(struct Message *)startup);
    ++running;
    return NULL;
}

/* ---- the games window ------------------------------------------------------ */

struct Panel {
    struct Screen *screen;
    APTR visual;
    struct Gadget *gadgets,*list,*filter_field,*play_button;
    struct List view;
    char filter[64];
    int visible_count,requested;
    struct Window *window;
    LONG selected;
    char typed[16];
    char title[128];
    ULONG typed_seconds;
    ULONG click_seconds,click_micros;
    LONG click_row;
};

static void panel_title(struct Panel *panel)
{
    const char *status=root_status!=ND_WHD_FOLDER_OK ? nd_whd_folder_value(root_status):scanner.head ? "indexing folders..." :
        scanner.errors ? "index incomplete; check folder / Rescan" :
        cache_warning ? "ready; index cache could not be saved" :
        cache_used ? "cached index - Rescan to refresh" :
        game_count>0 ? "Filter titles, Return to play" : "none found; edit tile settings";
    sprintf(panel->title,"%.31s - %d/%d - %s",tile_label,panel->visible_count,game_count<0 ? 0:game_count,status);
    if(panel->window)SetWindowTitles(panel->window,(STRPTR)panel->title,(STRPTR)-1);
}

static void tell(struct Panel *panel,const char *text)
{
    struct EasyStruct es;
    APTR args[1];
    es.es_StructSize=sizeof(es);
    es.es_Flags=0;
    es.es_Title=(UBYTE *)COLLECTION_NAME;
    es.es_TextFormat=(UBYTE *)"%s";   /* game names may contain '%' */
    es.es_GadgetFormat=(UBYTE *)"OK";
    args[0]=(APTR)text;
    (void)EasyRequestArgs(panel->window,&es,NULL,args);
}

static struct Game *game_at(struct Panel *panel,LONG index)
{
    struct Node *node;
    LONG i=0;
    if(index<0)return NULL;
    for (node=panel->view.lh_Head;node && node->ln_Succ;node=node->ln_Succ,++i)
        if (i==index) return (struct Game *)((char *)node-offsetof(struct Game,view_node));
    return NULL;
}

static void select_row(struct Panel *panel,LONG row)
{
    if (panel->visible_count<=0) {panel->selected=-1;return;}
    if (row<0) row=0;
    if (row>=panel->visible_count) row=panel->visible_count-1;
    panel->selected=row;
    GT_SetGadgetAttrs(panel->list,panel->window,NULL,GTLV_Selected,row,
                      GTLV_MakeVisible,row,TAG_DONE);
}

static void close_panel(struct Panel *panel)
{
    if (panel->window) {CloseWindow(panel->window);panel->window=NULL;}
    if (panel->gadgets) {FreeGadgets(panel->gadgets);panel->gadgets=NULL;}
    if (panel->visual) {FreeVisualInfo(panel->visual);panel->visual=NULL;}
    if (panel->screen) {UnlockPubScreen(NULL,panel->screen);panel->screen=NULL;}
    panel->selected=-1;panel->requested=0;
}

static void attach_list(struct Panel *panel)
{
    struct Node *node;struct Game *selected=game_at(panel,panel->selected);LONG row=-1;
    GT_SetGadgetAttrs(panel->list,panel->window,NULL,GTLV_Labels,~0UL,TAG_DONE);
    panel->view.lh_Head=(struct Node *)&panel->view.lh_Tail;panel->view.lh_Tail=NULL;
    panel->view.lh_TailPred=(struct Node *)&panel->view.lh_Head;panel->visible_count=0;
    for(node=games.lh_Head;node->ln_Succ;node=node->ln_Succ){
        struct Game *game=(struct Game *)node;
        if(!nd_games_matches(game->name,panel->filter))continue;
        if(game==selected)row=panel->visible_count;
        AddTail(&panel->view,&game->view_node);++panel->visible_count;
    }
    panel->selected=-1;panel->click_row=-1;
    GT_SetGadgetAttrs(panel->list,panel->window,NULL,GTLV_Labels,(ULONG)&panel->view,
        GTLV_Selected,(ULONG)-1,TAG_DONE);
    if(panel->visible_count)select_row(panel,row<0 ? 0:row);
    GT_SetGadgetAttrs(panel->play_button,panel->window,NULL,GA_Disabled,(ULONG)(!panel->visible_count),TAG_DONE);
    panel_title(panel);
}
static void apply_filter(struct Panel *panel)
{
    struct StringInfo *info;
    if(!panel->filter_field || !panel->filter_field->SpecialInfo)return;
    info=(struct StringInfo *)panel->filter_field->SpecialInfo;
    if(!strcmp(info->Buffer,panel->filter))return;
    strncpy(panel->filter,info->Buffer,sizeof(panel->filter)-1);panel->filter[sizeof(panel->filter)-1]=0;
    panel->selected=-1;attach_list(panel);
}

static void open_panel(struct Panel *panel,const char *id)
{
    struct NewGadget ng;
    struct Gadget *last;
    struct TextAttr *font;
    int fh,width=400,list_h,height,filter_top,max_height;
    if (panel->window) {WindowToFront(panel->window);ActivateWindow(panel->window);return;}
    check_root(id);
    if(!root_valid || game_count<=0)return;
    if (!(panel->screen=LockPubScreen(NULL))) return;
    font=panel->screen->Font;
    fh=font ? font->ta_YSize : 8;
    if(width>panel->screen->Width-16)width=panel->screen->Width-16;
    filter_top=panel->screen->WBorTop+fh+1+6;
    max_height=panel->screen->Height-panel->screen->BarHeight-8;
    list_h=GAMES_ROWS*(fh+1)+4;
    if(list_h>max_height-filter_top-(fh+10)-(fh+6)-12)
        list_h=max_height-filter_top-(fh+10)-(fh+6)-12;
    if(list_h<(fh+1)*2){close_panel(panel);return;}
    height=filter_top+fh+10+list_h+4+fh+6+8;
    if (!(panel->visual=GetVisualInfo(panel->screen,TAG_DONE)) ||
        !CreateContext(&panel->gadgets)) {close_panel(panel);return;}
    last=panel->gadgets;
    memset(&ng,0,sizeof(ng));
    ng.ng_VisualInfo=panel->visual;
    ng.ng_TextAttr=font;
    ng.ng_LeftEdge=panel->screen->WBorLeft+8;
    ng.ng_TopEdge=filter_top;ng.ng_LeftEdge+=56;
    ng.ng_Width=width-ng.ng_LeftEdge-panel->screen->WBorRight-80;
    ng.ng_Height=fh+6;ng.ng_GadgetID=G_FILTER;ng.ng_GadgetText=(STRPTR)"_Filter";
    ng.ng_Flags=PLACETEXT_LEFT;
    panel->filter_field=last=CreateGadget(STRING_KIND,last,&ng,
        GTST_String,(ULONG)panel->filter,GTST_MaxChars,63,GT_Underscore,'_',TAG_DONE);
    ng.ng_LeftEdge=width-panel->screen->WBorRight-72;ng.ng_Width=64;
    ng.ng_GadgetID=G_CLEAR;ng.ng_GadgetText=(STRPTR)"_Clear";ng.ng_Flags=PLACETEXT_IN;
    last=CreateGadget(BUTTON_KIND,last,&ng,GT_Underscore,'_',TAG_DONE);
    ng.ng_LeftEdge=panel->screen->WBorLeft+8;ng.ng_TopEdge+=fh+10;
    ng.ng_GadgetText=NULL;ng.ng_Flags=0;
    ng.ng_Width=width-ng.ng_LeftEdge-panel->screen->WBorRight-8;
    ng.ng_Height=list_h;
    ng.ng_GadgetID=G_LIST;
    panel->list=last=CreateGadget(LISTVIEW_KIND,last,&ng,
        GTLV_Labels,(ULONG)&panel->view,GTLV_ShowSelected,0,TAG_DONE);
    ng.ng_TopEdge+=list_h+4;
    ng.ng_Height=fh+6;
    ng.ng_Width=90;
    ng.ng_GadgetText=(UBYTE *)"_Play";
    ng.ng_GadgetID=G_PLAY;
    panel->play_button=last=CreateGadget(BUTTON_KIND,last,&ng,GT_Underscore,'_',TAG_DONE);
    ng.ng_LeftEdge+=98;
    ng.ng_GadgetText=(UBYTE *)"_Rescan";
    ng.ng_GadgetID=G_RESCAN;
    last=CreateGadget(BUTTON_KIND,last,&ng,GT_Underscore,'_',TAG_DONE);
    if (!last) {close_panel(panel);return;}
    panel_title(panel);
    panel->window=OpenWindowTags(NULL,WA_PubScreen,(ULONG)panel->screen,
        WA_Width,width,WA_Height,height,
        WA_Left,(panel->screen->Width-width)/2,
        WA_Top,(panel->screen->Height-height)/2,
        WA_Title,(ULONG)panel->title,
        WA_Gadgets,(ULONG)panel->gadgets,WA_DragBar,TRUE,WA_DepthGadget,TRUE,
        WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,LISTVIEWIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|
                 IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY|IDCMP_RAWKEY|IDCMP_INTUITICKS,
        TAG_DONE);
    if (!panel->window) {close_panel(panel);return;}
    GT_RefreshWindow(panel->window,NULL);
    panel->click_row=-1;
    panel->selected=-1;attach_list(panel);
    ActivateGadget(panel->filter_field,panel->window,NULL);
}

static void play(struct Panel *panel)
{
    struct Game *game=game_at(panel,panel->selected);
    const char *problem;
    if (!game) return;
    problem=start_game(game->project);
    if (problem) tell(panel,problem);
    else close_panel(panel);
}

/* Type-ahead: jump to the first game starting with what was typed. */
static void type_ahead(struct Panel *panel,char c,ULONG seconds)
{
    struct Node *node;
    size_t n;
    LONG i=0;
    if (seconds-panel->typed_seconds>1) panel->typed[0]='\0';
    panel->typed_seconds=seconds;
    n=strlen(panel->typed);
    if (n+1>=sizeof(panel->typed)) return;
    panel->typed[n]=c;panel->typed[n+1]='\0';
    for (node=panel->view.lh_Head;node->ln_Succ;node=node->ln_Succ,++i) {
        const char *a=node->ln_Name,*b=panel->typed;
        while (*b && lower((unsigned char)*a)==lower((unsigned char)*b)) {++a;++b;}
        if (!*b) {select_row(panel,i);return;}
    }
}

/* 1 while the window stays open. */
static int panel_events(struct Panel *panel,const char *id)
{
    struct IntuiMessage *msg;
    ULONG cls,seconds,micros;
    UWORD code;
    struct Gadget *gadget;
    if (!panel->window) return 0;
    while (panel->window && (msg=GT_GetIMsg(panel->window->UserPort))!=NULL) {
        cls=msg->Class;code=msg->Code;gadget=(struct Gadget *)msg->IAddress;
        seconds=msg->Seconds;micros=msg->Micros;
        GT_ReplyIMsg(msg);
        if (cls==IDCMP_CLOSEWINDOW) close_panel(panel);
        else if (cls==IDCMP_REFRESHWINDOW) {
            GT_BeginRefresh(panel->window);GT_EndRefresh(panel->window,TRUE);
        } else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_LIST) {
            /* A second click on the same game starts it. */
            if ((LONG)code==panel->click_row &&
                DoubleClick(panel->click_seconds,panel->click_micros,seconds,micros)) {
                panel->selected=code;play(panel);
            } else {
                panel->selected=code;panel->click_row=code;
                panel->click_seconds=seconds;panel->click_micros=micros;
            }
        } else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_PLAY) play(panel);
        else if(cls==IDCMP_INTUITICKS || (cls==IDCMP_GADGETUP && gadget->GadgetID==G_FILTER))apply_filter(panel);
        else if(cls==IDCMP_GADGETUP && gadget->GadgetID==G_CLEAR){
            GT_SetGadgetAttrs(panel->filter_field,panel->window,NULL,GTST_String,(ULONG)"",TAG_DONE);
            apply_filter(panel);ActivateGadget(panel->filter_field,panel->window,NULL);
        }
        else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_RESCAN) {
            panel->selected=-1;
            GT_SetGadgetAttrs(panel->list,panel->window,NULL,GTLV_Labels,~0UL,TAG_DONE);
            rescan(id);
            panel->selected=-1;
            panel_title(panel);
            attach_list(panel);
        } else if (cls==IDCMP_RAWKEY && (code==0x4C || code==0x4D))
            select_row(panel,panel->selected+(code==0x4C ? -1 : 1));
        else if (cls==IDCMP_VANILLAKEY) {
            if (code==27) close_panel(panel);
            else if (code==13) play(panel);
            else if (code>32 && code<127) type_ahead(panel,(char)code,seconds);
        }
    }
    return panel->window!=NULL;
}

/* ---- resident tile ---------------------------------------------------------- */

int main(int argc,char **argv)
{
    struct NDClient client;
    struct Panel panel;
    unsigned int event,elapsed=0;
    char value[32],badge[8];
    if (argc<2) return 20;
    /* An empty exec list (NewList() without amiga.lib). */
    games.lh_Head=(struct Node *)&games.lh_Tail;
    games.lh_Tail=NULL;
    games.lh_TailPred=(struct Node *)&games.lh_Head;
    memset(&panel,0,sizeof(panel));
    panel.selected=-1;panel.view.lh_Head=(struct Node *)&panel.view.lh_Tail;
    panel.view.lh_Tail=NULL;panel.view.lh_TailPred=(struct Node *)&panel.view.lh_Head;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library",39);
    GadToolsBase=OpenLibrary((STRPTR)"gadtools.library",39);
    IconBase=OpenLibrary((STRPTR)"icon.library",37);
    wb_reply=CreateMsgPort();
    if (!IntuitionBase || !GadToolsBase || !IconBase || !wb_reply ||
        !nd_client_init(&client,argv[1])) {
        if (wb_reply) DeleteMsgPort(wb_reply);
        if (IconBase) CloseLibrary(IconBase);
        if (GadToolsBase) CloseLibrary(GadToolsBase);
        if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    check_root(argv[1]);
    for (;;) {
        finished_games();
        if(!root_valid)strcpy(value,nd_whd_folder_value(root_status));
        else if(COLLECTION_MUSIC && !music_formats)strcpy(value,"NO FORMATS SELECTED");
        else if(scanner.head || game_count<0)strcpy(value,"INDEXING");
        else if(scanner.errors)strcpy(value,"INDEX INCOMPLETE");
        else if (!game_count) strcpy(value,"NO ITEMS");
        else sprintf(value,"%d ITEMS",game_count);
        if(root_valid && game_count>=0 && !scanner.head)sprintf(badge,"%d",game_count);
        else if(root_valid)strcpy(badge,"...");else badge[0]=0;
        event=nd_client_set_badge(&client,badge);
        if(!(event&ND_EVENT_QUIT)){
            if(!root_valid)event|=nd_client_update_art_quiet(&client,tile_label,value,
                COLLECTION_MUSIC ? "SadMusic":"SadWHDLoad",0,elapsed,500U);
            else event|=nd_client_update_quiet(&client,tile_label,value,ND_NO_METER,elapsed,500U);
        }
        if (event&ND_EVENT_QUIT) break;
        if (event&ND_EVENT_SETTINGS) {
            /* Another drawer: read it again the next time the list opens. */
            close_panel(&panel);
            nd_games_scan_cancel(&scanner);free_games();check_root(argv[1]);
        }
        if(event&ND_EVENT_CLICK){
            check_root(argv[1]);
            if(root_valid){
                if(game_count==0 && !scanner.head)rescan(argv[1]);
                panel.requested=1;
                if(game_count>0)open_panel(&panel,argv[1]);
            }
        }
        if(root_valid && game_count<0 && !scanner.head)load_index(argv[1]);
        if(panel.requested && !panel.window){
            if(game_count>0)open_panel(&panel,argv[1]);
            else if(!scanner.head && game_count==0)panel.requested=0;
        }
        (void)panel_events(&panel,argv[1]);
        if(scanner.head){
            APTR old;int before=game_count;
            if(panel.window)GT_SetGadgetAttrs(panel.list,panel.window,NULL,GTLV_Labels,~0UL,TAG_DONE);
            old=quiet();nd_games_scan_step(&scanner,24);unquiet(old);
            if(!scanner.head && !scanner.errors)save_cache();
            if(panel.window){
                if(before!=game_count)attach_list(&panel);
                else GT_SetGadgetAttrs(panel.list,panel.window,NULL,GTLV_Labels,(ULONG)&panel.view,TAG_DONE);
                if(!scanner.head)panel_title(&panel);
            }
            Delay(1);elapsed=1;
        }
        else if (panel.window) {Delay(5);elapsed=5;}
        else elapsed=nd_client_wait(&client,nd_client_delay_ticks(&client,250));
    }
    close_panel(&panel);
    nd_games_scan_cancel(&scanner);
    nd_client_close(&client);
    /* Running games reply to WB_REPLY when they end: wait for them before
     * the port goes away. */
    while (running>0) {
        WaitPort(wb_reply);
        finished_games();
    }
    free_games();
    DeleteMsgPort(wb_reply);
    CloseLibrary(IconBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
