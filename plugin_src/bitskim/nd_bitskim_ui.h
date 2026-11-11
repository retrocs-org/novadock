#ifndef ND_BITSKIM_UI_H
#define ND_BITSKIM_UI_H
/* Separate helper process: requester waits never block dock IPC. */
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL,*AslBase=NULL;
struct BSPicker {
    struct Pending request;struct NDBitSkimConfig config;
    char host[64],response[4096],path[1024],command[1200];
    char ids[32][33],labels[32][144];struct Node nodes[32];struct List list;
    int count;struct Screen *screen;
};
static void bs_notice(const char *text)
{
    struct EasyStruct es;ULONG args[1];
    memset(&es,0,sizeof(es));es.es_StructSize=sizeof(es);es.es_Title="NovaDock BitSkim";
    es.es_TextFormat="%s";es.es_GadgetFormat="OK";args[0]=(ULONG)text;
    EasyRequestArgs(NULL,&es,NULL,args);
}
static int bs_query(struct BSPicker *p,const char *command)
{
    int kind;LONG rc=20;
    if(!send_request(&p->request,p->host,command,DO_COMMAND,1))return 20;
    while(p->request.age<REPLY_TIMEOUT){
        kind=take_reply(&p->request,&rc,p->response,sizeof(p->response));if(kind)return (int)rc;
        Delay(1);++p->request.age;
    }
    bs_notice("BitSkim has not replied yet.\nClose any BitSkim requester and try again.\nThis does not mean that a transfer failed.");return -1;
}
static void bs_error(struct BSPicker *p)
{
    char encoded[770],decoded[256],local[256];
    if(!p->request.message && bs_query(p,"ERROR")==0 &&
       nd_bitskim_field(p->response,"TEXT",encoded,sizeof(encoded)) &&
       nd_bitskim_decode(encoded,decoded,sizeof(decoded))){
        nd_bitskim_local(local,sizeof(local),decoded);bs_notice(local[0] ? local:"BitSkim rejected the operation.");
    }else if(!p->request.message)bs_notice("BitSkim rejected the operation. Check its window for details.");
}
static int bs_peers(struct BSPicker *p)
{
    char key[16],ids[32][33],id[33],command[64],encoded[194],name[64],platform[32],local[96],address[48],localname[64];
    unsigned long count;int i,n,rc;
    p->list.lh_Head=(struct Node *)&p->list.lh_Tail;p->list.lh_Tail=NULL;p->list.lh_TailPred=(struct Node *)&p->list.lh_Head;p->count=0;
    rc=bs_query(p,"PEERS");if(rc)return 0;
    if(!nd_bitskim_number(p->response,"COUNT",&count) || count>32)return 0;
    for(i=0;i<(int)count;++i){sprintf(key,"ID%d",i);
        if(!nd_bitskim_field(p->response,key,ids[i],sizeof(ids[i])) || !nd_bitskim_hex(ids[i],32))return 0;
        for(n=0;n<i;++n)if(!strcmp(ids[n],ids[i]))return 0;
    }
    for(i=0;i<(int)count;++i){
        sprintf(command,"PEERINFO %s",ids[i]);rc=bs_query(p,command);
        if(p->request.message)return 0;if(rc)continue;
        if(!nd_bitskim_field(p->response,"ID",id,sizeof(id)) || strcmp(id,ids[i]))continue;
        if(!nd_bitskim_field(p->response,"NAME",encoded,sizeof(encoded)) || !nd_bitskim_decode(encoded,name,sizeof(name)))continue;
        if(!nd_bitskim_field(p->response,"PLATFORM",encoded,sizeof(encoded)) || !nd_bitskim_decode(encoded,platform,sizeof(platform)))continue;
        if(!nd_bitskim_field(p->response,"ADDRESS",encoded,sizeof(encoded)) || !nd_bitskim_decode(encoded,address,sizeof(address)))continue;
        n=p->count++;strcpy(p->ids[n],id);nd_bitskim_local(localname,sizeof(localname),name);
        nd_bitskim_local(local,sizeof(local),platform);
        sprintf(p->labels[n],"%.48s (%.16s) %.47s",localname,local,address);
        p->nodes[n].ln_Name=p->labels[n];AddTail(&p->list,&p->nodes[n]);
    }
    return 1;
}
static int bs_choose_file(struct BSPicker *p)
{
    struct FileRequester *fr;int ok=0;
    fr=AllocAslRequestTags(ASL_FileRequest,ASLFR_TitleText,(ULONG)"Choose one file to send",
        ASLFR_DoMultiSelect,FALSE,ASLFR_DoSaveMode,FALSE,ASLFR_PubScreenName,(ULONG)"Workbench",TAG_DONE);
    if(!fr)return 0;
    if(AslRequest(fr,NULL)){
        strncpy(p->path,fr->fr_Drawer,sizeof(p->path)-1);
        if(fr->fr_File[0] && AddPart(p->path,fr->fr_File,sizeof(p->path)))ok=1;
    }
    FreeAslRequest(fr);return ok;
}
static int bs_regular_file(const char *path)
{
    BPTR lock;struct FileInfoBlock *fib;int ok=0;
    lock=Lock((STRPTR)path,ACCESS_READ);if(!lock)return 0;
    fib=AllocDosObject(DOS_FIB,NULL);
    if(fib && Examine(lock,fib))ok=fib->fib_DirEntryType<0 && fib->fib_Size>=0;
    if(fib)FreeDosObject(DOS_FIB,fib);UnLock(lock);return ok;
}
static int bs_picker(struct BSPicker *p)
{
    struct Gadget *gadgets=NULL,*last,*list,*send;struct NewGadget ng;
    struct Window *win=NULL;struct IntuiMessage *im;APTR visual=NULL;
    int fx,fy,width,height,top,selected=-1,done=0,refresh=0,submit=0,rc,result=20;
    char line[160],chosen[33];ULONG kind;UWORD id,code;
    p->screen=LockPubScreen("Workbench");if(!p->screen)return 20;
    fx=p->screen->RastPort.Font->tf_XSize;fy=p->screen->RastPort.Font->tf_YSize;
    width=66*fx+32;if(width>p->screen->Width-16)width=p->screen->Width-16;
    height=14*fy+100;if(height>p->screen->Height-p->screen->BarHeight-12)height=p->screen->Height-p->screen->BarHeight-12;
    if(width<360 || height<160){bs_notice("The Workbench screen is too small for the recipient picker.");goto finish;}
    visual=GetVisualInfo(p->screen,TAG_DONE);if(!visual || !CreateContext(&gadgets))goto finish;last=gadgets;
    memset(&ng,0,sizeof(ng));ng.ng_TextAttr=p->screen->Font;ng.ng_VisualInfo=visual;
    ng.ng_LeftEdge=12;ng.ng_TopEdge=p->screen->BarHeight+8;ng.ng_Width=width-24;ng.ng_Height=fy+4;
    sprintf(line,"File: %.*s",(width-24)/fx-7,FilePart(p->path));
    last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)line,TAG_DONE);if(!last)goto finish;
    top=ng.ng_TopEdge+fy+8;ng.ng_TopEdge=top;ng.ng_Height=height-top-fy-40;ng.ng_GadgetID=1;
    list=last=CreateGadget(LISTVIEW_KIND,last,&ng,GTLV_Labels,(ULONG)&p->list,GTLV_ShowSelected,NULL,GTLV_Selected,(ULONG)-1,GTLV_ScrollWidth,16,TAG_DONE);if(!last)goto finish;
    ng.ng_TopEdge=height-fy-26;ng.ng_Height=fy+8;ng.ng_Width=(width-48)/3;ng.ng_Flags=PLACETEXT_IN;
    ng.ng_LeftEdge=12;ng.ng_GadgetID=2;ng.ng_GadgetText="Refresh";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto finish;
    ng.ng_LeftEdge=24+ng.ng_Width;ng.ng_GadgetID=3;ng.ng_GadgetText="Send";
    send=last=CreateGadget(BUTTON_KIND,last,&ng,GA_Disabled,TRUE,TAG_DONE);if(!last)goto finish;
    ng.ng_LeftEdge=36+ng.ng_Width*2;ng.ng_GadgetID=4;ng.ng_GadgetText="Cancel";
    last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto finish;
    win=OpenWindowTags(NULL,WA_PubScreen,(ULONG)p->screen,WA_Left,(p->screen->Width-width)/2,
        WA_Top,p->screen->BarHeight+1+(p->screen->Height-p->screen->BarHeight-height)/2,
        WA_Width,width,WA_Height,height,WA_Title,(ULONG)(p->count ? "BitSkim - choose a computer":"BitSkim - no devices yet; use Refresh"),
        WA_Gadgets,(ULONG)gadgets,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,
        WA_Flags,WFLG_SMART_REFRESH|WFLG_ACTIVATE,
        WA_IDCMP,LISTVIEWIDCMP|BUTTONIDCMP|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,TAG_DONE);
    if(!win)goto finish;GT_RefreshWindow(win,NULL);
    while(!done){
        Wait((1UL<<win->UserPort->mp_SigBit)|SIGBREAKF_CTRL_F);
        if(SetSignal(0,SIGBREAKF_CTRL_F)&SIGBREAKF_CTRL_F){WindowToFront(win);ActivateWindow(win);}
        while((im=GT_GetIMsg(win->UserPort))!=NULL){
            kind=im->Class;code=im->Code;id=kind==IDCMP_GADGETUP && im->IAddress ? ((struct Gadget *)im->IAddress)->GadgetID:0;GT_ReplyIMsg(im);
            if(kind==IDCMP_CLOSEWINDOW || (kind==IDCMP_RAWKEY && code==0x45)){done=1;result=0;}
            else if(kind==IDCMP_REFRESHWINDOW){GT_BeginRefresh(win);GT_EndRefresh(win,TRUE);}
            else if(kind==IDCMP_GADGETUP){
                if(id==1 && code<p->count){selected=(int)code;GT_SetGadgetAttrs(send,win,NULL,GA_Disabled,FALSE,TAG_DONE);}
                else if(id==2)refresh=1;
                else if(id==3 && selected>=0 && selected<p->count)submit=1;
                else if(id==4){done=1;result=0;}
            }
        }
        if(!done && refresh){
            GT_SetGadgetAttrs(list,win,NULL,GTLV_Labels,(ULONG)-1,TAG_DONE);selected=-1;
            GT_SetGadgetAttrs(send,win,NULL,GA_Disabled,TRUE,TAG_DONE);
            if(!bs_peers(p) && p->request.message){done=1;break;}
            GT_SetGadgetAttrs(list,win,NULL,GTLV_Labels,(ULONG)&p->list,GTLV_Selected,(ULONG)-1,TAG_DONE);
            SetWindowTitles(win,(STRPTR)(p->count ? "BitSkim - choose a computer":"BitSkim - no devices yet; use Refresh"),(STRPTR)-1);refresh=0;submit=0;
        }
        if(!done && submit){
            strcpy(chosen,p->ids[selected]);
            if(!nd_bitskim_send_command(p->command,sizeof(p->command),chosen,p->path))bs_notice("The path is too long for BitSkim's command interface.");
            else {rc=bs_query(p,p->command);if(rc==0){done=1;result=0;}else if(p->request.message)done=1;else bs_error(p);}
            submit=0;
        }
    }
