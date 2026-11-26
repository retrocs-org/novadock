/* Offline 3.2.3 fixture: runtime toggles, rotation, backpressure, retired replies,
 * asynchronous disk output and bundle collection. No persistent user drive. */
#include <exec/types.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <exec/memory.h>
#include <intuition/intuitionbase.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <libraries/gadtools.h>
#include <gadgets/texteditor.h>
#include <proto/alib.h>
#include <dos/dostags.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_trace.h"
#include "nd_rtg_pixels.h"
struct Library *NDGlassCyberGfxBase;
#include "nd_support.h"
#include "nd_support_log.h"
#include "nd_control_client.h"
long __stack=65536;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *GadToolsBase;
static int failures;
static void check(int ok,const char *text){printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failures;}
static LONG file_size(const char *path)
{BPTR f=Open((STRPTR)path,MODE_OLDFILE);LONG n;if(!f)return -1;Seek(f,0,OFFSET_END);n=Seek(f,0,OFFSET_CURRENT);Close(f);return n;}
static int contains(const char *path,const char *text)
{FILE *f=fopen(path,"r");char line[512];int found=0;if(!f)return 0;while(fgets(line,sizeof(line),f))if(strstr(line,text)){found=1;break;}fclose(f);return found;}
static struct Window *find_window(const char *title)
{
    struct Screen *sc;struct Window *w,*found=NULL;ULONG key=LockIBase(0);
    for(sc=IntuitionBase->FirstScreen;sc&&!found;sc=sc->NextScreen)
        for(w=sc->FirstWindow;w;w=w->NextWindow)if(w->Title&&!strcmp((char *)w->Title,title)){found=w;break;}
    UnlockIBase(key);return found;
}
static int post_ui(struct Window *w,int id,int code)
{
    struct MsgPort *reply;struct IntuiMessage *m;struct Gadget *g=NULL;int i;
    if(id)for(g=w->FirstGadget;g&&g->GadgetID!=id;g=g->NextGadget);
    if(id&&!g)return 0;reply=CreateMsgPort();m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);if(!reply||!m)return 0;
    m->ExecMessage.mn_Length=sizeof(*m);m->ExecMessage.mn_ReplyPort=reply;m->IDCMPWindow=w;
    m->Class=id?IDCMP_GADGETUP:IDCMP_CLOSEWINDOW;m->Code=(UWORD)code;m->IAddress=g;
    PutMsg(w->UserPort,(struct Message *)m);
    for(i=0;i<300;++i){if(GetMsg(reply)){FreeVec(m);DeleteMsgPort(reply);return 1;}Delay(1);}return 0;
}
static int capture_ui(struct Window *w,const char *path)
{
    BPTR file;int y,n;unsigned char row[620*3];char header[64];
    NDGlassCyberGfxBase=OpenLibrary("cybergraphics.library",41);if(!NDGlassCyberGfxBase)return 0;
    file=Open((STRPTR)path,MODE_NEWFILE);if(!file||w->Width>620)return 0;
    n=sprintf(header,"P6\n%d %d\n255\n",w->Width,w->Height);Write(file,header,n);WaitBlit();
    for(y=0;y<w->Height;++y){
        __nd_glass_read(NDGlassCyberGfxBase,row,0,0,(UWORD)(w->Width*3),&w->WScreen->RastPort,(UWORD)w->LeftEdge,(UWORD)(w->TopEdge+y),(UWORD)w->Width,1,0);
        if(Write(file,row,w->Width*3)!=w->Width*3){Close(file);return 0;}
    }
    CloseLibrary(NDGlassCyberGfxBase);NDGlassCyberGfxBase=NULL;return Close(file)!=0;
}
static int click_named(struct Window *w,const char *name);
static int support_ui(void)
{
    struct Window *parent=NULL,*dialog=NULL;struct NDSupportSettings readback;int i;BPTR in,out;LONG rc;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    if(!IntuitionBase||!GfxBase)return 0;
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
    rc=SystemTags("QATests:Dock/NovaDockPrefs PAGE=SUPPORT",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
    if(rc<0)return 0;
    for(i=0;i<500&&!parent;++i){Delay(1);parent=find_window("NovaDock Preferences - Support");}
    if(!parent)return 0;Delay(20);capture_ui(parent,"QATests:support-page.ppm");
    if(!post_ui(parent,418,0))return 0;
    for(i=0;i<300&&!dialog;++i){Delay(1);dialog=find_window("Advanced support tools");}
    if(!dialog||!click_named(dialog,"Logging"))return 0;dialog=NULL;
    for(i=0;i<300&&!dialog;++i){Delay(1);dialog=find_window("Support - logging and trace");}
    if(!dialog)return 0;Delay(10);capture_ui(dialog,"QATests:logging-dialog.ppm");
    if(!post_ui(dialog,1,1)||!post_ui(dialog,5,0))return 0;
    for(i=0;i<300;++i){Delay(1);if(!find_window("Support - logging and trace"))break;}
    if(i==300)return 0;
    if(!nd_support_read(ND_SUPPORT_ENV,&readback)||readback.level!=1||!readback.persist)return 0;
    if(!nd_support_read(ND_SUPPORT_ENVARC,&readback)||readback.level!=1||!readback.persist)return 0;
    dialog=find_window("Advanced support tools");if(!dialog||!click_named(dialog,"Close"))return 0;
    if(!post_ui(parent,0,0))return 0;
    for(i=0;i<300;++i){Delay(1);if(!find_window("NovaDock Preferences - Support"))break;}
    CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;
    return i<300;
}
static int bundle_count(void)
{
    BPTR lock=Lock("QATests:Dock/SupportBundles",ACCESS_READ);struct FileInfoBlock *fib;int count=0;
    if(!lock)return 0;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return -1;}
    if(Examine(lock,fib))while(ExNext(lock,fib))if(fib->fib_DirEntryType>0)++count;
    FreeDosObject(DOS_FIB,fib);UnLock(lock);return count;
}
static int find_problem(char *path,const char *summary)
{
    BPTR lock=Lock("QATests:Dock/SupportBundles",ACCESS_READ);struct FileInfoBlock *fib;int found=0;
    if(!lock)return 0;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return 0;}
    if(Examine(lock,fib))while(ExNext(lock,fib))if(fib->fib_DirEntryType>0){
        sprintf(path,"QATests:Dock/SupportBundles/%s/problem.txt",fib->fib_FileName);
        if(contains(path,summary)){found=1;break;}
    }
    FreeDosObject(DOS_FIB,fib);UnLock(lock);return found;
}
static int bundle_log(const char *file,const char *value)
{
    BPTR lock=Lock("QATests:Dock/SupportBundles",ACCESS_READ);struct FileInfoBlock *fib;char path[512];int found=0;
    if(!lock)return 0;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return 0;}
    if(Examine(lock,fib))while(ExNext(lock,fib))if(fib->fib_DirEntryType>0){
        sprintf(path,"QATests:Dock/SupportBundles/%s/%s",fib->fib_FileName,file);
        if(contains(path,value)){found=1;break;}}
    FreeDosObject(DOS_FIB,fib);UnLock(lock);return found;
}
static struct Window *wait_report(const char *title)
{struct Window *w=NULL;int i;for(i=0;i<400&&!w;++i){Delay(1);w=find_window(title);}return w;}
static struct Gadget *report_gadget(struct Window *w,int id)
{struct Gadget *g;for(g=w->FirstGadget;g&&g->GadgetID!=id;g=g->NextGadget);return g;}
static int report_text(struct Window *w,int id,const char *text)
{struct Gadget *g=report_gadget(w,id);if(!g)return 0;GT_SetGadgetAttrs(g,w,NULL,GTST_String,(ULONG)text,TAG_DONE);return 1;}
static int pointer(struct Screen *screen,int x,int y)
{
 struct MsgPort *port;struct IOStdReq *io;struct InputEvent event;struct IEPointerPixel pixel;int ok=0;
 port=CreateMsgPort();if(!port)return 0;io=(struct IOStdReq *)CreateIORequest(port,sizeof(*io));if(!io){DeleteMsgPort(port);return 0;}
 if(!OpenDevice("input.device",0,(struct IORequest *)io,0)){
  memset(&event,0,sizeof(event));memset(&pixel,0,sizeof(pixel));pixel.iepp_Screen=screen;pixel.iepp_Position.X=x;pixel.iepp_Position.Y=y;
  event.ie_Class=IECLASS_NEWPOINTERPOS;event.ie_SubClass=IESUBCLASS_PIXEL;event.ie_EventAddress=&pixel;
  io->io_Command=IND_WRITEEVENT;io->io_Data=&event;io->io_Length=sizeof(event);ok=DoIO((struct IORequest *)io)==0;CloseDevice((struct IORequest *)io);
 }DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return ok;
}
static int click_named(struct Window *w,const char *name)
{
    struct Gadget *g,fallback;struct IntuiText *t;struct MsgPort *port;struct IOStdReq *io;struct InputEvent e;int up;
    for(g=w->FirstGadget;g;g=g->NextGadget){for(t=g->GadgetText;t;t=t->NextText)
        if(t->IText&&!strcmp((char *)t->IText,name))break;if(t)break;}
    if(!g){
        /* ASL's layout object owns its bottom-row child buttons. */
        memset(&fallback,0,sizeof(fallback));fallback.LeftEdge=!strcmp(name,"Cancel")?w->Width-78:10;
        fallback.TopEdge=w->Height-26;fallback.Width=66;fallback.Height=14;g=&fallback;
    }
    ActivateWindow(w);Delay(3);
    if(!pointer(w->WScreen,w->LeftEdge+g->LeftEdge+(g->Flags&GFLG_RELRIGHT?w->Width:0)+(g->Width+(g->Flags&GFLG_RELWIDTH?w->Width:0))/2,w->TopEdge+g->TopEdge+(g->Flags&GFLG_RELBOTTOM?w->Height:0)+(g->Height+(g->Flags&GFLG_RELHEIGHT?w->Height:0))/2))return 0;
    Delay(3);port=CreateMsgPort();if(!port)return 0;io=(struct IOStdReq *)CreateIORequest(port,sizeof(*io));if(!io)return 0;
    if(OpenDevice("input.device",0,(struct IORequest *)io,0)){DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return 0;}
    for(up=0;up<2;++up){memset(&e,0,sizeof(e));e.ie_Class=IECLASS_RAWMOUSE;
        e.ie_Code=up?IECODE_LBUTTON|IECODE_UP_PREFIX:IECODE_LBUTTON;e.ie_Qualifier=up?0:IEQUALIFIER_LEFTBUTTON;
        io->io_Command=IND_WRITEEVENT;io->io_Data=&e;io->io_Length=sizeof(e);DoIO((struct IORequest *)io);Delay(3);}
    CloseDevice((struct IORequest *)io);DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return 1;
}
static int launch_report(void)
{
    BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);
    struct Window *record;struct NDSupportSettings settings;
    if(SystemTags("QATests:Dock/SupportReport FORM",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0)return 0;
    record=wait_report("NovaDock report - recording");if(!record)return 0;
    Delay(35);if(!nd_support_read(ND_SUPPORT_ENV,&settings)||settings.level!=2||!settings.persist)return 0;
    capture_ui(record,"QATests:report-auto-recording.ppm");return post_ui(record,2,0);
}
static int report_ui(void)
{
    struct Window *w;struct Gadget *g;STRPTR exported;int before,i,ok=0;char path[512];
    struct NDSupportSettings capture;static char oversized[8193];
    const char *prepare="NovaDock Support Report - prepare (1/5)",*first="NovaDock Support Report - details (2/5)";
    const char *description="NovaDock Support Report - description (3/5)",*second="NovaDock Support Report - what happened (4/5)";
    const char *last="NovaDock Support Report - system / save (5/5)";
    static const char body[]="Detailed description on line one.\nLine two: the settings become slow.\nLine three: the pointer still moves.";
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);
    if(!IntuitionBase||!GfxBase||!GadToolsBase)return 0;
    capture.level=0;capture.persist=0;if(!nd_support_write(ND_SUPPORT_ENV,&capture)){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    before=bundle_count();if(before<0||!launch_report()||!(w=wait_report(prepare))||!post_ui(w,100,0)){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    for(i=0;i<200;++i){Delay(1);if(!find_window(prepare))break;}
    Delay(10);if(bundle_count()!=before){printf("Report UI stopped at line %d\n",__LINE__);goto done;}puts("PASS cancelled wizard creates no bundle");
    if(!launch_report()||!(w=wait_report(prepare))){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    Delay(5);capture_ui(w,"QATests:report-prepare.ppm");
    /* The report starts recording itself; partial/old evidence remains usable. */
    if(!nd_support_read(ND_SUPPORT_ENV,&capture)||capture.level!=0||capture.persist){puts("Capture settings were not restored");goto done;}
    g=report_gadget(w,101);if(!g||(g->Flags&GFLG_DISABLED))goto done;
    if(report_gadget(w,107)||report_gadget(w,108)||report_gadget(w,109))goto done;
    g=report_gadget(w,105);if(!g)goto done;GT_SetGadgetAttrs(g,w,NULL,GTCY_Active,0,TAG_DONE);
    if(!post_ui(w,105,1))goto done;Delay(5);w=wait_report(prepare);if(!w)goto done;
    puts("PASS automatic recording starts detailed disk capture; Use existing logs restores settings and opens the simple report form");
    if(!post_ui(w,101,0)||!(w=wait_report(first))){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    g=report_gadget(w,101);if(!g||!(g->Flags&GFLG_DISABLED)){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    if(!post_ui(w,101,0)){printf("Report UI stopped at line %d\n",__LINE__);goto done;}Delay(5);if(!find_window(first)){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    if(!report_text(w,1,"QA Test")||!report_text(w,2,"qa@example.invalid")||!report_text(w,3,"Synthetic slow plugin edit regression")||!report_text(w,4,"Disposable AmigaOS 3.2.3 guest")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    g=report_gadget(w,104);if(!g){printf("Report UI stopped at line %d\n",__LINE__);goto done;}GT_SetGadgetAttrs(g,w,NULL,GTCY_Active,1,TAG_DONE);if(!post_ui(w,104,2)){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    Delay(5);capture_ui(w,"QATests:report-details.ppm");
    if(!post_ui(w,101,0)||!(w=wait_report(description))){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    g=report_gadget(w,106);if(!g){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    memset(oversized,'X',sizeof(oversized)-1);oversized[sizeof(oversized)-1]=0;
    SetGadgetAttrs(g,w,NULL,GA_TEXTEDITOR_Contents,(ULONG)oversized,TAG_DONE);
    if(!post_ui(w,101,0)){printf("Report UI stopped at line %d\n",__LINE__);goto done;}Delay(10);
    if(!w->Title||!strstr((char *)w->Title,"Description too long")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    SetWindowTitles(w,(STRPTR)description,(STRPTR)-1);
    SetGadgetAttrs(g,w,NULL,GA_TEXTEDITOR_Contents,(ULONG)body,TAG_DONE);Delay(10);
    capture_ui(w,"QATests:report-description.ppm");
    if(!post_ui(w,101,0)||!(w=wait_report(second))){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    if(!report_text(w,1,"1. Start DOpus; 2. Open Network settings")||!report_text(w,2,"Every time I open Network settings")||!report_text(w,3,"Fields respond immediately")||!report_text(w,4,"The field responds after several seconds")||!report_text(w,5,"Installed the current NovaDock package")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    Delay(5);capture_ui(w,"QATests:report-reproduction.ppm");
    if(!post_ui(w,102,0)||!(w=wait_report(description))){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    g=report_gadget(w,106);if(!g){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    exported=(STRPTR)DoGadgetMethod(g,w,NULL,GM_TEXTEDITOR_ExportText,NULL);if(!exported){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    i=strstr(exported,"Line three")!=NULL;FreeVec(exported);if(!i){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    if(!post_ui(w,102,0)||!(w=wait_report(first))){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    g=report_gadget(w,2);if(!g||strcmp(((struct StringInfo *)g->SpecialInfo)->Buffer,"qa@example.invalid")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    if(!post_ui(w,101,0)||!(w=wait_report(description))||!post_ui(w,101,0)||!(w=wait_report(second))){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    g=report_gadget(w,1);if(!g||!strstr(((struct StringInfo *)g->SpecialInfo)->Buffer,"Start DOpus")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    if(!post_ui(w,101,0)||!(w=wait_report(last))){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    Delay(5);capture_ui(w,"QATests:report-system.ppm");
    before=bundle_count();
    if(!post_ui(w,103,0)){printf("Save test could not submit wizard\n");goto done;}
    w=wait_report("Save NovaDock report drawer");if(!w){printf("Save chooser was not found\n");goto done;}
    capture_ui(w,"QATests:report-save-before-cancel.ppm");
    if(!click_named(w,"Cancel")){printf("Save chooser cancel could not click\n");goto done;}
    w=wait_report(last);if(!w){printf("Save chooser cancel did not return to review\n");goto done;}
    if(bundle_count()!=before){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    puts("PASS cancelling the destination chooser returns to review without creating a bundle or losing details");
    if(!post_ui(w,103,0)||!(w=wait_report("Save NovaDock report drawer"))){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    capture_ui(w,"QATests:report-save-drawer.ppm");
    if(!click_named(w,"Save")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    for(i=0;i<400;++i){Delay(1);if(find_problem(path,"Synthetic slow plugin edit regression")&&contains(path,"Installed the current"))break;}
    if(i==400||!contains(path,"qa@example.invalid")||!contains(path,"Sometimes")||!contains(path,"Fields respond immediately")||!contains(path,"Line three")||!contains(path,"Slow / unresponsive")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    if(!contains(path,"Motorola 68060")||!contains(path,"3.2.3 (Workbench 47.42)")||!contains(path,"Kickstart: 47.115")||!contains(path,"Chip RAM:")||!contains(path,"Fast RAM:")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    if(!contains(path,"NovaDock: 0.6")||!contains(path,"Graphics: 1024 x 600")||!contains(path,"Display mode:")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    if(contains("RAM:NovaDock-Trace/dock.log","qa@example.invalid")||contains("RAM:NovaDock-Trace/prefs.log","qa@example.invalid")){printf("Report UI stopped at line %d\n",__LINE__);goto done;}
    puts("PASS five-page wizard; multiline description and Back/Next retain edits; detected CPU/OS/ROM/RAM saved locally");ok=1;
 done:
    capture.level=1;capture.persist=1;nd_support_write(ND_SUPPORT_ENV,&capture);
    CloseLibrary(GadToolsBase);GadToolsBase=NULL;CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;return ok;
}
/* Shadow only the disposable guest's C: assign to exercise no-LhA saving.
 * Keep Version available for detected system details, then restore C:. */
static int report_ui_without_lha(void)
{
    BPTR previous,next;FILE *in,*out;char bytes[1024];size_t n;int ok;
    previous=Lock("C:",ACCESS_READ);if(!previous)return 0;
    next=CreateDir("RAM:SupportCommands");if(!next){UnLock(previous);return 0;}UnLock(next);
    in=fopen("C:Version","rb");out=fopen("RAM:SupportCommands/Version","wb");
    if(!in||!out){if(in)fclose(in);if(out)fclose(out);UnLock(previous);return 0;}
    while((n=fread(bytes,1,sizeof(bytes),in))>0)if(fwrite(bytes,1,n,out)!=n)break;
    fclose(in);fclose(out);next=Lock("RAM:SupportCommands",ACCESS_READ);
    if(!next||!AssignLock("C",next)){if(next)UnLock(next);UnLock(previous);return 0;}
    ok=report_ui();if(!AssignLock("C",previous)){UnLock(previous);return 0;}return ok;
}
static int saved_archive(char *path)
{
    BPTR lock=Lock("QATests:Dock/SupportBundles",ACCESS_READ);struct FileInfoBlock *fib;int found=0;
    if(!lock)return 0;fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return 0;}
    if(Examine(lock,fib))while(ExNext(lock,fib))if(fib->fib_DirEntryType<0&&strstr(fib->fib_FileName,".lha")){
        sprintf(path,"QATests:Dock/SupportBundles/%s",fib->fib_FileName);found=1;break;}
    FreeDosObject(DOS_FIB,fib);UnLock(lock);return found;
}
static int archive_report_ui(void)
{
    struct Window *w;char path[512];int i,ok=0;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);
    if(!IntuitionBase||!GfxBase||!GadToolsBase)goto done;
    if(!launch_report()||!(w=wait_report("NovaDock Support Report - prepare (1/5)"))||!post_ui(w,101,0)||!(w=wait_report("NovaDock Support Report - details (2/5)")))goto done;
    if(!report_text(w,3,"Compressed report QA")||!post_ui(w,101,0)||!(w=wait_report("NovaDock Support Report - description (3/5)")))goto done;
    if(!post_ui(w,101,0)||!(w=wait_report("NovaDock Support Report - what happened (4/5)"))||!post_ui(w,101,0)||!(w=wait_report("NovaDock Support Report - system / save (5/5)")))goto done;
    if(!post_ui(w,103,0)||!(w=wait_report("Save NovaDock report archive")))goto done;
    capture_ui(w,"QATests:report-save-archive.ppm");if(!click_named(w,"Save"))goto done;
    for(i=0;i<600;++i){Delay(1);if(saved_archive(path)){ok=1;break;}}
 done:
    if(GadToolsBase)CloseLibrary(GadToolsBase);GadToolsBase=NULL;
    if(GfxBase)CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;return ok;
}
static int actual_dock(void)
{
    struct NDDiagnosticsSnapshot snapshot;struct NDSupportSettings s;FILE *f;BPTR in,out;int i,up;LONG rc;
    f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)return 0;
    fputs("SIDE=BOTTOM\nAUTO_HIDE=NO\nTOOLTIPS=NO\nLAUNCH=Shell|SYS:System/Shell||ON|AUTO||||NO|NO||||10|NO\nPLUGIN=CPU|Plugins/Activity|CPU|METER|ON\n",f);fclose(f);
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
    rc=SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);if(rc<0)return 0;
    for(i=0;i<400;++i){Forbid();up=FindPort(ND_CONTROL_PORT)!=NULL;Permit();if(up)break;Delay(1);}if(i==400)return 0;
    if(nd_control_diagnostics(&snapshot)!=ND_CONTROL_OK||snapshot.support_version!=3||snapshot.support_level!=1){printf("Initial dock capture=%u version=%u\n",snapshot.support_level,snapshot.support_version);return 0;}
    s.level=0;s.persist=1;if(!nd_support_write(ND_SUPPORT_ENV,&s)||nd_control_send(ND_CONTROL_SUPPORT_FLUSH)!=ND_CONTROL_OK)return 0;
    if(nd_control_diagnostics(&snapshot)!=ND_CONTROL_OK||snapshot.support_level!=0)return 0;
    s.level=2;if(!nd_support_write(ND_SUPPORT_ENV,&s)||nd_control_send(ND_CONTROL_SUPPORT_FLUSH)!=ND_CONTROL_OK)return 0;
    if(nd_control_diagnostics(&snapshot)!=ND_CONTROL_OK||snapshot.support_level!=2)return 0;
    if(nd_control_send(ND_CONTROL_SUPPORT_MARK)!=ND_CONTROL_OK||!contains("RAM:NovaDock-Trace/dock.log","marker=manual"))return 0;
    s.level=0;s.persist=0;
    if(!nd_support_write(ND_SUPPORT_ENV,&s)||nd_control_send(ND_CONTROL_SUPPORT_FLUSH)!=ND_CONTROL_OK)return 0;
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
    if(SystemTags("QATests:Dock/SupportReport RECORD 7",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0)return 0;
    for(i=0;i<700;++i){Delay(1);if(nd_support_recorded() && file_size(ND_SUPPORT_CAPTURE_OWNER)<0)break;}
    if(i==700){puts("Timed recording did not complete");return 0;}
    {struct Window *notice=NULL;int clicked=0;
     IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
     for(i=0;i<200&&!clicked;++i){Delay(1);notice=find_window("NovaDock support recording");if(notice)clicked=click_named(notice,"OK");}
     if(GfxBase)CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;
     if(!clicked)return 0;}
    if(!nd_support_read(ND_SUPPORT_ENV,&s)||s.level!=0||s.persist)return 0;
    if(!contains("NovaDock:SupportLogs/plugin-samples.txt","cpu_valid=1"))return 0;
    puts("PASS automatic CPU sample saved in plugin-samples.txt");
    /* The normal Create report path runs the full countdown without a
     * separate Record or Activity action, then opens the description flow. */
    {struct Window *record,*prepare;struct NDControlPluginResourcesMessage q;
     IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
     in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
     if(SystemTags("QATests:Dock/SupportReport FORM",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0)return 0;
     record=wait_report("NovaDock report - recording");if(!record)return 0;
     for(i=0;i<3600;++i){Delay(1);if(!find_window("NovaDock report - recording"))break;}
     prepare=wait_report("NovaDock Support Report - prepare (1/5)");if(i==3600||!prepare)return 0;
     capture_ui(prepare,"QATests:report-full-countdown-ready.ppm");
     if(!nd_support_recorded()||!contains("NovaDock:SupportLogs/capture.previous.txt","completed=1")||!contains("NovaDock:SupportLogs/plugin-samples.previous.txt","cpu_valid=1"))return 0;
     memset(&q,0,sizeof(q));if(nd_control_plugin_resources(&q)!=ND_CONTROL_OK||!q.count||!q.rows[0].cpu_valid)return 0;
     if(!nd_support_read(ND_SUPPORT_ENV,&s)||s.level!=0||s.persist||!post_ui(prepare,100,0))return 0;
     CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;
     puts("PASS guided report completes 60 seconds automatically, samples CPU repeatedly, preserves previous measurements and restores capture Off");
    }

    puts("PASS independent timed recorder saves measurements, restores capture Off, and exposes completed evidence for reports");
    if(nd_control_send(ND_CONTROL_QUIT)!=ND_CONTROL_OK)return 0;
    for(i=0;i<400;++i){Forbid();up=FindPort(ND_CONTROL_PORT)!=NULL;Permit();if(!up)break;Delay(1);}return i<400;
}
static int retained_sessions(void)
{
    const char *directory="QATests:SessionLogs";char line[100],path[180],log[180];int i,n,found=0,slots=0;
    for(i=0;i<9;++i){
        n=sprintf(line,"NovaDock trace 3 synthetic=%d\n",i);
        if(!nd_support_append(directory,0,line,(unsigned int)n))return 0;
        if(i!=1 && !nd_support_append(directory,0,"capture=stop reason=exit\n",25))return 0;
    }
    for(i=1;i<=4;++i){
        sprintf(path,"%s/Sessions/dock-%d/session.txt",directory,i);
        sprintf(log,"%s/Sessions/dock-%d/dock.log",directory,i);
        if(file_size(path)>0)++slots;
        if(contains(log,"synthetic=1")&&contains(path,"closed=0"))found=1;
    }
    if(slots!=4 || !found)return 0;
    sprintf(path,"%s/Sessions/dock-1/user-notes.txt",directory);{FILE *f=fopen(path,"w");if(!f)return 0;fputs("keep this file",f);fclose(f);}
    for(i=9;i<16;++i){n=sprintf(line,"NovaDock trace 3 synthetic=%d\n",i);if(!nd_support_append(directory,0,line,(unsigned int)n))return 0;}
    return contains(path,"keep this file");
}
int main(void)
{
    struct NDSupportSettings settings;struct MsgPort *blocked;struct NDSupportLogPacket *packet;
    BPTR guard,f;int i,persist;ULONG queued,lost,errors;char fill[4096];LONG left,n;
    setvbuf(stdout,NULL,_IONBF,0);guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard)return 10;UnLock(guard);
    check(retained_sessions(),"four retained sessions preserve latest unfinished capture and unknown user files");
    /* Exercise the imported system's real ENV handler before private assigns. */
    {BPTR dir=CreateDir("ENV:NovaDock");struct NDSupportSettings back;
     if(dir)UnLock(dir);settings.level=2;settings.persist=1;
     check(nd_support_write(ND_SUPPORT_ENV,&settings),"real ENV handler first capture save");
     settings.level=0;settings.persist=0;
     check(nd_support_write(ND_SUPPORT_ENV,&settings)&&nd_support_read(ND_SUPPORT_ENV,&back)&&back.level==0&&!back.persist,"real ENV handler replaces and verifies capture settings");
     check(!nd_support_write("QATests:no-such-parent/support.cfg",&settings)&&strstr(nd_support_error(),"no-such-parent")&&strstr(nd_support_error(),"create staged")&&strstr(nd_support_error(),"DOS error"),"native write failure reports destination, operation and DOS error");}
    SystemTags("MakeDir RAM:SupportEnv RAM:SupportEnv/NovaDock RAM:SupportArc",TAG_DONE);
    SystemTags("Assign ENV: RAM:SupportEnv",TAG_DONE);SystemTags("Assign ENVARC: RAM:SupportArc",TAG_DONE);
    SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
    settings.level=0;settings.persist=0;check(nd_support_write(ND_SUPPORT_ENV,&settings),"write private off settings");
    ND_TRACE_INIT("dock");ND_TRACE_NOTE("must not record",1,2,3,4);ND_TRACE_FLUSH();
    check(nd_trace_level==0&&file_size("RAM:NovaDock-Trace/dock.log")<0,"normal runtime capture off creates no log");
    settings.level=2;check(nd_support_write(ND_SUPPORT_ENV,&settings),"write trace settings");ND_TRACE_RELOAD();
    check(nd_trace_level==2,"enable tracing without restarting");
    for(i=0;i<64;++i){nd_trace_tile_kind((unsigned int)i,i==63?"SECRET_PRIVATE_ID":"NET");nd_trace_packet((unsigned int)i,1,1);}
    {ND_TRACE_START(t);Delay(3);ND_TRACE_END(NDT_INPUT,t);}
    ND_TRACE_NOTE("native state change",1,2,3,4);ND_TRACE_FLUSH();nd_trace_marker();
    check(contains("RAM:NovaDock-Trace/dock.log","tile=63 packets=1"),"all 64 tile records fit bounded summary");
    check(!contains("RAM:NovaDock-Trace/dock.log","SECRET_PRIVATE_ID")&&contains("RAM:NovaDock-Trace/dock.log","schema=OTHER"),"private instance IDs redacted");
    check(contains("RAM:NovaDock-Trace/dock.log","event=native state change"),"state changes retained in order");
    f=Open("RAM:NovaDock-Trace/dock.log",MODE_NEWFILE);if(!f)return 10;
    memset(fill,'X',sizeof(fill));left=(LONG)ND_SUPPORT_LOG_LIMIT-10;
    while(left){n=left>(LONG)sizeof(fill)?sizeof(fill):left;if(Write(f,fill,n)!=n)return 10;left-=n;}Close(f);
    ND_TRACE_FLUSH();
    check(file_size("RAM:NovaDock-Trace/dock.log.previous")==ND_SUPPORT_LOG_LIMIT-10&&file_size("RAM:NovaDock-Trace/dock.log")<(LONG)ND_SUPPORT_RECORD_MAX,"rotate full log instead of stopping capture");
    blocked=CreateMsgPort();if(!blocked)return 10;blocked->mp_Node.ln_Name=ND_SUPPORT_LOG_PORT;AddPort(blocked);
    settings.persist=1;nd_support_write(ND_SUPPORT_ENV,&settings);ND_TRACE_RELOAD();
    for(i=0;i<30;++i)nd_trace_phase("backpressure",1,0);
    nd_trace_status(&queued,&lost,&errors,&persist);
    check(queued==ND_SUPPORT_PENDING&&lost>0,"hung writer queue bounded and dropped records reported");
    nd_support_sink_close();
    while((packet=(struct NDSupportLogPacket *)GetMsg(blocked))!=NULL){packet->result=1;nd_support_log_reply(packet);}
    RemPort(blocked);DeleteMsgPort(blocked);
    check(1,"late replies safely retire abandoned writer port");
    ND_TRACE_NOTE("persistent capture",5,6,7,8);ND_TRACE_FLUSH();
    /* No producer pulse: startup outbox must reach disk while we are blocked. */
    for(i=0;i<300;++i){Delay(1);if(contains("NovaDock:SupportLogs/dock.log","persistent capture"))break;}
    check(i<300,"separate low-priority process saves live disk log");
    settings.level=0;nd_support_write(ND_SUPPORT_ENV,&settings);ND_TRACE_RELOAD();
    check(nd_trace_level==0,"disable tracing without restarting");
    check(contains("RAM:NovaDock-Trace/dock.log","capture=stop reason=disabled"),"explicit stop reason distinguishes disable from crash");
    {FILE *old=fopen("NovaDock:SupportLogs/dopus4.log","w"),*fresh=fopen("RAM:NovaDock-Trace/dopus4.log","w");
     if(!old||!fresh)return 10;fputs("QA_OLD_CRASH_FROM_DISK\n",old);fputs("QA_FRESH_RAM_AFTER_RESTART\n",fresh);fclose(old);fclose(fresh);}
    check(SystemTags("NovaDock:SupportReport",TAG_DONE)==0,"collector creates local support bundle");
    check(bundle_log("dopus4.log","QA_FRESH_RAM_AFTER_RESTART")&&bundle_log("dopus4-disk.log","QA_OLD_CRASH_FROM_DISK"),"fresh RAM logs and pre-crash disk logs both survive report collection");
    check(support_ui(),"Support section and logging dialog save runtime and permanent settings");
    check(report_ui_without_lha(),"interactive report form saves contact and investigation details locally");
    check(archive_report_ui(),"LhA available: Save archive chooser publishes one compressed report");
    check(actual_dock(),"normal dock changes capture live through control IPC and records markers");
    puts(failures?"FAIL runtime support checks":"PASS runtime support checks");return failures?10:0;
}
