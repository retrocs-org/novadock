#include <exec/types.h>
#include <exec/memory.h>
#include <devices/clipboard.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_client.h"
#include "nd_panel.h"
#include "nd_task.h"
#include "nd_text.h"
#include "nd_qr.h"
#include "nd_qr_view.h"
#include "nd_plugin_versions.h"
#include "../clipboard/nd_clipboard.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(QR,"QR");long __stack=65536;
#include "nd_qr_job.h"
static int clipboard(char text[ND_QR_TEXT_MAX+1])
{struct MsgPort *port=CreateMsgPort();struct IOClipReq *io=NULL;unsigned char *raw=NULL;unsigned long size=0;int ok=0,opened=0;if(port)io=(struct IOClipReq *)CreateIORequest(port,sizeof(*io));raw=AllocVec(ND_CLIP_RAW_MAX,MEMF_PUBLIC);if(!io||!raw)goto done;if(OpenDevice("clipboard.device",0,(struct IORequest *)io,0))goto done;opened=1;io->io_Command=CMD_READ;io->io_Data=raw;io->io_Length=ND_CLIP_RAW_MAX;
 if(!DoIO((struct IORequest *)io)){size=io->io_Actual;ok=nd_clip_decode(raw,size,text,ND_QR_TEXT_MAX+1)==1;}
 if(io->io_Actual){do{io->io_Error=0;io->io_Command=CMD_READ;io->io_Data=NULL;io->io_Length=0x7fffffffUL;DoIO((struct IORequest *)io);}while(!io->io_Error&&io->io_Actual);}
 done:if(opened)CloseDevice((struct IORequest *)io);if(raw)FreeVec(raw);if(io)DeleteIORequest((struct IORequest *)io);if(port)DeleteMsgPort(port);return ok;}
static int work(unsigned int action,void *data,unsigned int bytes,const char *parent)
{struct QRJob *j=data;unsigned char *image;unsigned int length;int ok;(void)parent;if(bytes!=sizeof(*j)||!memchr(j->text,0,sizeof(j->text))||!memchr(j->path,0,sizeof(j->path)))return 0;
 if(action==1)return nd_qr_generate(j->text,&j->qr);
 if(action==2)return clipboard(j->text) ? 1:-2;
 if(action!=3)return 0;image=AllocVec(ND_QR_ILBM_MAX,MEMF_PUBLIC);if(!image)return 0;length=nd_qr_ilbm(&j->qr,image,ND_QR_ILBM_MAX);ok=length ? nd_text_export(j->path,(const char *)image,length):0;FreeVec(image);return ok;}
static struct NDPanel *input(const struct QRJob *j,const char *status)
{struct NDPanelField f[]={{"Text / URL",j->text,ND_QR_TEXT_MAX+1},{"Export ILBM",j->path,240},{"Status",status,0}};const char *b[]={"Generate","Paste","Export","Close"};return nd_panel_open("QR - local text and URL codes",f,3,b,4);}
int main(int argc,char **argv)
{struct NDClient dock;struct NDTask task;struct NDTaskPacket *reply;struct QRJob *j;struct NDPanel *panel=NULL;struct NDQRView *view=NULL;char requested[ND_QR_TEXT_MAX+1]="",status[128]="Enter text or paste clipboard; Generate",value[48]="Click to generate QR";unsigned int event,elapsed=0,kind;int action,ready=0,ok;
 if(argc==3&&!strcmp(argv[1],"TASK"))return nd_task_worker(argv[2],work);j=AllocVec(sizeof(*j),MEMF_PUBLIC|MEMF_CLEAR);if(!j)return 20;strcpy(j->path,"RAM:NovaDock-QR.iff");
 if(argc==2&&(!strcmp(argv[1],"SELFTEST")||!strcmp(argv[1],"VIEW-SELFTEST"))){strcpy(j->text,"https://novadock.retrocs.org/");ok=nd_qr_generate(j->text,&j->qr);if(ok&&!strcmp(argv[1],"VIEW-SELFTEST")){view=nd_qr_view_open(&j->qr);ok=view!=NULL;Delay(15);nd_qr_view_close(view);}FreeVec(j);return ok ? 0:10;}
 if(argc!=2||!nd_client_init(&dock,argv[1])){FreeVec(j);return 20;}memset(&task,0,sizeof(task));((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 for(;;){reply=nd_task_poll(&task);if(reply){kind=reply->action;if(reply->result==1&&reply->bytes==sizeof(*j)){
    if((kind==1||kind==2)&&panel&&strcmp(nd_panel_get(panel,0),requested))strcpy(status,"Text changed; result discarded, generate again");
    else{*j=*(struct QRJob *)reply->data;if(kind==1){ready=1;sprintf(value,"%u x %u modules",j->qr.size,j->qr.size);strcpy(status,"Ready; code includes a four-module quiet zone");if(panel){nd_qr_view_close(view);view=nd_qr_view_open(&j->qr);if(!view)strcpy(status,nd_qr_view_error());}}
     else if(kind==2){ready=0;nd_qr_view_close(view);view=NULL;strcpy(status,"Clipboard pasted; Generate to encode");if(panel){nd_panel_replace_string(panel,0,j->text);}}
     else strcpy(status,"ILBM exported; existing files are never overwritten");}
   }else strcpy(status,kind==3 ? nd_text_export_problem(reply->result):kind==2 ? "Clipboard empty, unsupported or over 512 bytes":kind==1 ? "Cannot encode: maximum 512 UTF-8 bytes":"Worker/export failed");if(panel)nd_panel_set(panel,2,status);FreeVec(reply);}
  event=nd_client_update_art_quiet(&dock,"QR",task.port ? "Working...":value,"qr",ready ? 1:0,elapsed,250);if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=input(j,status);if(ready){if(view)nd_qr_view_raise(view);else view=nd_qr_view_open(&j->qr);}}
  if(nd_qr_view_event(view)){nd_qr_view_close(view);view=NULL;}
  action=nd_panel_event(panel);if(action==-1||action==103){nd_panel_close(panel);panel=NULL;}else if((action==1||action==100||action==101||action==102)&&!task.port){kind=action==101 ? 2:action==102 ? 3:1;
   if(kind==3&&(!ready||strcmp(nd_panel_get(panel,0),j->text))){nd_panel_set(panel,2,"Generate the current text before exporting");continue;}
   if(strlen(nd_panel_get(panel,0))>ND_QR_TEXT_MAX||strlen(nd_panel_get(panel,1))>=sizeof(j->path)){nd_panel_set(panel,2,"Text or path is too long");continue;}
   strcpy(requested,nd_panel_get(panel,0));strcpy(j->text,requested);strcpy(j->path,nd_panel_get(panel,1));if(kind==1){ready=0;nd_qr_view_close(view);view=NULL;}if(nd_task_start(&task,argv[0],"NovaDock.QR.Worker",kind,j,sizeof(*j)))nd_panel_set(panel,2,"Working locally...");else nd_panel_set(panel,2,"Previous worker is still stopping; try again");}
  nd_panel_button(panel,0,"Generate",!task.port);nd_panel_button(panel,1,"Paste",!task.port);nd_panel_button(panel,2,"Export",!task.port&&ready&&panel&&!strcmp(nd_panel_get(panel,0),j->text));
  elapsed=nd_client_wait_signals(&dock,task.port ? 5:250,nd_panel_signals(panel)|nd_qr_view_signals(view)|nd_task_signals(&task));
 }nd_task_close(&task);nd_qr_view_close(view);nd_panel_close(panel);nd_client_close(&dock);FreeVec(j);return 0;
}
