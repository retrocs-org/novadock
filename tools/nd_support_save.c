/* Native report destination chooser and optional LhA export. */
#define GfxBase NDSupportSaveGfxBase
#define AslBase NDSupportSaveAslBase
#define IntuitionBase NDSupportSaveIntuitionBase
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "nd_support_save.h"
#include "nd_interaction.h"
struct Library *AslBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
static struct Window *progress_window;
void nd_support_save_progress(const char *text)
{
    struct Screen *screen;struct RastPort *rp;struct DrawInfo *info;int n,x,y,w;
    if(!text){
        if(progress_window)CloseWindow(progress_window);progress_window=NULL;
        if(GfxBase)CloseLibrary((struct Library *)GfxBase);
        GfxBase=NULL;
        if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
        IntuitionBase=NULL;
        return;
    }
    if(!progress_window){
        IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
        GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
        if(!IntuitionBase || !GfxBase){nd_support_save_progress(NULL);return;}
        screen=LockPubScreen("Workbench");if(!screen){nd_support_save_progress(NULL);return;}
        w=screen->Width-20;if(w>480)w=480;
        progress_window=OpenWindowTags(NULL,WA_PubScreen,(ULONG)screen,
            WA_Title,(ULONG)"Saving NovaDock report",WA_Left,(screen->Width-w)/2,
            WA_Top,screen->BarHeight+12,WA_Width,w,WA_InnerHeight,screen->RastPort.TxHeight+20,
            WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_Flags,WFLG_SMART_REFRESH,TAG_DONE);
        UnlockPubScreen(NULL,screen);
        if(!progress_window){nd_support_save_progress(NULL);return;}
    }
    rp=progress_window->RPort;info=GetScreenDrawInfo(progress_window->WScreen);
    if(!info)return;
    SetAPen(rp,info->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp,progress_window->BorderLeft,progress_window->BorderTop,
        progress_window->Width-progress_window->BorderRight-1,progress_window->Height-progress_window->BorderBottom-1);
    SetAPen(rp,info->dri_Pens[TEXTPEN]);SetDrMd(rp,JAM1);
    x=progress_window->BorderLeft+10;y=progress_window->BorderTop+10+rp->TxBaseline;
    n=(int)strlen(text);while(n && TextLength(rp,(STRPTR)text,n)>progress_window->Width-x-10)--n;
    Move(rp,x,y);Text(rp,(STRPTR)text,n);FreeScreenDrawInfo(progress_window->WScreen,info);WaitBlit();
}
int nd_support_find_lha(char *path,unsigned int size)
{
    static const char *const names[]={"C:LhA","PROGDIR:LhA",NULL};
    struct Process *self=(struct Process *)FindTask(NULL);APTR requesters=self->pr_WindowPtr;
    BPTR lock;struct FileInfoBlock *fib;int i,found=0;
    self->pr_WindowPtr=(APTR)-1L;
    fib=AllocDosObject(DOS_FIB,NULL);
    if(fib){for(i=0;names[i]&&!found;++i){
        lock=Lock((STRPTR)names[i],ACCESS_READ);if(!lock)continue;
        found=Examine(lock,fib)&&fib->fib_DirEntryType<0&&NameFromLock(lock,path,size);UnLock(lock);
    }FreeDosObject(DOS_FIB,fib);}
    self->pr_WindowPtr=requesters;return found;
}
static int archive_suffix(const char *name)
{
    unsigned int n=(unsigned int)strlen(name);
    return n>=4&&name[n-4]=='.'&&tolower((unsigned char)name[n-3])=='l'&&
        tolower((unsigned char)name[n-2])=='h'&&tolower((unsigned char)name[n-1])=='a';
}
int nd_support_select_save(const char *initial,const char *name,int archive,char *path,unsigned int size)
{
    struct FileRequester *request;BPTR lock;int result=-1,selected;char filename[256],drawer[512];
    static char last_drawer[512],last_file[256];
    struct EasyStruct notice={sizeof(notice),0,"Save NovaDock report","%s","OK"};const char *message;
    AslBase=OpenLibrary("asl.library",39);IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
    if(!AslBase||!IntuitionBase)goto done;
    request=AllocAslRequestTags(ASL_FileRequest,TAG_DONE);if(!request)goto done;
    sprintf(filename,"%.64s.lha",name);
    if(last_file[0])strcpy(filename,last_file);
    if(strlen(initial)>=sizeof(drawer))goto free_request;
    strcpy(drawer,last_drawer[0]?last_drawer:initial);
    for(;;){
        selected=AslRequestTags(request,ASLFR_TitleText,(ULONG)(archive?"Save NovaDock report archive":"Save NovaDock report drawer"),
            ASLFR_PositiveText,(ULONG)"Save",ASLFR_DoSaveMode,(ULONG)archive,ASLFR_DrawersOnly,(ULONG)!archive,
            ASLFR_InitialDrawer,(ULONG)drawer,ASLFR_InitialFile,(ULONG)(archive?filename:""),TAG_DONE);
        if(strlen(request->fr_Drawer)<sizeof(drawer)){
            strcpy(drawer,request->fr_Drawer);strcpy(last_drawer,drawer);
        }
        if(archive && strlen(request->fr_File)<sizeof(filename)){
            strcpy(filename,request->fr_File);strcpy(last_file,filename);
        }
        if(!selected){result=0;break;}
        lock=Lock(request->fr_Drawer,ACCESS_READ);if(!lock)break;
        result=NameFromLock(lock,path,size)?1:-1;UnLock(lock);if(result<0)break;
        if(!archive)break;
        if(!request->fr_File[0]||strchr(request->fr_File,':')||strchr(request->fr_File,'/')||
           !strcmp(request->fr_File,".")||!strcmp(request->fr_File,"..")||
           !AddPart(path,request->fr_File,size)||(!archive_suffix(path)&&strlen(path)+5>size)){
            message="Enter a filename for the report, for example NovaDock-report.lha.";
            EasyRequestArgs(NULL,&notice,NULL,(APTR)&message);result=-1;continue;
        }
        if(!archive_suffix(path))strcat(path,".lha");
        lock=Lock(path,ACCESS_READ);if(!lock)break;UnLock(lock);
        message="That filename already exists. Please choose another name.";
        EasyRequestArgs(NULL,&notice,NULL,(APTR)&message);result=-1;
    }
 free_request:
    FreeAslRequest(request);
 done:
    if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase=NULL;
    if(AslBase)CloseLibrary(AslBase);
    AslBase=NULL;return result;
}
int nd_support_unique_drawer(const char *parent,const char *name,char *path,unsigned int size)
{
    BPTR lock;int i;char leaf[96];
    for(i=0;i<20;++i){
        if(i)sprintf(leaf,"%.64s-%d",name,i);else sprintf(leaf,"%.64s",name);
        if(strlen(parent)>=size)return 0;strcpy(path,parent);
        if(!AddPart(path,leaf,size))return 0;
        lock=CreateDir(path);if(lock){UnLock(lock);return 1;}
        if(IoErr()!=ERROR_OBJECT_EXISTS)return 0;
    }
    return 0;
}
int nd_support_pack_report(const char *tool,const char *source,const char *destination)
{
    char parent[512],stage[512],archive[512],leaf[48],command[2200],qtool[540],qarchive[1040],qsource[160];
    char header[7];BPTR lock,old,in,out,file;LONG result;int ok=0;
    if(!source||!destination||strlen(source)>=sizeof(parent)||!strchr(source,':')||!strchr(destination,':'))return 0;
    lock=Lock((STRPTR)destination,ACCESS_READ);if(lock){UnLock(lock);return 0;}
    strcpy(parent,source);*PathPart(parent)=0;
    sprintf(leaf,"ND-report-stage-%08lx",(ULONG)FindTask(NULL));
    if(!nd_support_unique_drawer(parent,leaf,stage,sizeof(stage)))return 0;
    strcpy(archive,stage);if(!AddPart(archive,"report.lha",sizeof(archive)))goto done;
    if(!nd_quote_path(qtool,sizeof(qtool),tool)||!nd_quote_path(qarchive,sizeof(qarchive),archive)||
       !nd_quote_path(qsource,sizeof(qsource),FilePart((STRPTR)source)))goto done;
    lock=Lock(parent,ACCESS_READ);if(!lock)goto done;old=CurrentDir(lock);
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);result=-1;
    if(in&&out){sprintf(command,"%s -r a %s %s",qtool,qarchive,qsource);
        result=SystemTags(command,SYS_Input,in,SYS_Output,out,TAG_DONE);}
    if(in)Close(in);if(out)Close(out);
    UnLock(CurrentDir(old));
    if(result!=0)goto done;
    file=Open(archive,MODE_OLDFILE);if(!file)goto done;
    result=Read(file,header,sizeof(header));Close(file);
    if(result!=sizeof(header)||header[2]!='-'||header[3]!='l'||header[4]!='h'||header[6]!='-')goto done;
    lock=Lock((STRPTR)destination,ACCESS_READ);if(lock){UnLock(lock);goto done;}
    ok=Rename(archive,(STRPTR)destination)!=0;
 done:
    DeleteFile(archive);DeleteFile(stage);return ok;
}
void nd_support_discard_report(const char *path)
{
    static const char *const files[]={"capture.txt","capture.previous.txt","plugin-samples.txt","plugin-samples.previous.txt","support.txt","problem.txt","dock.log","dock.log.previous",
        "prefs.log","prefs.log.previous","dopus4.log","dopus4.log.previous",
        "dock-disk.log","dock-disk.log.previous","prefs-disk.log","prefs-disk.log.previous",
        "dopus4-disk.log","dopus4-disk.log.previous",NULL};
    char file[512];int i;
    for(i=0;files[i];++i){if(strlen(path)>=sizeof(file))return;strcpy(file,path);
        if(AddPart(file,(STRPTR)files[i],sizeof(file)))DeleteFile(file);}
    {
        static const char *const components[]={"dock","prefs","dopus4"};
        static const char *const suffixes[]={"log","previous","txt"};
        int c,location,slot,g;char name[64];
        for(c=0;c<3;++c)for(location=0;location<2;++location){
            sprintf(name,"%s%s-session.txt",components[c],location?"-disk":"");strcpy(file,path);
            if(AddPart(file,name,sizeof(file)))DeleteFile(file);
            for(slot=1;slot<=4;++slot)for(g=0;g<3;++g){
                sprintf(name,"%s-%s-%d.%s",components[c],location?"disk":"ram",slot,suffixes[g]);strcpy(file,path);
                if(AddPart(file,name,sizeof(file)))DeleteFile(file);
            }
        }
    }
    DeleteFile((STRPTR)path);
}
