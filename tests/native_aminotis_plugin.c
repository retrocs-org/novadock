/* AMINOTIS tile against the real AmiNotis service, offline disposable guest only.
 * Covers the sad offline state, live discovery, unread badge text, quiet mode,
 * read-all, service stop/restart recovery and the click panel. */
#include "native_qa_ui.h"
#include "nd_control.h"
#include "../plugin_src/aminotis/sdk/aminotis.h"
long __stack=65536;
static struct ANClient sdk;
static struct ANResponse reply;
static char value[32];

static int tile(const char *expect,int prefix)
{
 struct NDControlPluginHealthMessage h;int i;
 for(i=0;i<400;++i){
  memset(&h,0,sizeof(h));
  if(nd_control_plugin_health("AMINOTIS",&h)==ND_CONTROL_OK){
   strcpy(value,h.value);
   if(prefix ? !strncmp(h.value,expect,strlen(expect)):!strcmp(h.value,expect))return 1;
  }
  Delay(2);
 }
 printf("Tile value '%s', expected %s'%s'\n",value,prefix ? "prefix ":"",expect);return 0;
}
static int call(ULONG operation,ULONG v,const char *title)
{
 struct ANRequest r;int result;
 an_request(&r,(enum ANOperation)operation);r.value=v;
 if(operation==AN_POST){strcpy(r.event.app,"AmiNotisQA");strcpy(r.event.name,"AmiNotis QA");strcpy(r.event.source,"native");
  strcpy(r.event.event,title);strcpy(r.event.title,title);strcpy(r.event.body,"Posted by the AMINOTIS native test");}
 if(operation==AN_READ_ALL){struct ANRequest s;an_request(&s,AN_STATUS);if(an_call(&sdk,&s,&reply,100)!=AN_OK)return 0;an_use_cursor(&r,&reply.status);}
 result=an_call(&sdk,&r,&reply,100);an_close(&sdk);
 if(result!=AN_OK&&result!=AN_REPLACED){printf("SDK operation %lu returned %d\n",operation,result);return 0;}
 return 1;
}
static int service_up(void){int i;if(!qa_start("QATests:AmiNotisService"))return 0;for(i=0;i<300;++i){if(qa_port(AN_PORT))return 1;Delay(1);}return 0;}
static int service_down(void){int i;(void)call(AN_STOP,0,"");for(i=0;i<400;++i){if(!qa_port(AN_PORT))return 1;Delay(1);}return 0;}

int main(void)
{
 FILE *f;int ok=0,stage=1,i;struct NDDiagnosticsSnapshot d;
 if(!qa_init())return 20;if(qa_port(ND_CONTROL_PORT)||qa_port(AN_PORT))goto done;
 if(SystemTags("MakeDir RAM:NotisEnv RAM:NotisEnv/NovaDock RAM:NotisArc RAM:NotisArc/NovaDock",TAG_DONE)||SystemTags("Assign ENV: RAM:NotisEnv",TAG_DONE)||
    SystemTags("Assign ENVARC: RAM:NotisArc",TAG_DONE)||SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE)||SystemTags("Assign AmiNotis: QATests:",TAG_DONE))goto done;
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)goto done;
 fputs("SIDE=BOTTOM\nHOVER_MAGNIFICATION=0\nDRAG_BAR=NO\nREMOVABLE_MEDIA=NO\nTOOLTIPS=NO\nPLUGIN=AmiNotis|Plugins/AmiNotis|AMINOTIS|METER|ON\n",f);fclose(f);

 stage=2;if(!qa_start("QATests:Dock/NovaDock"))goto stop;
 for(i=0;i<300;++i){if(nd_control_diagnostics(&d)==ND_CONTROL_OK&&d.plugin_count==1)break;Delay(2);}if(i==300)goto stop;
 stage=3;if(!tile("AmiNotis not running",0))goto stop;puts("PASS sad offline state without the service");

 stage=4;if(!service_up()||!tile("No new notifications",0))goto stop;puts("PASS discovers a service started after the dock");
 stage=5;if(!call(AN_POST,0,"First")||!tile("1 unread",0))goto stop;
 if(!call(AN_POST,0,"Second")||!tile("2 unread",0))goto stop;puts("PASS live unread count from the parked watch");
 stage=6;if(!call(AN_SET_DND,3600,"")||!tile("Quiet until ",1))goto stop;printf("PASS quiet mode shown: %s\n",value);
 stage=7;if(!call(AN_SET_DND,0,"")||!tile("2 unread",0))goto stop;
 if(!call(AN_READ_ALL,0,"")||!tile("No new notifications",0))goto stop;puts("PASS quiet off and read-all reach the tile");

 stage=8;if(!qa_rexx("CLICK 1")||!qa_wait_window("AmiNotis"))goto stop;Delay(25);
 if(!qa_close("AmiNotis"))goto stop;puts("PASS click opens and closes the inbox panel");

 stage=9;if(!service_down()||!tile("AmiNotis not running",0))goto stop;puts("PASS service stop returns the tile to the sad state");
 stage=10;if(!service_up()||!tile("No new notifications",0))goto stop;
 if(!call(AN_POST,0,"Third")||!tile("1 unread",0))goto stop;puts("PASS recovers after a service restart");
 stage=11;if(nd_control_diagnostics(&d)!=ND_CONTROL_OK||d.plugin_count!=1)goto stop;
 ok=1;puts("PASS AMINOTIS native service checks");
 stop:if(!ok)printf("FAIL AMINOTIS native stage=%d\n",stage);qa_close("AmiNotis");if(qa_port(ND_CONTROL_PORT))qa_stop();if(qa_port(AN_PORT))service_down();
 done:an_close(&sdk);qa_finish();return ok?0:10;
}
