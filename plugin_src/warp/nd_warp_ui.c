/* Warp settings helpers. Library waits belong to a separately loaded worker,
 * never the resident sampler, dock, or Intuition event loop. */
#define IntuitionBase NDWarpIntuitionBase
#define GadToolsBase NDWarpGadToolsBase
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <intuition/sghooks.h>
#include <libraries/gadtools.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>
#include "nd_warp_control.h"
#include "nd_warp_ui.h"
#include "nd_cswarp.h"
#include "nd_warp_detect.h"
#include "nd_interaction.h"
#include "../prism/nd_prism.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
struct IntuitionBase *IntuitionBase=NULL;
struct Library *GadToolsBase=NULL;
#define WARP_MAGIC 0x4e445743UL
#define WARP_WORKER_ABI "1"
#define WARP_GUARD "NovaDock.Warp.Control"
#define UI_PREFIX "NovaDock.Warp.UI."
#define WARP_OK 1
#define WARP_BUSY 2
#define WARP_NO_BOARD 3
#define WARP_LIBRARY 4
#define WARP_DIAG_FAILED 5
#define WARP_MODEL 6
#define WARP_INVALID 7
#define WARP_SSID_FAILED 8
#define WARP_PASS_FAILED 9
#define WARP_FREQ_FAILED 10
struct WarpMessage {
 struct Message msg;ULONG magic;int kind,operation,level,result;
 char ssid[33],password[64];unsigned long khz,unit;int open_network;struct NDPrismScan scan;
};
struct WarpUI {
 struct MsgPort *port;char portname[72];int busy,operation,level,ready,timed_out;
 ULONG started,unit;int open_network;char ssid[33],password[64],status[100];struct Hook hook;
 struct NDPrismScan scan;struct List networks;struct Node nodes[ND_PRISM_MAX_NETWORKS];char labels[ND_PRISM_MAX_NETWORKS][64];
};
extern ULONG nd_warp_password_hook_entry(void);
static ULONG ticks(void)
{struct DateStamp d;DateStamp(&d);return (ULONG)d.ds_Days*4320000UL+(ULONG)d.ds_Minute*3000UL+d.ds_Tick;}
static int parent_valid(const char *name)
{
 unsigned int n;if(!name || strncmp(name,UI_PREFIX,sizeof(UI_PREFIX)-1))return 0;
 for(n=sizeof(UI_PREFIX)-1;name[n];++n)if(n>=70 || !((name[n]>='0' && name[n]<='9') || (name[n]>='a' && name[n]<='f') || name[n]=='.'))return 0;
 return n>sizeof(UI_PREFIX)-1;
}
static int send_message(const char *name,struct WarpMessage *m)
{struct MsgPort *port;int sent=0;Forbid();port=FindPort((STRPTR)name);if(port){PutMsg(port,(struct Message *)m);sent=1;}Permit();return sent;}
static void free_message(struct WarpMessage *m)
{nd_warp_erase(m,sizeof(*m));FreeVec(m);}
/* Neither full diagnostics (which include a Wi-Fi password) nor credentials
 * leave this worker. The reply carries only the selected public fields. */
