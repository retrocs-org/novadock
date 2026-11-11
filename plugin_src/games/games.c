/* NovaDock WHDLoad games: a dock tile that lists the WHDLoad installs in a
 * games drawer and starts them the way Workbench does.
 *
 * A game is a drawer containing a *.slave file, either directly in the
 * games drawer (Games:Turrican) or one level down (Games:T/Turrican).  Its
 * icon is <drawer>.info or <slave name>.info.  Starting it loads the icon's
 * default tool (normally WHDLoad) and runs it as a Workbench process with
 * the icon as argument, so WHDLoad sees the icon's tool types exactly as on
 * a double-click - on AmigaOS 3.0 and later, without workbench.library 44.
 *
 * The games drawer (option DRAWER, default Games:) is read only when the
 * list is first opened, never at boot.  AmigaOS 3.0+, any 68000; C89. */
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
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"

const char nd_version[]=ND_VER_STRING("Games");
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
struct Library *IconBase=NULL;

#define GAMES_PATH 300
#define GAMES_MAX 1000
#define GAMES_ROWS 14
#define G_LIST 1
#define G_PLAY 2
#define G_RESCAN 3

struct Game {
    struct Node node;      /* ln_Name = name */
    char *project;         /* icon path without .info */
    char name[1];
};

static struct List games;
static int game_count=-1;  /* -1: not scanned yet */
static struct MsgPort *wb_reply;
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

static int ends_with(const char *text,const char *suffix)
{
    size_t t=strlen(text),s=strlen(suffix);
    return t>s && !compare_names(text+t-s,suffix);
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
    game->node.ln_Name=game->name;
    /* Keep the list sorted by name. */
    for (after=games.lh_TailPred;after->ln_Pred;after=after->ln_Pred)
        if (compare_names(after->ln_Name,name)<=0) break;
    Insert(&games,&game->node,after->ln_Pred ? after : NULL);
    ++game_count;
}

/* The game in drawer DIR (called NAME), if it has a slave and an icon. */
static int consider_game(const char *dir,const char *name)
{
    struct FileInfoBlock *fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    char slave[108],icon[GAMES_PATH],project[GAMES_PATH];
    BPTR lock;
    int found=0;
    if (!fib) return 0;
    slave[0]='\0';
    if ((lock=Lock((STRPTR)dir,ACCESS_READ))!=0) {
        if (Examine(lock,fib))
            while (!slave[0] && ExNext(lock,fib))
                if (fib->fib_DirEntryType<0 && ends_with(fib->fib_FileName,".slave"))
                    strcpy(slave,fib->fib_FileName);
        UnLock(lock);
    }
    FreeDosObject(DOS_FIB,fib);
    if (!slave[0]) return 0;
    slave[strlen(slave)-6]='\0';
    /* Usually <drawer>.info, sometimes <slave>.info. */
    if (join(project,sizeof(project),dir,name)) {
        sprintf(icon,"%s.info",project);
        found=exists(icon);
    }
    if (!found && join(project,sizeof(project),dir,slave)) {
        sprintf(icon,"%s.info",project);
        found=exists(icon);
    }
    if (found) add_game(name,project);
    return 1;     /* a game drawer, even without a usable icon */
}

/* Games in DIR; with DEPTH 0, also inside letter drawers (Games:T/...). */
static void scan(const char *dir,int depth)
{
    struct FileInfoBlock *fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    char path[GAMES_PATH];
    BPTR lock;
    if (!fib) return;
    if ((lock=Lock((STRPTR)dir,ACCESS_READ))!=0) {
        if (Examine(lock,fib) && fib->fib_DirEntryType>0)
            while (ExNext(lock,fib) && game_count<GAMES_MAX) {
                if (fib->fib_DirEntryType<=0 ||
                    !join(path,sizeof(path),dir,fib->fib_FileName)) continue;
                if (!consider_game(path,fib->fib_FileName) && depth==0)
                    scan(path,1);
            }
        UnLock(lock);
    }
    FreeDosObject(DOS_FIB,fib);
}