finish:
    if(win)CloseWindow(win);if(gadgets)FreeGadgets(gadgets);if(visual)FreeVisualInfo(visual);
    if(p->screen)UnlockPubScreen(NULL,p->screen);return result;
}
static void bs_wrap(char *out,const char *label,const char *text,int maxlines)
{
    char local[256];unsigned int n=0,used=(unsigned int)strlen(out);int lines=0;
    nd_bitskim_local(local,sizeof(local),text);used+=sprintf(out+used,"%s",label);
    while(local[n] && lines<maxlines){unsigned int k=0;while(local[n] && k++<54)out[used++]=local[n++];out[used++]='\n';++lines;}
    if(!lines)out[used++]='\n';out[used]=0;
}
static void bs_details(struct BSPicker *p)
{
    struct NDBitSkimTransfer current,last,*t;char out[1600];int rc;
    memset(&current,0,sizeof(current));memset(&last,0,sizeof(last));
    rc=bs_query(p,"DETAILS");if(rc || !nd_bitskim_transfer(&current,p->response)){if(!p->request.message)bs_error(p);return;}
    rc=bs_query(p,"LAST");if(rc || !nd_bitskim_transfer(&last,p->response)){if(!p->request.message)bs_error(p);return;}
    t=current.available && !current.result ? &current:last.available ? &last:&current;
    if(!t->available){bs_notice("No transfer has started in this BitSkim session.");return;}
    sprintf(out,"%s (%s)\n%lu of %lu bytes\n",nd_bitskim_result(t->result,t->incoming),t->incoming ? "Receiving":"Sending",t->bytes,t->total);
    bs_wrap(out,"File: ",t->filename,3);bs_wrap(out,"Computer: ",t->peer,2);
    if(t->error[0])bs_wrap(out,"Reason: ",t->error,3);
    if(current.available && !current.result && last.available){
        strcat(out,"Previous transfer: ");strcat(out,nd_bitskim_result(last.result,last.incoming));strcat(out,"\n");
    }
    bs_notice(out);
}
static int bitskim_helper(int argc,char **argv)
{
    struct BSPicker *p=NULL;struct MsgPort *single=NULL,*prior;unsigned long n;char format[16];int rc,result=20,abandoned=0;
    /* Find and claim the name in one Forbid(), so two quick clicks cannot
     * both open the helper. */
    single=CreateMsgPort();if(!single)return 20;single->mp_Node.ln_Name="NovaDock.BitSkimUI";
    Forbid();prior=FindPort("NovaDock.BitSkimUI");
    if(prior){if(prior->mp_SigTask)Signal(prior->mp_SigTask,SIGBREAKF_CTRL_F);}else AddPort(single);
    Permit();
    if(prior){DeleteMsgPort(single);return 0;}
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",37);
    GadToolsBase=OpenLibrary("gadtools.library",37);AslBase=OpenLibrary("asl.library",37);
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",36);
    if(!IntuitionBase || !GadToolsBase || !AslBase || !RexxSysBase)goto finish;
    p=AllocVec(sizeof(*p),MEMF_PUBLIC|MEMF_CLEAR);if(!p)goto finish;
    p->request.reply=CreateMsgPort();if(!p->request.reply)goto finish;
    nd_bitskim_defaults(&p->config);(void)nd_plugin_options_effective(argv[1],nd_bitskim_option,&p->config);
    if(!find_bitskim(&p->config,p->host)){bs_notice("Open BitSkim first, then try again.");goto finish;}
    rc=bs_query(p,"CAPABILITIES");
    if(rc || !nd_bitskim_number(p->response,"AREXX",&n) || n<2 || !nd_bitskim_field(p->response,"FORMAT",format,sizeof(format)) || strcmp(format,"PCTBYTES")){if(!p->request.message)bs_notice("This action needs BitSkim with ARexx protocol 2.\nThe tile's basic controls still work with older versions.");goto finish;}
    if(!strcmp(argv[2],"DETAILS")){bs_details(p);result=0;goto finish;}
    if(!nd_bitskim_number(p->response,"SENDID",&n) || n!=1 || !nd_bitskim_number(p->response,"PEERS",&n) || n!=1){bs_notice("This BitSkim version does not offer stable-ID sending.");goto finish;}
    if(argc>3){if(strlen(argv[3])>=sizeof(p->path))goto finish;strcpy(p->path,argv[3]);}
    else if(!bs_choose_file(p)){result=0;goto finish;}
    if(!bs_regular_file(p->path)){bs_notice("Choose one regular file, up to 2 GB minus one byte.\nDrawers and multiple-file sends are not supported.");goto finish;}
    rc=bs_query(p,"SHOW");if(rc){if(!p->request.message)bs_error(p);goto finish;}
    /* SHOW resumes discovery; Refresh in the picker can collect later arrivals. */
    if(!bs_peers(p) && p->request.message)goto finish;
    result=bs_picker(p);
finish:
    if(p){abandoned=close_requests(&p->request);FreeVec(p);}
    if(RexxSysBase && !abandoned)CloseLibrary((struct Library *)RexxSysBase);
    if(AslBase)CloseLibrary(AslBase);if(GadToolsBase)CloseLibrary(GadToolsBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);
    if(single){RemPort(single);DeleteMsgPort(single);}return result;
}
#endif
