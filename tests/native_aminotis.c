/* General setting and real dock->SDK->service messages, offline guest only. */
#include "native_qa_ui.h"
#include "nd_control.h"
#include "nd_config.h"
#include "../src/aminotis_sdk/aminotis.h"
long __stack=65536;
static int lines(void){FILE *f=fopen("QATests:notis-received.log","r");char b[800];int n=0;if(f){while(fgets(b,sizeof(b),f))++n;fclose(f);}return n;}
static int trigger(unsigned serial,unsigned body,unsigned total)
{
 FILE *f=NULL;struct NDControlAlertsMessage a;unsigned ack=0;int i;
 for(i=0;i<100&&!f;++i){f=fopen("QATests:notis-trigger","w");if(!f)Delay(1);}
 if(!f){puts("Trigger file stayed locked");return 0;}fprintf(f,"%u %u",serial,body);fclose(f);
 for(i=0;i<300;++i){f=fopen("QATests:notis-ack","r");if(f){(void)fscanf(f,"%u",&ack);fclose(f);}if(ack==serial)break;Delay(2);}
 memset(&a,0,sizeof(a));if(ack==serial&&nd_control_alerts(&a)==ND_CONTROL_OK&&a.total==total)return 1;
 printf("Trigger serial=%u ack=%u body=%u expected=%u actual=%u count=%u\n",serial,ack,body,total,a.total,a.count);
 for(i=0;i<a.count;++i)printf("Entry %s %s %s\n",a.entries[i].source,a.entries[i].title,a.entries[i].text);
 return 0;
}
static int delivered(int n){int i;for(i=0;i<200;++i){if(lines()==n)return 1;Delay(2);}return 0;}
static int stop_sink(void){struct MsgPort *p;int i;Forbid();p=FindPort(AN_PORT);if(p)Signal(p->mp_SigTask,SIGBREAKF_CTRL_C);Permit();for(i=0;i<200;++i){if(!qa_port(AN_PORT))return 1;Delay(1);}return 0;}
static int start_sink(void){int i;if(!qa_start("QATests:NotisSink"))return 0;for(i=0;i<200;++i){if(qa_port(AN_PORT))return 1;Delay(1);}return 0;}
static int click_recording(struct Window *w,const char *name)
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
    if(!qa_pointer(w->WScreen,w->LeftEdge+g->LeftEdge+(g->Flags&GFLG_RELRIGHT?w->Width:0)+(g->Width+(g->Flags&GFLG_RELWIDTH?w->Width:0))/2,w->TopEdge+g->TopEdge+(g->Flags&GFLG_RELBOTTOM?w->Height:0)+(g->Height+(g->Flags&GFLG_RELHEIGHT?w->Height:0))/2))return 0;
    Delay(3);port=CreateMsgPort();if(!port)return 0;io=(struct IOStdReq *)CreateIORequest(port,sizeof(*io));if(!io)return 0;
    if(OpenDevice("input.device",0,(struct IORequest *)io,0)){DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return 0;}
    for(up=0;up<2;++up){memset(&e,0,sizeof(e));e.ie_Class=IECLASS_RAWMOUSE;
        e.ie_Code=up?IECODE_LBUTTON|IECODE_UP_PREFIX:IECODE_LBUTTON;e.ie_Qualifier=up?0:IEQUALIFIER_LEFTBUTTON;
        io->io_Command=IND_WRITEEVENT;io->io_Data=&e;io->io_Length=sizeof(e);DoIO((struct IORequest *)io);Delay(3);}
    CloseDevice((struct IORequest *)io);DeleteIORequest((struct IORequest *)io);DeleteMsgPort(port);return 1;
}
static int forwarding_report(void)
{
 BPTR lock;struct FileInfoBlock *fib;int ok=0,i;char path[512],text[8192];FILE *f;size_t n;struct Window *notice;struct Gadget *button;struct IntuiText *label;
 if(SystemTags("QATests:Dock/SupportReport CLI",TAG_DONE))return 0;
 lock=Lock("QATests:Dock/SupportBundles",ACCESS_READ);if(!lock)return 0;
 fib=AllocDosObject(DOS_FIB,NULL);if(!fib){UnLock(lock);return 0;}
 if(Examine(lock,fib))while(ExNext(lock,fib))if(fib->fib_DirEntryType>0){
  strcpy(path,"QATests:Dock/SupportBundles");if(!AddPart(path,fib->fib_FileName,sizeof(path))||!AddPart(path,"support.txt",sizeof(path)))continue;
  f=fopen(path,"r");if(!f)continue;n=fread(text,1,sizeof(text)-1,f);text[n]=0;fclose(f);
  if(strstr(text,"AmiNotis_enabled=1 Received=3 Delivered=2 Dropped=1") && strstr(text,"Last_failure=service unavailable"))ok=1;
 }
 FreeDosObject(DOS_FIB,fib);UnLock(lock);if(!ok){puts("Support report fields missing");return 0;}puts("PASS support.txt forwarding diagnostics");
 if(!qa_start("QATests:Dock/SupportReport RECORD 1"))return 0;
 for(i=0;i<500;++i){f=fopen("NovaDock:SupportLogs/capture.txt","r");if(f){n=fread(text,1,sizeof(text)-1,f);text[n]=0;fclose(f);if(strstr(text,"AmiNotis_enabled=1 Received=3 Delivered=2 Dropped=1"))break;}Delay(1);}if(i==500)return 0;
 /* The normal recorder waits for its final OK requester; acknowledge it. */
 for(i=0;i<200;++i){notice=qa_window("NovaDock support recording");if(notice){
  for(button=notice->FirstGadget;button;button=button->NextGadget){for(label=button->GadgetText;label;label=label->NextText)if(label->IText&&!strcmp((char *)label->IText,"OK"))break;if(label)break;}
  if(button)break;
 }Delay(1);}if(i==200){puts("Recorder OK requester not found");return 0;}
 if(!click_recording(notice,"OK")||!qa_gone("NovaDock support recording")){puts("Recorder requester did not close");return 0;}
 return 1;
}
int main(int argc,char **argv)
{
 FILE *f;struct Window *w;struct Gadget *g;ULONG checked;struct NDDiagnosticsSnapshot d;static struct NDConfig saved;int i,ok=0,stage=1;
 if(!qa_init())return 20;if(qa_port(ND_CONTROL_PORT)||qa_port(AN_PORT))goto done;
 if(SystemTags("MakeDir RAM:NotisEnv RAM:NotisEnv/NovaDock RAM:NotisArc RAM:NotisArc/NovaDock",TAG_DONE)||SystemTags("Assign ENV: RAM:NotisEnv",TAG_DONE)||SystemTags("Assign ENVARC: RAM:NotisArc",TAG_DONE)||SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE))goto done;
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)goto done;
 fputs("SIDE=BOTTOM\nHOVER_MAGNIFICATION=0\nDRAG_BAR=NO\nREMOVABLE_MEDIA=NO\nTOOLTIPS=NO\nPLUGIN=Test|Plugins/NoticeProducer|NOTISTEST|METER|ON\nPLUGIN=Old alerts|Plugins/Alerts|ALERTS|METER|ON\nPLUGIN=Old chat|Plugins/AmIRC|AMIRC|METER|ON\nPLUGIN=Old rocket|Plugins/AmiRocket|AMIROCKET|METER|ON\nOPTION.AMIROCKET.PROGRAM=Work:Apps/AmiRocket\n",f);fclose(f);
 if(argc>1){if(SystemTags("Assign AmiNotis: QATests:",TAG_DONE)||!qa_start("QATests:NotisObserver"))goto stop;for(i=0;i<200&&!qa_port("NovaDock.QA.NotisObserver");++i)Delay(1);if(i==200)goto stop;}
 stage=2;if(!start_sink()||!qa_start("QATests:Dock/NovaDock"))goto stop;
 for(i=0;i<300;++i){if(nd_control_diagnostics(&d)==ND_CONTROL_OK&&d.plugin_count==1)break;Delay(2);}if(i==300)goto stop;
 stage=3;if(!trigger(1,1,1))goto stop;Delay(30);if(lines())goto stop;puts("PASS internal notices remain collected with forwarding off");
 stage=4;if(!qa_rexx("SET SEND_ALERTS_TO_AMINOTIS YES")||!qa_rexx("STATUS")||!trigger(2,2,2)||!delivered(1))goto stop;
 f=fopen("QATests:notis-received.log","r");if(f){char b[800];if(!fgets(b,sizeof(b),f)||!strstr(b,"NovaDock|NOTISTEST|Forward test|Case 2|1")){fclose(f);goto stop;}fclose(f);}else goto stop;
 if(nd_control_diagnostics(&d)!=ND_CONTROL_OK || d.aminotis_version!=1 || !d.aminotis_enabled || d.aminotis.delivered!=1 || d.aminotis.dropped)goto stop;
 puts("PASS versioned AmiNotis delivered/dropped counters available through diagnostics");
 puts("PASS enabled internal notice delivered through the real asynchronous SDK");
 stage=5;if(!trigger(3,2,2))goto stop;Delay(50);if(lines()!=1)goto stop;puts("PASS duplicate internal notice is not forwarded twice");
 stage=61;if(!stop_sink())goto stop;stage=62;if(!trigger(4,3,3))goto stop;Delay(30);stage=63;if(lines()!=1)goto stop;stage=64;if(!start_sink())goto stop;stage=65;if(!trigger(5,4,4))goto stop;stage=66;if(!delivered(2)){printf("Delivery lines=%d\n",lines());goto stop;}if(nd_control_diagnostics(&d)!=ND_CONTROL_OK || d.aminotis.delivered!=2 || d.aminotis.dropped!=1 || d.aminotis.unavailable!=1 || d.aminotis.last_failure!=ND_AMINOTIS_FAILURE_UNAVAILABLE)goto stop;puts("PASS service absence counters and last failure retained after recovery");
 puts("PASS service absence preserves local collection and restart restores forwarding without replay");
 stage=69;if(!forwarding_report())goto stop;puts("PASS packaged support report and timed capture include delivery counters and last failure");
 stage=7;if(!qa_rexx("SET SEND_ALERTS_TO_AMINOTIS NO")||!qa_rexx("STATUS")||!trigger(6,5,5))goto stop;Delay(30);if(lines()!=2)goto stop;
 stage=81;if(!qa_start("QATests:Dock/NovaDockPrefs PAGE=GENERAL"))goto stop;
 stage=82;w=qa_wait_window("NovaDock Preferences");if(!w){puts("No Preferences window");goto stop;}
 stage=83;if(!qa_bounds(w))goto stop;
 stage=84;g=qa_gadget(w,61);if(!g){puts("No forwarding gadget");goto stop;}
 checked=9;{ULONG got=GT_GetGadgetAttrs(g,w,NULL,GTCB_Checked,(ULONG)&checked,TAG_DONE);printf("Checkbox getter=%lu checked=%lu flags=%u\n",got,checked,g->Flags);if(got!=1||checked)goto stop;}
 stage=85;GT_SetGadgetAttrs(g,w,NULL,GTCB_Checked,TRUE,TAG_DONE);if(!qa_post(w,IDCMP_GADGETUP,1,61)||!qa_post(w,IDCMP_GADGETUP,0,21))goto stop;
 stage=86;if(!qa_gone("NovaDock Preferences"))goto stop;
 stage=87;{int loaded=nd_load_config(&saved,"ENV:NovaDock/novadock.cfg");printf("Saved config result=%d forwarding=%d\n",loaded,saved.send_alerts_aminotis);if(loaded!=0||!saved.send_alerts_aminotis)goto stop;}
 for(i=0;i<saved.count;++i)if(!strcmp(saved.items[i].id,"ALERTS")||!strcmp(saved.items[i].id,"AMIRC")||!strcmp(saved.items[i].id,"AMIROCKET"))goto stop;
 if(!nd_option_get(&saved,"AMIROCKET","PROGRAM")||strcmp(nd_option_get(&saved,"AMIROCKET","PROGRAM"),"Work:Apps/AmiRocket"))goto stop;
 puts("PASS General checkbox saves, legacy enabled tiles are retired and source settings survive");
 stage=91;if(!trigger(7,91,6)||!delivered(3)||!trigger(8,92,7)||!delivered(4))goto stop;
 {char line[1000];int found=0;FILE *longlog=fopen("QATests:notis-received.log","r");if(!longlog)goto stop;while(fgets(line,sizeof(line),longlog)){char *title=strchr(line,'|'),*body;if(!title)continue;title=strchr(title+1,'|');if(!title)continue;++title;body=strchr(title,'|');if(!body)continue;if(body-title==80 && strlen(body+1)>512)++found;}fclose(longlog);if(found!=2)goto stop;}
 puts("PASS negotiated long titles/bodies reach real service; differences beyond the history preview remain distinct");ok=1;
 stop:if(!ok)printf("FAIL AmiNotis native stage=%d\n",stage);qa_close("NovaDock Preferences");if(qa_port(ND_CONTROL_PORT))qa_stop();stop_sink();
 done:{struct MsgPort *observer;Forbid();observer=FindPort("NovaDock.QA.NotisObserver");if(observer)Signal(observer->mp_SigTask,SIGBREAKF_CTRL_C);Permit();}qa_finish();return ok?0:10;
}