static void rescan(const char *id)
{
    char drawer[GAMES_PATH];
    APTR old;
    free_games();
    game_count=0;
    nd_plugin_option_string(id,"DRAWER","Games:",drawer,sizeof(drawer));
    old=quiet();
    scan(drawer,0);
    unquiet(old);
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

/* The icon's default tool: an absolute path, beside the game, or in the
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
    struct DiskObject *icon;
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
    if (!(icon=GetDiskObject((STRPTR)project))) return "The game has no icon.";
    if (icon->do_Type!=WBPROJECT || !icon->do_DefaultTool || !icon->do_DefaultTool[0])
        problem="The game's icon does not name a program (such as WHDLoad).";
    else if (!find_tool((char *)icon->do_DefaultTool,game_dir,tool,sizeof(tool)))
        problem="The game's program (such as WHDLoad) was not found.";
    else if (!(segment=LoadSeg((STRPTR)tool)))
        problem="The game's program could not be loaded.";
    else if (!(startup=(struct WBStartup *)AllocVec(sizeof(*startup)+2*sizeof(struct WBArg),
                                                    MEMF_PUBLIC|MEMF_CLEAR)))
        problem="Not enough memory.";
    if (problem) {
        if (segment) UnLoadSeg(segment);
        FreeDiskObject(icon);
        return problem;
    }
    stack=icon->do_StackSize<8192 ? 8192 : icon->do_StackSize;
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
    FreeDiskObject(icon);
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
        return "The game could not be started.";
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
    struct Gadget *gadgets,*list;
    struct Window *window;
    LONG selected;
    char typed[16];
    ULONG typed_seconds;
    ULONG click_seconds,click_micros;
    LONG click_row;
};

static void tell(struct Panel *panel,const char *text)
{
    struct EasyStruct es;
    APTR args[1];
    es.es_StructSize=sizeof(es);
    es.es_Flags=0;
    es.es_Title=(UBYTE *)"NovaDock Games";
    es.es_TextFormat=(UBYTE *)"%s";   /* game names may contain '%' */
    es.es_GadgetFormat=(UBYTE *)"OK";
    args[0]=(APTR)text;
    (void)EasyRequestArgs(panel->window,&es,NULL,args);
}

static struct Game *game_at(LONG index)
{
    struct Node *node;
    LONG i=0;
    for (node=games.lh_Head;node->ln_Succ;node=node->ln_Succ,++i)
        if (i==index) return (struct Game *)node;
    return NULL;
}

static void select_row(struct Panel *panel,LONG row)
{
    if (game_count<=0) return;
    if (row<0) row=0;
    if (row>=game_count) row=game_count-1;
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
}

static void attach_list(struct Panel *panel)
{
    GT_SetGadgetAttrs(panel->list,panel->window,NULL,GTLV_Labels,(ULONG)&games,TAG_DONE);
    if (game_count>0) select_row(panel,panel->selected<0 ? 0 : panel->selected);
}