static void board_operation(struct WarpMessage *m)
{
 struct Library *library=NULL;unsigned char data[ND_WARP_BUFFER_SIZE],packet[10];
 int operation=m->operation,level=m->level;char ssid[33],password[64];
 memcpy(ssid,m->ssid,sizeof(ssid));memcpy(password,m->password,sizeof(password));
 nd_warp_erase(m->password,sizeof(m->password));m->ssid[0]=0;m->khz=0;m->result=WARP_NO_BOARD;
 memset(data,0,sizeof(data));memset(packet,0,sizeof(packet));
 if(operation==ND_PRISM_SCAN){m->result=nd_prism_scan(m->unit,&m->scan);goto finished;}
 if(operation==ND_PRISM_SAVE){m->result=nd_prism_save(ssid,password,m->open_network);goto finished;}
 if(!nd_warp_control_present())goto finished;
 m->result=WARP_LIBRARY;library=OpenLibrary("cswarp.library",30);
 if(!library || !nd_warp_supported(library->lib_Version,library->lib_Revision))goto finished;
 m->result=WARP_DIAG_FAILED;if(!nd_cswarp_diag(library,data))goto finished;
 m->result=WARP_MODEL;if(!nd_warp_1260(data,sizeof(data)))goto finished;
 m->khz=nd_warp_frequency_khz(data,sizeof(data));
 memcpy(m->ssid,data+84,32);m->ssid[32]=0;
 /* Erase diagnostic credentials before any setter or further wait. */
 nd_warp_erase(data,sizeof(data));m->result=WARP_INVALID;
 if(operation==ND_WARP_QUERY)m->result=WARP_OK;
 else if(operation==ND_WARP_WIFI && nd_warp_wifi_valid(ssid,password)){
  m->result=WARP_SSID_FAILED;
  if(nd_cswarp_ssid(library,ssid)){
   m->result=WARP_PASS_FAILED;
   if(nd_cswarp_password(library,password)){m->result=WARP_OK;strcpy(m->ssid,ssid);}
  }
 }else if(operation==ND_WARP_TURBO && nd_warp_turbo_packet(level,packet)){
  m->result=nd_cswarp_frequency(library,packet) ? WARP_OK:WARP_FREQ_FAILED;
  /* Pre-change MHz must not be presented as a verified new clock. */
  m->khz=0;
 }
finished:
 nd_warp_erase(password,sizeof(password));nd_warp_erase(ssid,sizeof(ssid));
 nd_warp_erase(data,sizeof(data));nd_warp_erase(packet,sizeof(packet));
 if(library)CloseLibrary(library);
}
static int worker(const char *parent)
{
 struct MsgPort *guard=NULL,*reply=NULL;struct WarpMessage *m=NULL;int owner=0;
 if(!parent_valid(parent))return 20;
 ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 guard=CreateMsgPort();reply=CreateMsgPort();m=AllocVec(sizeof(*m),MEMF_PUBLIC|MEMF_CLEAR);
 if(!guard || !reply || !m)goto cleanup;
 Forbid();if(!FindPort(WARP_GUARD)){guard->mp_Node.ln_Name=WARP_GUARD;AddPort(guard);owner=1;}Permit();
 m->magic=WARP_MAGIC;m->msg.mn_Length=sizeof(*m);
 if(!owner){m->kind=2;m->result=WARP_BUSY;if(send_message(parent,m))m=NULL;goto cleanup;}
 m->kind=1;m->msg.mn_ReplyPort=reply;
 if(!send_message(parent,m))goto cleanup;
 /* The UI owns this message until it replies. Closing the UI replies CANCEL.
  * Do not free a borrowed message, even when its receiver is unresponsive. */
 WaitPort(reply);GetMsg(reply);DeleteMsgPort(reply);reply=NULL;
 if(m->kind==1)board_operation(m);else goto cleanup;
 nd_warp_erase(m->password,sizeof(m->password));m->kind=2;m->msg.mn_ReplyPort=NULL;
 if(send_message(parent,m))m=NULL;
cleanup:
 if(m)free_message(m);if(reply)DeleteMsgPort(reply);
 if(guard){if(owner){Forbid();RemPort(guard);Permit();}DeleteMsgPort(guard);}return 0;
}
static void close_port(struct MsgPort *port)
{
 struct WarpMessage *m;if(!port)return;
 Forbid();RemPort(port);Permit();
 while((m=(struct WarpMessage *)GetMsg(port))!=NULL){
  if(m->kind==1 && m->msg.mn_ReplyPort){m->kind=-1;ReplyMsg((struct Message *)m);}
  else free_message(m);
 }
 DeleteMsgPort(port);
}
static int start_job(struct WarpUI *ui,const char *program,int operation)
{
 char path[520],command[1200];BPTR input,output;LONG rc;int occupied;
 if(ui->busy)return 0;
 strcpy(ui->status,"Could not start the settings worker.");
 Forbid();occupied=FindPort(WARP_GUARD)!=NULL;Permit();
 if(occupied){strcpy(ui->status,"Another Wi-Fi or clock operation is still running.");return 0;}
 if(!GetProgramDir() || !NameFromLock(GetProgramDir(),path,sizeof(path)) ||
    !AddPart(path,FilePart((STRPTR)program),sizeof(path)) || !nd_quote_path(command,sizeof(command)-100,path))return 0;
 strcat(command," WARPWORK ");strcat(command,ui->portname);strcat(command," " WARP_WORKER_ABI);
 input=Open("NIL:",MODE_OLDFILE);output=Open("NIL:",MODE_NEWFILE);
 if(!input || !output){if(input)Close(input);if(output)Close(output);return 0;}
 ui->busy=1;ui->operation=operation;ui->started=ticks();ui->timed_out=0;
 rc=SystemTags(command,SYS_Input,input,SYS_Output,output,SYS_Asynch,TRUE,NP_StackSize,32768UL,TAG_DONE);
 if(rc<0){Close(input);Close(output);ui->busy=0;return 0;}
 strcpy(ui->status,operation==ND_PRISM_SCAN ? "Scanning Prism networks...":operation==ND_PRISM_SAVE ?
  "Deriving key and saving Wireless.prefs...":operation==ND_WARP_QUERY ? "Reading Warp1260 settings...":"Applying Warp1260 settings...");return 1;
}
static const char *error_text(int result)
{
 switch(result){
 case ND_PRISM_OPEN_FAILED:return "Cannot open prism2.device. Check the card and unit number.";
 case ND_PRISM_SCAN_FAILED:return "Scan failed. Bring the Prism interface online, then scan again.";
 case ND_PRISM_PROFILE_FAILED:return "Profile save failed. Originals retained; check pending files.";
 case ND_PRISM_PARTIAL:return "Profile transaction incomplete. Inspect both prefs and backups.";
 case ND_PRISM_INVALID:return "Enter an SSID (1-32) and WPA password (8-63 ASCII characters).";
 case WARP_BUSY:return "Another Wi-Fi or clock operation is still running.";
 case WARP_NO_BOARD:return "No supported Warp control board detected.";
 case WARP_LIBRARY:return "This feature requires cswarp.library 30.74.";
 case WARP_DIAG_FAILED:return "Could not read Warp diagnostics. No settings changed.";
 case WARP_MODEL:return "These controls currently support Warp1260 only.";
 case WARP_SSID_FAILED:return "SSID write failed; password was not sent.";
 case WARP_PASS_FAILED:return "Password write failed after SSID; check board settings.";
 case WARP_FREQ_FAILED:return "The board did not confirm the clock change.";
 default:return "Invalid settings. No settings changed.";
 }
}
ULONG nd_warp_password_hook(struct Hook *hook,struct SGWork *work,ULONG *message)
{
 struct WarpUI *ui=(struct WarpUI *)hook->h_Data;int len;
 if(*message==SGH_CLICK)return 1;if(*message!=SGH_KEY)return 0;
 if(!nd_warp_password_edit(ui->password,work->WorkBuffer,work->StringInfo->BufferPos,
      work->BufferPos,work->StringInfo->NumChars,work->NumChars,work->EditOp)){
  /* Do not leave a rejected literal keystroke in the work buffer. */
  len=(int)strlen(ui->password);memset(work->WorkBuffer,'*',len);work->WorkBuffer[len]=0;
  work->Actions&=~SGA_USE;work->Actions|=SGA_BEEP;return 1;
 }
 if(work->EditOp==EO_CLEAR || work->EditOp==EO_RESET){work->NumChars=0;work->BufferPos=0;}
 work->Actions|=SGA_USE|SGA_REDISPLAY;return 1;
}
static void clear_password(struct WarpUI *ui,struct Gadget *password,struct Window *win)
{
 struct StringInfo *info=(struct StringInfo *)password->SpecialInfo;
 nd_warp_erase(ui->password,sizeof(ui->password));
 if(info->Buffer)nd_warp_erase(info->Buffer,info->MaxChars);
 if(info->UndoBuffer)nd_warp_erase(info->UndoBuffer,info->MaxChars);
 if(info->Extension && info->Extension->WorkBuffer)nd_warp_erase(info->Extension->WorkBuffer,info->MaxChars);
 if(win)GT_SetGadgetAttrs(password,win,NULL,GTST_String,(ULONG)"",TAG_DONE);
}
static const char *field(struct Gadget *g)
{return ((struct StringInfo *)g->SpecialInfo)->Buffer;}
static int dialog(const char *program,int mode)
{
 static STRPTR security[]={"WPA / WPA2 personal","Open (no password)",NULL};
 static STRPTR levels[]={"Choose a turbo level...","0: 50 MHz","1: 66.6 MHz","2: 75 MHz","3: 85 MHz",
  "4: 90 MHz","5: 95 MHz","6: 100 MHz","7: 105 MHz",NULL};
 struct WarpUI *ui=NULL;struct Screen *screen=NULL;struct Window *win=NULL;APTR visual=NULL;
 struct Gadget *gadgets=NULL,*last,*ssid=NULL,*password=NULL,*apply=NULL,*refresh=NULL,*status[3]={NULL,NULL,NULL},*current=NULL,*level=NULL,*unit_gadget=NULL,*security_gadget=NULL,*network_list=NULL,*g;
 struct NewGadget ng;struct TextAttr font={(STRPTR)"topaz.font",8,0,0};
 struct IntuiMessage *msg;struct WarpMessage *m;char clocktext[80],statuslines[3][72];const char *text;LONG unit_value;unsigned int n,row,len,columns;
 ULONG cls;UWORD code,id;int w,top,compact=0,step,done=0,changed,first=1,selection=0,result=20,wifi=mode!=0,prism=mode==2;
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GadToolsBase=OpenLibrary("gadtools.library",39);
 if(!IntuitionBase || !GadToolsBase)goto cleanup;
 ui=AllocVec(sizeof(*ui),MEMF_PUBLIC|MEMF_CLEAR);if(!ui)goto cleanup;NewList(&ui->networks);
 ui->port=CreateMsgPort();if(!ui->port)goto cleanup;
 sprintf(ui->portname,UI_PREFIX"%08lx.%08lx",(ULONG)FindTask(NULL),ticks());
 ui->port->mp_Node.ln_Name=ui->portname;AddPort(ui->port);
 screen=LockPubScreen("Workbench");if(!screen)goto cleanup;
 w=screen->Width-24;if(w>560)w=560;if(w<400)goto cleanup;
 compact=screen->Height-screen->BarHeight<400;step=compact ? 20:28;
 visual=GetVisualInfo(screen,TAG_DONE);if(!visual)goto cleanup;
 last=CreateContext(&gadgets);if(!last)goto cleanup;
 memset(&ng,0,sizeof(ng));ng.ng_VisualInfo=visual;ng.ng_TextAttr=&font;
 top=screen->WBorTop+screen->Font->ta_YSize+10;
 ng.ng_LeftEdge=12;ng.ng_TopEdge=top;ng.ng_Width=w-24;ng.ng_Height=14;ng.ng_GadgetText="";
 current=last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)(prism ? "Prism Wi-Fi - prism2.device":"Warp1260 - checking hardware"),TAG_DONE);if(!last)goto cleanup;
 ng.ng_LeftEdge=100;ng.ng_TopEdge+=compact ? 16:24;ng.ng_Width=w-116;ng.ng_Height=compact ? 16:20;ng.ng_Flags=PLACETEXT_LEFT;ng.ng_GadgetID=1;
 if(prism){
  ng.ng_GadgetText="Card unit";ng.ng_GadgetID=6;
  unit_gadget=last=CreateGadget(INTEGER_KIND,last,&ng,GTIN_Number,0,GTIN_MaxChars,3,TAG_DONE);if(!last)goto cleanup;
  ng.ng_TopEdge+=step;ng.ng_GadgetID=1;
 }
 if(wifi){
  ng.ng_GadgetText="Wi-Fi name";ssid=last=CreateGadget(STRING_KIND,last,&ng,GTST_MaxChars,33,TAG_DONE);if(!last)goto cleanup;
  ng.ng_TopEdge+=step;ng.ng_GadgetText="Password";ng.ng_GadgetID=2;
  ui->hook.h_Entry=(HOOKFUNC)nd_warp_password_hook_entry;ui->hook.h_Data=ui;
  password=last=CreateGadget(STRING_KIND,last,&ng,GTST_MaxChars,64,GTST_EditHook,(ULONG)&ui->hook,TAG_DONE);if(!last)goto cleanup;
 }else{
  ng.ng_GadgetText="Turbo level";ng.ng_GadgetID=2;
  level=last=CreateGadget(CYCLE_KIND,last,&ng,GTCY_Labels,(ULONG)levels,TAG_DONE);if(!last)goto cleanup;
 }
 if(prism){
  ng.ng_TopEdge+=step;ng.ng_GadgetID=7;ng.ng_GadgetText="Security";
  security_gadget=last=CreateGadget(CYCLE_KIND,last,&ng,GTCY_Labels,(ULONG)security,TAG_DONE);if(!last)goto cleanup;
  ng.ng_TopEdge+=step;ng.ng_LeftEdge=12;ng.ng_Width=w-24;ng.ng_Height=compact ? 40:84;ng.ng_Flags=0;ng.ng_GadgetText="";ng.ng_GadgetID=8;
  network_list=last=CreateGadget(LISTVIEW_KIND,last,&ng,GTLV_Labels,(ULONG)&ui->networks,GTLV_ScrollWidth,16,TAG_DONE);if(!last)goto cleanup;
  ng.ng_TopEdge+=compact ? 28:66;
 }
 if(compact && prism){
  ng.ng_LeftEdge=12;ng.ng_TopEdge+=prism ? 16:24;ng.ng_Width=w-24;
  ng.ng_Flags=0;ng.ng_GadgetText="";ng.ng_Height=12;
 }else{
 ng.ng_LeftEdge=12;ng.ng_TopEdge+=30;ng.ng_Width=w-24;ng.ng_Height=14;ng.ng_Flags=0;ng.ng_GadgetText="";
 last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)(prism ? "Save updates ENV: and ENVARC:Sys/Wireless.prefs.":wifi ? "Apply saves both values to the board.":"Apply changes and saves the board clock."),TAG_DONE);if(!last)goto cleanup;
 ng.ng_TopEdge+=16;
 last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)(prism ? "Keeps other profiles. Reconnect WirelessManager.":wifi ? "Password is masked and kept only in memory.":"Higher clocks require a suitable CPU and cooling."),TAG_DONE);if(!last)goto cleanup;
 ng.ng_TopEdge+=24;ng.ng_Height=12;
 }
 for(row=0;row<3;++row){
  status[row]=last=CreateGadget(TEXT_KIND,last,&ng,GTTX_Text,(ULONG)"",TAG_DONE);if(!last)goto cleanup;
  ng.ng_TopEdge+=compact ? 12:14;
 }
 ng.ng_TopEdge+=compact ? 4:6;ng.ng_Width=96;ng.ng_Height=20;ng.ng_Flags=PLACETEXT_IN;
 ng.ng_GadgetID=3;ng.ng_GadgetText=prism ? "Save":"Apply";apply=last=CreateGadget(BUTTON_KIND,last,&ng,GA_Disabled,TRUE,TAG_DONE);if(!last)goto cleanup;
 ng.ng_LeftEdge+=108;ng.ng_GadgetID=4;ng.ng_GadgetText=prism ? "Scan":"Read again";
 refresh=last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
 ng.ng_LeftEdge=w-108;ng.ng_GadgetID=5;ng.ng_GadgetText="Close";
 last=CreateGadget(BUTTON_KIND,last,&ng,TAG_DONE);if(!last)goto cleanup;
 win=OpenWindowTags(NULL,WA_Title,(ULONG)(prism ? "Prism - Configure Wi-Fi Network":wifi ? "Warp1260 - Configure Wi-Fi Network":"Warp1260 - Set Turbo Level"),
  WA_PubScreen,(ULONG)screen,WA_Left,(screen->Width-w)/2,WA_Top,screen->BarHeight+2,
  WA_Width,w,WA_Height,ng.ng_TopEdge+32,WA_Gadgets,(ULONG)gadgets,
  WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SIMPLE_REFRESH,
  WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_GADGETDOWN|IDCMP_INTUITICKS|IDCMP_MOUSEMOVE|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW|IDCMP_RAWKEY,TAG_DONE);
 if(!win)goto cleanup;GT_RefreshWindow(win,NULL);result=0;
 start_job(ui,program,prism ? ND_PRISM_SCAN:ND_WARP_QUERY);
 while(!done){
  changed=first;first=0;
  while((m=(struct WarpMessage *)GetMsg(ui->port))!=NULL){
   if(m->magic!=WARP_MAGIC || m->msg.mn_Length!=sizeof(*m)){if(m->msg.mn_ReplyPort)ReplyMsg((struct Message *)m);continue;}
   if(m->kind==1 && m->msg.mn_ReplyPort){
    m->operation=ui->operation;m->level=ui->level;m->unit=ui->unit;m->open_network=ui->open_network;
    if(ui->operation==ND_WARP_WIFI || ui->operation==ND_PRISM_SAVE){memcpy(m->ssid,ui->ssid,sizeof(m->ssid));memcpy(m->password,ui->password,sizeof(m->password));clear_password(ui,password,win);}
    ReplyMsg((struct Message *)m);
   }else{
    ui->busy=0;ui->timed_out=0;ui->ready=prism || m->result==WARP_OK;
    if(m->result==ND_PRISM_OK && prism){
     if(m->operation==ND_PRISM_SCAN){
      GT_SetGadgetAttrs(network_list,win,NULL,GTLV_Labels,(ULONG)~0UL,TAG_DONE);
      ui->scan=m->scan;NewList(&ui->networks);
      for(n=0;n<ui->scan.count;++n){
       sprintf(ui->labels[n],"%-32s %ld dBm  ch %lu",ui->scan.networks[n].ssid,ui->scan.networks[n].signal,ui->scan.networks[n].channel);
       ui->nodes[n].ln_Name=ui->labels[n];AddTail(&ui->networks,&ui->nodes[n]);
      }
      GT_SetGadgetAttrs(network_list,win,NULL,GTLV_Labels,(ULONG)&ui->networks,GTLV_Top,0,TAG_DONE);
      sprintf(ui->status,"%u networks. Choose one or enter a hidden SSID.",ui->scan.count);
     }else strcpy(ui->status,"Wi-Fi profile saved. Reconnect with WirelessManager to use it.");
    }else if(m->result!=WARP_OK)strcpy(ui->status,error_text(m->result));
    else if(m->operation==ND_WARP_QUERY){
     if(m->khz)sprintf(clocktext,"Warp1260 - current clock: %lu.%lu MHz",m->khz/1000UL,(m->khz%1000UL)/100UL);
     else strcpy(clocktext,"Warp1260 - current clock unavailable");
     GT_SetGadgetAttrs(current,win,NULL,GTTX_Text,(ULONG)clocktext,TAG_DONE);
     if(wifi)GT_SetGadgetAttrs(ssid,win,NULL,GTST_String,(ULONG)m->ssid,TAG_DONE);
     strcpy(ui->status,"Ready. Settings change only when you click Apply.");
    }else if(m->operation==ND_WARP_WIFI)strcpy(ui->status,"Wi-Fi settings accepted. Connection may take a moment.");
    else{strcpy(ui->status,"Clock setting accepted. Read again to check the new clock.");
     GT_SetGadgetAttrs(current,win,NULL,GTTX_Text,(ULONG)"Warp1260 - clock setting sent",TAG_DONE);}
    free_message(m);
   }changed=1;
  }
  while((msg=GT_GetIMsg(win->UserPort))!=NULL){
   cls=msg->Class;code=msg->Code;g=cls==IDCMP_GADGETUP ? (struct Gadget *)msg->IAddress:NULL;id=g ? g->GadgetID:0;GT_ReplyIMsg(msg);
   if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45)){done=1;break;}
   if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(win);GT_EndRefresh(win,TRUE);}
   if(cls!=IDCMP_GADGETUP)continue;
   if(id==5){done=1;break;}
   if(id==2 && !wifi)selection=code;
   if(prism && id==7)ui->open_network=code!=0;
   if(prism && id==8 && code<ui->scan.count)GT_SetGadgetAttrs(ssid,win,NULL,GTST_String,(ULONG)ui->scan.networks[code].ssid,TAG_DONE);
   if(prism && unit_gadget){unit_value=((struct StringInfo *)unit_gadget->SpecialInfo)->LongInt;ui->unit=unit_value>=0 && unit_value<=255 ? (ULONG)unit_value:256;}
   if(!ui->busy && id==4){if(prism && ui->unit>255)strcpy(ui->status,"Enter a card unit between 0 and 255.");
    else{ui->ready=0;start_job(ui,program,prism ? ND_PRISM_SCAN:ND_WARP_QUERY);}}
   if(!ui->busy && ui->ready && id==3){
    if(wifi){
     strncpy(ui->ssid,field(ssid),32);ui->ssid[32]=0;
     if(prism ? !nd_prism_credentials(ui->ssid,ui->password,ui->open_network):!nd_warp_wifi_valid(ui->ssid,ui->password))strcpy(ui->status,prism ? error_text(ND_PRISM_INVALID):"Enter an SSID (2-32) and password (1-63 ASCII characters).");
     else start_job(ui,program,prism ? ND_PRISM_SAVE:ND_WARP_WIFI);
    }else if(selection>=1 && selection<=8){
     ui->level=selection-1;start_job(ui,program,ND_WARP_TURBO);
    }else strcpy(ui->status,"Choose a turbo level before applying.");
   }changed=1;
  }
  if(ui->busy && !ui->timed_out && (ULONG)(ticks()-ui->started)>=1500UL){
   ui->timed_out=1;strcpy(ui->status,"Device still busy. You may close; new operations must wait.");changed=1;
  }
  if(changed){
   text=ui->status;columns=(w-24)/8;if(columns>70)columns=70;
   for(row=0;row<3;++row){
    len=(unsigned int)strlen(text);if(len>columns){len=columns;while(len && text[len]!=' ')--len;if(!len)len=columns;}
    memcpy(statuslines[row],text,len);statuslines[row][len]=0;text+=len;while(*text==' ')++text;
    GT_SetGadgetAttrs(status[row],win,NULL,GTTX_Text,(ULONG)statuslines[row],TAG_DONE);
   }
   if(ssid)GT_SetGadgetAttrs(ssid,win,NULL,GA_Disabled,ui->busy,TAG_DONE);
   if(password)GT_SetGadgetAttrs(password,win,NULL,GA_Disabled,ui->busy,TAG_DONE);
   if(level)GT_SetGadgetAttrs(level,win,NULL,GA_Disabled,ui->busy,TAG_DONE);
   if(unit_gadget)GT_SetGadgetAttrs(unit_gadget,win,NULL,GA_Disabled,ui->busy,TAG_DONE);
   if(security_gadget)GT_SetGadgetAttrs(security_gadget,win,NULL,GA_Disabled,ui->busy,TAG_DONE);
   if(network_list)GT_SetGadgetAttrs(network_list,win,NULL,GA_Disabled,ui->busy,TAG_DONE);
   GT_SetGadgetAttrs(apply,win,NULL,GA_Disabled,ui->busy || !ui->ready || (!wifi && !selection),TAG_DONE);
   GT_SetGadgetAttrs(refresh,win,NULL,GA_Disabled,ui->busy,TAG_DONE);
  }
  Delay(2);
 }
