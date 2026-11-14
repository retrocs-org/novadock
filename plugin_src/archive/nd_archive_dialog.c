/* Native creation dialog; the helper owns it, never the dock process. */
#define IntuitionBase NDArchiveIntuitionBase
#define GadToolsBase NDArchiveGadToolsBase
#define AslBase NDArchiveAslBase
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/var.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
#include <stdio.h>
#include <string.h>
#include "nd_archive.h"
#include "nd_archive_dialog.h"
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL,*AslBase=NULL;
/* Last-used dialog values are runtime state, separate from dock options. */
static int last_value(const char *key,char *out,unsigned int size)
{
    LONG n=GetVar((STRPTR)key,out,(LONG)size,GVF_GLOBAL_ONLY);
    if(n<0 || n>=(LONG)size){out[0]=0;return 0;}
    out[n]=0;return 1;
}
static int last_bool(const char *key,int fallback)
{char value[8];if(!last_value(key,value,sizeof(value)))return fallback;
 if(!strcmp(value,"YES"))return 1;if(!strcmp(value,"NO"))return 0;return fallback;}
static int save_failed;
static void save_value(const char *key,const char *value)
{
    BPTR lock;const char *dirs[2]={"ENV:NovaDock","ENVARC:NovaDock"};int i;
    for(i=0;i<2;++i){lock=Lock((STRPTR)dirs[i],ACCESS_READ);
        if(!lock)lock=CreateDir((STRPTR)dirs[i]);if(lock)UnLock(lock);}
    if(!SetVar((STRPTR)key,(STRPTR)value,-1,GVF_GLOBAL_ONLY|GVF_SAVE_VAR))save_failed=1;
}
static void save_bool(const char *key,int value)
{save_value(key,value ? "YES":"NO");}
static void notice(struct Window *w,const char *text)
{
    struct EasyStruct e;
    memset(&e,0,sizeof(e));e.es_StructSize=sizeof(e);
    e.es_Title=(STRPTR)"Archiver";e.es_TextFormat=(STRPTR)text;e.es_GadgetFormat=(STRPTR)"OK";
    EasyRequestArgs(w,&e,NULL,NULL);
}
static const char *field(struct Gadget *g)
{return ((struct StringInfo *)g->SpecialInfo)->Buffer;}
int nd_archive_compression_request(const struct NDArchiveSource *sources,unsigned int count,
                                   int require_name,int mask,int *kind,
                                   char *output,unsigned int size,int *create_icon)
{
    static STRPTR formats[]={"ZIP","LHA",NULL};
    struct Screen *screen=NULL;struct Window *win=NULL;
    struct Gadget *gadgets=NULL,*last,*cycle,*drawer,*file,*g;
    struct NewGadget ng;struct TextAttr font={(STRPTR)"topaz.font",8,0,0};
    struct IntuiMessage *msg;struct FileRequester *request;
    APTR visual=NULL;BPTR lock;struct FileInfoBlock *fib;int is_drawer;
    char parent[260],name[160],canonical[260],filename[164];
    char source_label[76];
    ULONG cls;UWORD id,code;
    int w,top,format=mask&1 ? 0 : 1,result=-1,done=0,icon=1,open=0;
    char remembered[260];
    const char *source=sources[0].path,*base=source,*p;
    unsigned int item;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);AslBase=OpenLibrary("asl.library",39);
    if (!IntuitionBase || !GadToolsBase || !AslBase) goto cleanup;
    screen=LockPubScreen("Workbench");if (!screen) goto cleanup;
    w=screen->Width-24;if(w>560)w=560;if(w<400)goto cleanup;
    visual=GetVisualInfo(screen,TAG_DONE);if(!visual)goto cleanup;
    for(p=source;*p;++p)if(*p==':' || *p=='/')base=p+1;
    if(base==source || !*base || base-source>=sizeof(parent))goto cleanup;
    memcpy(parent,source,(size_t)(base-source));parent[base-source]=0;
    if(strlen(parent)>1 && parent[strlen(parent)-1]=='/')parent[strlen(parent)-1]=0;
    if(last_value("NovaDock/ArchiveFormat",remembered,sizeof(remembered))) {
        if(!strcmp(remembered,"LHA") && (mask&2))format=1;
        else if(!strcmp(remembered,"ZIP") && (mask&1))format=0;
    }
    if(last_value("NovaDock/ArchiveSaveDrawer",remembered,sizeof(remembered))) {
        {struct Process *self=(struct Process *)FindTask(NULL);APTR old=self->pr_WindowPtr;
        self->pr_WindowPtr=(APTR)-1;lock=Lock(remembered,ACCESS_READ);self->pr_WindowPtr=old;}
        if(lock){
            fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
            if(fib && Examine(lock,fib) && fib->fib_DirEntryType>0 &&
               NameFromLock(lock,canonical,sizeof(canonical)))strcpy(parent,canonical);
            if(fib)FreeDosObject(DOS_FIB,fib);UnLock(lock);
        }
    }
    icon=last_bool("NovaDock/ArchiveCreateIcon",1);
    open=last_bool("NovaDock/ArchiveOpenDestination",0);
    name[0]=0;
    if(!require_name && !nd_archive_output_name(base,format ? ND_ARCHIVE_LHA:ND_ARCHIVE_ZIP,name,sizeof(name)))goto cleanup;
    last=CreateContext(&gadgets);if(!last)goto cleanup;
    memset(&ng,0,sizeof(ng));ng.ng_VisualInfo=visual;ng.ng_TextAttr=&font;
    top=screen->WBorTop+screen->Font->ta_YSize+8;
    ng.ng_LeftEdge=12;ng.ng_TopEdge=top;ng.ng_Width=w-24;ng.ng_Height=12;
    if(require_name)sprintf(source_label,"%u selected files and drawers",count);
    else sprintf(source_label,"Source: %.60s",base);
    ng.ng_GadgetText="";
    last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)source_label,TAG_DONE);if(!last)goto cleanup;
    ng.ng_LeftEdge=100;ng.ng_TopEdge=top+22;ng.ng_Width=w-116;ng.ng_Height=18;
    ng.ng_Flags=PLACETEXT_LEFT;ng.ng_GadgetID=1;ng.ng_GadgetText="Format";
    cycle=last=CreateGadget(CYCLE_KIND,last,&ng,GTCY_Labels,(ULONG)formats,GTCY_Active,format,TAG_DONE);if(!last)goto cleanup;
    ng.ng_TopEdge+=27;ng.ng_GadgetID=2;ng.ng_GadgetText="Save drawer";ng.ng_Width=w-204;
    drawer=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)parent,GTST_MaxChars,255,TAG_DONE);if(!last)goto cleanup;
    ng.ng_LeftEdge=w-96;ng.ng_Width=80;ng.ng_GadgetID=4;ng.ng_GadgetText="Browse...";ng.ng_Flags=PLACETEXT_IN;
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
    ng.ng_LeftEdge=100;ng.ng_TopEdge+=27;ng.ng_Width=w-116;ng.ng_Flags=PLACETEXT_LEFT;
    ng.ng_GadgetID=3;ng.ng_GadgetText="Filename";
    file=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)name,GTST_MaxChars,155,TAG_DONE);if(!last)goto cleanup;
    ng.ng_TopEdge+=27;ng.ng_LeftEdge=100;ng.ng_Width=26;ng.ng_GadgetID=7;
    ng.ng_GadgetText="Create Workbench icon (.info)";ng.ng_Flags=PLACETEXT_RIGHT;
    last=CreateGadget(CHECKBOX_KIND,last,&ng,GTCB_Checked,icon,TAG_DONE);if(!last)goto cleanup;
    ng.ng_TopEdge+=24;ng.ng_GadgetID=8;ng.ng_GadgetText="Open destination when finished";
    last=CreateGadget(CHECKBOX_KIND,last,&ng,GTCB_Checked,open,TAG_DONE);if(!last)goto cleanup;
    ng.ng_TopEdge+=31;ng.ng_Width=104;ng.ng_LeftEdge=w-232;ng.ng_Flags=PLACETEXT_IN;
    ng.ng_GadgetID=5;ng.ng_GadgetText="Compress";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
    ng.ng_LeftEdge=w-116;ng.ng_GadgetID=6;ng.ng_GadgetText="Cancel";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
    win=OpenWindowTags(NULL,WA_Title,(ULONG)"Archiver - Create archive",
        WA_PubScreen,(ULONG)screen,WA_Left,(screen->Width-w)/2,WA_Top,screen->BarHeight+16,
        WA_Width,w,WA_Height,ng.ng_TopEdge+ng.ng_Height+12,
        WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Gadgets,(ULONG)gadgets,TAG_DONE);
    if(!win)goto cleanup;GT_RefreshWindow(win,NULL);result=0;
    if(require_name)ActivateGadget(file,win,NULL);
    while(!done){
        Wait(1UL<<win->UserPort->mp_SigBit);
        while((msg=GT_GetIMsg(win->UserPort))!=NULL){
            cls=msg->Class;code=msg->Code;g=cls==IDCMP_GADGETUP ? (struct Gadget *)msg->IAddress:NULL;id=g ? g->GadgetID:0;
            GT_ReplyIMsg(msg);
            if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45)){done=1;break;}
            if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(win);GT_EndRefresh(win,TRUE);}
            if(cls!=IDCMP_GADGETUP)continue;
            if(id==6){done=1;break;}
            if(id==8){open=code!=0;}
            else if(id==7){icon=code!=0;}
            else if(id==1){
                format=code ? 1:0;
                if(nd_archive_output_name(field(file),format ? ND_ARCHIVE_LHA:ND_ARCHIVE_ZIP,name,sizeof(name)))
                    GT_SetGadgetAttrs(file,win,NULL,GTST_String,(ULONG)name,TAG_DONE);
            } else if(id==4){
                request=AllocAslRequestTags(ASL_FileRequest,TAG_DONE);
                if(request){
                    if(AslRequestTags(request,ASLFR_Window,(ULONG)win,ASLFR_TitleText,(ULONG)"Save archive",
                        ASLFR_DoSaveMode,TRUE,ASLFR_InitialDrawer,(ULONG)field(drawer),
                        ASLFR_InitialFile,(ULONG)field(file),TAG_DONE)){
                        GT_SetGadgetAttrs(drawer,win,NULL,GTST_String,(ULONG)request->fr_Drawer,TAG_DONE);
                        GT_SetGadgetAttrs(file,win,NULL,GTST_String,(ULONG)request->fr_File,TAG_DONE);
                    }
                    FreeAslRequest(request);
                    GT_RefreshWindow(win,NULL);
                }
            } else if(id==5){
                if(!(mask & (format ? 2:1))){notice(win,format ? "Install C:LhA to create LHA archives.":"Install C:Zip to create ZIP archives.");continue;}
                if(!nd_archive_output_name(field(file),format ? ND_ARCHIVE_LHA:ND_ARCHIVE_ZIP,filename,sizeof(filename))){notice(win,"Enter a valid archive filename.");continue;}
                lock=Lock((STRPTR)field(drawer),ACCESS_READ);
                if(!lock){notice(win,"Choose an existing destination drawer.");continue;}
                fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
                is_drawer=fib && Examine(lock,fib) && fib->fib_DirEntryType>0;
                if(fib)FreeDosObject(DOS_FIB,fib);
                if(!is_drawer){UnLock(lock);notice(win,"Choose a drawer, not a file.");continue;}
                canonical[0]=0;
                if(!NameFromLock(lock,canonical,sizeof(canonical))){UnLock(lock);notice(win,"The destination path is too long.");continue;}
                UnLock(lock);
                if(strlen(canonical)>=size){notice(win,"The destination path is too long.");continue;}
                strcpy(output,canonical);
                if(!AddPart(output,filename,size) || !nd_archive_safe_path(output)){notice(win,"The destination path cannot be used.");continue;}
                for(item=0;item<count;++item)
                    if(!nd_archive_output_allowed(sources[item].path,sources[item].drawer,output))break;
                if(item<count){
                    notice(win,"Choose a destination outside the source drawer, with a different filename.");continue;
                }
                lock=Lock(output,ACCESS_READ);
                if(lock){UnLock(lock);notice(win,"That archive already exists. Choose another name.");continue;}
                save_failed=0;
                save_value("NovaDock/ArchiveFormat",format ? "LHA":"ZIP");
                save_value("NovaDock/ArchiveSaveDrawer",canonical);
                save_bool("NovaDock/ArchiveCreateIcon",icon);
                save_bool("NovaDock/ArchiveOpenDestination",open);
                if(save_failed)notice(win,"The operation will continue, but the choices could not be saved.");
                *kind=format ? ND_ARCHIVE_LHA:ND_ARCHIVE_ZIP;*create_icon=icon;result=1;done=1;break;
            }
        }
    }