static void open_panel(struct Panel *panel,const char *id)
{
    struct NewGadget ng;
    struct Gadget *last;
    struct TextAttr *font;
    int fh,width=340,list_h,height;
    if (panel->window) {WindowToFront(panel->window);ActivateWindow(panel->window);return;}
    if (game_count<0) rescan(id);
    if (!(panel->screen=LockPubScreen(NULL))) return;
    font=panel->screen->Font;
    fh=font ? font->ta_YSize : 8;
    list_h=GAMES_ROWS*(fh+1)+4;
    height=panel->screen->WBorTop+fh+1+8+list_h+6+fh+6+8;
    if (!(panel->visual=GetVisualInfo(panel->screen,TAG_DONE)) ||
        !CreateContext(&panel->gadgets)) {close_panel(panel);return;}
    last=panel->gadgets;
    memset(&ng,0,sizeof(ng));
    ng.ng_VisualInfo=panel->visual;
    ng.ng_TextAttr=font;
    ng.ng_LeftEdge=panel->screen->WBorLeft+8;
    ng.ng_TopEdge=panel->screen->WBorTop+fh+1+6;
    ng.ng_Width=width-ng.ng_LeftEdge-panel->screen->WBorRight-8;
    ng.ng_Height=list_h;
    ng.ng_GadgetID=G_LIST;
    panel->list=last=CreateGadget(LISTVIEW_KIND,last,&ng,
        GTLV_Labels,(ULONG)&games,GTLV_ShowSelected,0,TAG_DONE);
    ng.ng_TopEdge+=list_h+4;
    ng.ng_Height=fh+6;
    ng.ng_Width=90;
    ng.ng_GadgetText=(UBYTE *)"_Play";
    ng.ng_GadgetID=G_PLAY;
    last=CreateGadget(BUTTON_KIND,last,&ng,GT_Underscore,'_',TAG_DONE);
    ng.ng_LeftEdge+=98;
    ng.ng_GadgetText=(UBYTE *)"_Rescan";
    ng.ng_GadgetID=G_RESCAN;
    last=CreateGadget(BUTTON_KIND,last,&ng,GT_Underscore,'_',TAG_DONE);
    if (!last) {close_panel(panel);return;}
    panel->window=OpenWindowTags(NULL,WA_PubScreen,(ULONG)panel->screen,
        WA_Width,width,WA_Height,height,
        WA_Left,(panel->screen->Width-width)/2,
        WA_Top,(panel->screen->Height-height)/2,
        WA_Title,(ULONG)(game_count>0 ? "Games - type to find, Return to play" :
                         "Games - none found (set the drawer in Preferences)"),
        WA_Gadgets,(ULONG)panel->gadgets,WA_DragBar,TRUE,WA_DepthGadget,TRUE,
        WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_IDCMP,LISTVIEWIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|
                 IDCMP_REFRESHWINDOW|IDCMP_VANILLAKEY|IDCMP_RAWKEY,
        TAG_DONE);
    if (!panel->window) {close_panel(panel);return;}
    GT_RefreshWindow(panel->window,NULL);
    panel->click_row=-1;
    attach_list(panel);
}

static void play(struct Panel *panel)
{
    struct Game *game=game_at(panel->selected);
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
    for (node=games.lh_Head;node->ln_Succ;node=node->ln_Succ,++i) {
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
        else if (cls==IDCMP_GADGETUP && gadget->GadgetID==G_RESCAN) {
            GT_SetGadgetAttrs(panel->list,panel->window,NULL,GTLV_Labels,~0UL,TAG_DONE);
            rescan(id);
            panel->selected=0;
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
    char value[32];
    if (argc<2) return 20;
    /* An empty exec list (NewList() without amiga.lib). */
    games.lh_Head=(struct Node *)&games.lh_Tail;
    games.lh_Tail=NULL;
    games.lh_TailPred=(struct Node *)&games.lh_Head;
    memset(&panel,0,sizeof(panel));
    panel.selected=-1;
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
    for (;;) {
        finished_games();
        if (game_count<0) strcpy(value,"GAMES");
        else if (!game_count) strcpy(value,"NO GAMES");
        else sprintf(value,"%d GAMES",game_count);
        event=nd_client_update_quiet(&client,"GAMES",value,ND_NO_METER,elapsed,500U);
        if (event&ND_EVENT_QUIT) break;
        if (event&ND_EVENT_SETTINGS) {
            /* Another drawer: read it again the next time the list opens. */
            close_panel(&panel);
            free_games();
        }
        if (event&ND_EVENT_CLICK) open_panel(&panel,argv[1]);
        if (panel_events(&panel,argv[1])) {Delay(5);elapsed=5;}
        else elapsed=nd_client_wait(&client,nd_client_delay_ticks(&client,250));
    }
    close_panel(&panel);
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