cleanup:
 if(win)CloseWindow(win);
 if(password && ui)clear_password(ui,password,NULL);
 if(gadgets)FreeGadgets(gadgets);if(visual)FreeVisualInfo(visual);
 if(screen)UnlockPubScreen(NULL,screen);
 if(ui){close_port(ui->port);nd_warp_erase(ui,sizeof(*ui));FreeVec(ui);}
 if(GadToolsBase){CloseLibrary(GadToolsBase);GadToolsBase=NULL;}
 if(IntuitionBase){CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;}return result;
}
int nd_warp_helper_main(int argc,char **argv)
{
 if(argc>1 && !strcmp(argv[1],"WARPWORK")){if(argc==4 && !strcmp(argv[3],WARP_WORKER_ABI))return worker(argv[2]);return 20;}
 if(argc==3 && (!strcmp(argv[1],"NET") || !strcmp(argv[1],"ROADSHOW")) && !strcmp(argv[2],"PRISM_WIFI"))return dialog(argv[0],2);
 if(argc==3 && !strcmp(argv[1],"CPU") && !strcmp(argv[2],"WARP_TURBO"))return dialog(argv[0],0);
 if(argc==3 && (!strcmp(argv[1],"NET") || !strcmp(argv[1],"ROADSHOW")) && !strcmp(argv[2],"WARP_WIFI"))return dialog(argv[0],1);
 return -1;
}