cleanup:
    if(win)CloseWindow(win);
    if(gadgets)FreeGadgets(gadgets);
    if(visual)FreeVisualInfo(visual);
    if(screen)UnlockPubScreen(NULL,screen);
    if(AslBase){CloseLibrary(AslBase);AslBase=NULL;}
    if(GadToolsBase){CloseLibrary(GadToolsBase);GadToolsBase=NULL;}
    if(IntuitionBase){CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;}
    return result;
}

int nd_archive_extraction_request(const char *archive,char *destination,
                                  unsigned int size,int *create_drawer,int *create_icon)
{
    struct Screen *screen=NULL;struct Window *win=NULL;
    struct Gadget *gadgets=NULL,*last,*drawer,*file,*icon_gadget,*g;
    struct NewGadget ng;struct TextAttr font={(STRPTR)"topaz.font",8,0,0};
    struct IntuiMessage *msg;struct FileRequester *request;
    struct FileInfoBlock *fib;struct EasyStruct e;
    APTR visual=NULL;BPTR lock;int is_drawer;
    char parent[260],suggested[260],canonical[260],source_label[76];
    const char *base=FilePart((STRPTR)archive);
    ULONG cls;UWORD id,code;int w,top,make=1,result=-1,done=0,icon=1,open=0;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);AslBase=OpenLibrary("asl.library",39);
    if(!IntuitionBase || !GadToolsBase || !AslBase)goto cleanup;
    screen=LockPubScreen("Workbench");if(!screen)goto cleanup;
    w=screen->Width-24;if(w>560)w=560;if(w<400)goto cleanup;
    visual=GetVisualInfo(screen,TAG_DONE);if(!visual)goto cleanup;
    if(!base || base==archive || !*base || base-archive>=sizeof(parent))goto cleanup;
    memcpy(parent,archive,(size_t)(base-archive));parent[base-archive]=0;
    if(strlen(parent)>1 && parent[strlen(parent)-1]=='/')parent[strlen(parent)-1]=0;
    if(!nd_archive_destination(archive,suggested,sizeof(suggested))){
        if(strlen(base)+10>=sizeof(suggested))goto cleanup;
        strcpy(suggested,base);strcat(suggested,".unpacked");
    }else memmove(suggested,FilePart(suggested),strlen(FilePart(suggested))+1);
    icon=last_bool("NovaDock/ArchiveExtractIcon",1);
    open=last_bool("NovaDock/ArchiveOpenDestination",0);
    last=CreateContext(&gadgets);if(!last)goto cleanup;
    memset(&ng,0,sizeof(ng));ng.ng_VisualInfo=visual;ng.ng_TextAttr=&font;
    top=screen->WBorTop+screen->Font->ta_YSize+8;
    ng.ng_LeftEdge=12;ng.ng_TopEdge=top;ng.ng_Width=w-24;ng.ng_Height=12;
    sprintf(source_label,"Archive: %.59s",base);ng.ng_GadgetText="";
    last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)source_label,TAG_DONE);if(!last)goto cleanup;
    ng.ng_LeftEdge=100;ng.ng_TopEdge=top+22;ng.ng_Width=w-204;ng.ng_Height=18;
    ng.ng_Flags=PLACETEXT_LEFT;ng.ng_GadgetID=2;ng.ng_GadgetText="Extract to";
    drawer=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)parent,GTST_MaxChars,255,TAG_DONE);if(!last)goto cleanup;
    ng.ng_LeftEdge=w-96;ng.ng_Width=80;ng.ng_GadgetID=4;ng.ng_GadgetText="Browse...";ng.ng_Flags=PLACETEXT_IN;
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
    ng.ng_LeftEdge=100;ng.ng_TopEdge+=27;ng.ng_Width=26;ng.ng_GadgetID=1;
    ng.ng_GadgetText="Create new drawer";ng.ng_Flags=PLACETEXT_RIGHT;
    last=CreateGadget(CHECKBOX_KIND,last,&ng,GTCB_Checked,TRUE,TAG_DONE);if(!last)goto cleanup;
    ng.ng_TopEdge+=27;ng.ng_Width=w-116;ng.ng_Flags=PLACETEXT_LEFT;
    ng.ng_GadgetID=3;ng.ng_GadgetText="Drawer name";
    file=last=CreateGadget(STRING_KIND,last,&ng,GTST_String,(ULONG)suggested,GTST_MaxChars,155,TAG_DONE);if(!last)goto cleanup;
    ng.ng_TopEdge+=27;ng.ng_LeftEdge=100;ng.ng_Width=26;ng.ng_GadgetID=7;
    ng.ng_GadgetText="Create Workbench icon (.info)";ng.ng_Flags=PLACETEXT_RIGHT;
    icon_gadget=last=CreateGadget(CHECKBOX_KIND,last,&ng,GTCB_Checked,icon,TAG_DONE);if(!last)goto cleanup;
    ng.ng_TopEdge+=24;ng.ng_GadgetID=8;ng.ng_GadgetText="Open destination when finished";
    last=CreateGadget(CHECKBOX_KIND,last,&ng,GTCB_Checked,open,TAG_DONE);if(!last)goto cleanup;
    ng.ng_TopEdge+=31;ng.ng_Width=104;ng.ng_LeftEdge=w-232;ng.ng_Flags=PLACETEXT_IN;
    ng.ng_GadgetID=5;ng.ng_GadgetText="Extract";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
    ng.ng_LeftEdge=w-116;ng.ng_GadgetID=6;ng.ng_GadgetText="Cancel";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
    win=OpenWindowTags(NULL,WA_Title,(ULONG)"Archiver - Extract archive",
        WA_PubScreen,(ULONG)screen,WA_Left,(screen->Width-w)/2,WA_Top,screen->BarHeight+16,
        WA_Width,w,WA_Height,ng.ng_TopEdge+ng.ng_Height+12,
        WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,
        WA_Gadgets,(ULONG)gadgets,TAG_DONE);
    if(!win)goto cleanup;GT_RefreshWindow(win,NULL);result=0;
    while(!done){
        Wait(1UL<<win->UserPort->mp_SigBit);
        while((msg=GT_GetIMsg(win->UserPort))!=NULL){
            cls=msg->Class;code=msg->Code;g=cls==IDCMP_GADGETUP ? (struct Gadget *)msg->IAddress:NULL;id=g ? g->GadgetID:0;
            GT_ReplyIMsg(msg);
            if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45)){done=1;break;}
            if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(win);GT_EndRefresh(win,TRUE);}
            if(cls!=IDCMP_GADGETUP)continue;
            if(id==6){done=1;break;}
            if(id==8){open=code!=0;}
            else if(id==7){icon=code!=0;}
            else if(id==1){make=code!=0;
                GT_SetGadgetAttrs(file,win,NULL,GA_Disabled,!make,TAG_DONE);
                GT_SetGadgetAttrs(icon_gadget,win,NULL,GA_Disabled,!make,TAG_DONE);
            }
            else if(id==4){
                request=AllocAslRequestTags(ASL_FileRequest,TAG_DONE);
                if(request){
                    if(AslRequestTags(request,ASLFR_Window,(ULONG)win,ASLFR_TitleText,(ULONG)"Extract to drawer",
                        ASLFR_DrawersOnly,TRUE,ASLFR_InitialDrawer,(ULONG)field(drawer),TAG_DONE))
                        GT_SetGadgetAttrs(drawer,win,NULL,GTST_String,(ULONG)request->fr_Drawer,TAG_DONE);
                    FreeAslRequest(request);GT_RefreshWindow(win,NULL);
                }
            }else if(id==5){
                lock=Lock((STRPTR)field(drawer),ACCESS_READ);
                if(!lock){notice(win,"Choose an existing destination drawer.");continue;}
                fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
                is_drawer=fib && Examine(lock,fib) && fib->fib_DirEntryType>0;
                if(fib)FreeDosObject(DOS_FIB,fib);
                canonical[0]=0;
                if(!is_drawer || !NameFromLock(lock,canonical,sizeof(canonical))){
                    UnLock(lock);notice(win,"Choose a valid destination drawer.");continue;
                }
                UnLock(lock);
                if(!nd_archive_extract_destination(canonical,make,field(file),destination,size)){
                    notice(win,"Enter a valid destination and drawer name.");continue;
                }
                if(make){
                    lock=Lock(destination,ACCESS_READ);
                    if(lock){UnLock(lock);notice(win,"That drawer already exists. Choose another name or destination.");continue;}
                }else{
                    memset(&e,0,sizeof(e));e.es_StructSize=sizeof(e);e.es_Title=(STRPTR)"Extract into existing drawer";
                    e.es_TextFormat=(STRPTR)"Files will be extracted directly into this drawer.\nMatching files may be replaced by the unpacker.";
                    e.es_GadgetFormat=(STRPTR)"Extract|Cancel";
                    if(!EasyRequestArgs(win,&e,NULL,NULL))continue;
                }
                save_failed=0;
                save_bool("NovaDock/ArchiveExtractIcon",icon);
                save_bool("NovaDock/ArchiveOpenDestination",open);
                if(save_failed)notice(win,"The operation will continue, but the choices could not be saved.");
                *create_drawer=make;*create_icon=make && icon;result=1;done=1;break;
            }
        }
    }
cleanup:
    if(win)CloseWindow(win);
    if(gadgets)FreeGadgets(gadgets);
    if(visual)FreeVisualInfo(visual);
    if(screen)UnlockPubScreen(NULL,screen);
    if(AslBase){CloseLibrary(AslBase);AslBase=NULL;}
    if(GadToolsBase){CloseLibrary(GadToolsBase);GadToolsBase=NULL;}
    if(IntuitionBase){CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;}
    return result;
}
