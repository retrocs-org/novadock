/* ACTIONS: one independently configured, explicit click action per tile. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <rexx/rxslib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/rexxsyslib.h>
#include <stdio.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_panel.h"
#include "nd_actions.h"
struct IntuitionBase *IntuitionBase=NULL;
struct RxsLib *RexxSysBase=NULL;
const char nd_version[]=ND_PLUGIN_VER_STRING(ACTIONS,"Actions");
long __stack=16384;
#include "nd_actions_native.h"
struct Settings {char label[32],action[96],confirm[8];};
static void read_settings(const char *id,struct Settings *s)
{
 struct NDOptionString fields[3];
 fields[0].key="LABEL";fields[0].fallback="ACTIONS";fields[0].out=s->label;fields[0].limit=sizeof(s->label);
 fields[1].key="ACTION";fields[1].fallback="Restart Workbench";fields[1].out=s->action;fields[1].limit=sizeof(s->action);
 fields[2].key="CONFIRM";fields[2].fallback="YES";fields[2].out=s->confirm;fields[2].limit=sizeof(s->confirm);
 nd_plugin_option_strings(id,fields,3);
 if(!s->label[0])strcpy(s->label,"ACTIONS");
}
static struct NDPanel *confirm_action(int action)
{
 struct NDPanelField fields[2];static const char *buttons[]={"Run action","Cancel"};
 fields[0].label="Action";fields[0].value=nd_actions[action].name;fields[0].limit=0;
 fields[1].label="Confirm";fields[1].value=nd_actions[action].notice;fields[1].limit=0;
 return nd_panel_open("NovaDock - confirm action",fields,2,buttons,2);
}
int main(int argc,char **argv)
{
 struct NDClient client;struct Settings settings;struct NDPanel *panel=NULL;
 unsigned int event,elapsed=0;int reread=1,pending=-1,index,button,execute;
 char status[32]="";const char *result;
 if(argc<2 || !nd_client_init(&client,argv[1]))return 20;
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);
 RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",0);
 if(!IntuitionBase){nd_client_close(&client);if(RexxSysBase)CloseLibrary((struct Library *)RexxSysBase);return 20;}
 for(;;){
  if(reread){nd_panel_close(panel);panel=NULL;pending=-1;read_settings(argv[1],&settings);status[0]=0;reread=0;}
  event=nd_client_update_quiet(&client,settings.label,status[0] ? status:settings.action,ND_NO_METER,elapsed,500U);
  if(event&ND_EVENT_QUIT)break;
  if(event&ND_EVENT_SETTINGS){reread=1;continue;}
  execute=-1;
  if(event&ND_EVENT_CLICK){
   status[0]=0;index=nd_action_index(settings.action);
   if(panel)nd_panel_raise(panel);
   else if(index<0)strcpy(status,"INVALID ACTION");
   else if(strcmp(settings.confirm,"NO")){
    panel=confirm_action(index);pending=panel ? index:-1;
    if(!panel)strcpy(status,"CONFIRMATION UNAVAILABLE");
   }else execute=index;
  }
  button=nd_panel_event(panel);
  if(button==100 && pending>=0)execute=pending;
  if(button==-1 || button==100 || button==101){nd_panel_close(panel);panel=NULL;pending=-1;}
  if(execute>=0){
   result=run_action(execute);strncpy(status,result ? result:"DONE",sizeof(status)-1);status[sizeof(status)-1]=0;
  }
  elapsed=nd_client_wait_signals(&client,500U,nd_panel_signals(panel));
 }
 nd_panel_close(panel);
 if(RexxSysBase)CloseLibrary((struct Library *)RexxSysBase);
 CloseLibrary((struct Library *)IntuitionBase);nd_client_close(&client);return 0;
}
