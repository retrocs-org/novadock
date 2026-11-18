/* Disposable integration and worker-ownership checks; never use a live disk. */
#include <exec/types.h>
#include <exec/execbase.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <graphics/displayinfo.h>
#include "nd_scrmode.h"
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <workbench/workbench.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <stdio.h>
#include <string.h>
#include "nd_control_client.h"
#include "nd_protocol.h"
#include "nd_panel.h"
#include "nd_list.h"
#include "nd_task.h"
#include "nd_text.h"
#include "nd_tooltypes.h"
#include "nd_hass_bridge.h"
#include "nd_hass_compat.h"
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;struct Library *IconBase=NULL;long __stack=65536;
static FILE *report;static int required_height;
static int check(int ok,const char *name){fprintf(report,"%s: %s\n",ok ? "PASS":"FAIL",name);fflush(report);return ok;}
static int run(const char *command,int async)
{BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc;if(!in || !out){if(in)Close(in);if(out)Close(out);return 0;}rc=SystemTags((STRPTR)command,SYS_Input,in,SYS_Output,out,SYS_Asynch,async,NP_StackSize,65536UL,TAG_DONE);if(rc==-1 || !async){Close(in);Close(out);}return async ? rc!=-1:rc==0;}
static int present(const char *name){struct MsgPort *p;Forbid();p=FindPort((STRPTR)name);Permit();return p!=NULL;}
static int work(unsigned int action,void *data,unsigned int bytes,const char *parent)
{(void)parent;if(action!=7 || bytes!=16)return 0;Delay(75);strcpy(data,"finished");return 1;}
static int lifecycle(void)
{struct NDTask j,duplicate;struct NDTaskPacket *m;unsigned int n,k;ULONG before,after;char data[16]="test";memset(&j,0,sizeof(j));memset(&duplicate,0,sizeof(duplicate));before=AvailMem(MEMF_PUBLIC);
 for(k=0;k<5;++k){if(!nd_task_start(&j,"QATests:SixCheck","NovaDock.QA.Worker",7,data,sizeof(data)))return check(0,"start background worker");for(n=0;n<250;++n){m=nd_task_poll(&j);if(m){FreeVec(m);return check(0,"worker unexpectedly finished before guard");}if(present("NovaDock.QA.Worker"))break;Delay(2);}if(n==250)return check(0,"worker guard registered");if(!check(!nd_task_start(&duplicate,"QATests:SixCheck","NovaDock.QA.Worker",7,data,sizeof(data)),"duplicate worker refused"))return 0;
  if(k==0)nd_task_cancel(&j);else if(k==1){nd_task_close(&j);Delay(100);if(!check(!present("NovaDock.QA.Worker"),"late result cleans up after parent closes"))return 0;continue;}
  for(n=0;n<250 && j.port;++n){m=nd_task_poll(&j);if(m){if(k==0 || m->result!=1 || strcmp((char *)m->data,"finished")){FreeVec(m);nd_task_close(&j);return check(0,"worker result ownership");}FreeVec(m);}Delay(2);}if(!check(!j.port&&!present("NovaDock.QA.Worker"),k==0 ? "cancelled result retired without replay":"completed worker cleaned up")){nd_task_close(&j);return 0;}}
 if(!nd_task_start(&j,"QATests:SixCheck","NovaDock.QA.Worker",7,data,sizeof(data)))return 0;nd_task_close(&j);Delay(150);after=AvailMem(MEMF_PUBLIC);fprintf(report,"Worker memory before=%lu after=%lu\n",before,after);fflush(report);return check(after+8192UL>=before && !present("NovaDock.QA.Worker"),"no accumulating worker allocations or abandoned guard");}
static int fingerprint(const char *path,unsigned long *hash)
{BPTR f=Open((STRPTR)path,MODE_OLDFILE);unsigned char b[1024];LONG n;unsigned long h=2166136261UL;if(!f)return 0;while((n=Read(f,b,sizeof(b)))>0)h=nd_tt_hash(h,b,(unsigned int)n);Close(f);*hash=h;return n==0;}
static int icon_test(void)
{struct DiskObject *icon;STRPTR *old,types[]={"ORIGINAL=YES",NULL};unsigned long before,backup;int ok;IconBase=OpenLibrary("icon.library",37);if(!IconBase)return 0;icon=GetDefDiskObject(WBTOOL);if(!icon){CloseLibrary(IconBase);return 0;}old=icon->do_ToolTypes;icon->do_ToolTypes=types;ok=PutDiskObject("QATests:Editable",icon)!=0;icon->do_ToolTypes=old;FreeDiskObject(icon);if(!ok || !fingerprint("QATests:Editable.info",&before)){CloseLibrary(IconBase);return 0;}
 ok=run("QATests:ToolTypes SELFTEST QATests:Editable",0);if(ok){icon=GetDiskObject("QATests:Editable");ok=icon && icon->do_StackSize==16384 && icon->do_DefaultTool && !strcmp(icon->do_DefaultTool,"C:IconX") && icon->do_ToolTypes && !strcmp(icon->do_ToolTypes[0],"ORIGINAL=YES");if(icon)FreeDiskObject(icon);}CloseLibrary(IconBase);IconBase=NULL;return check(ok && fingerprint("QATests:Editable.info.bak",&backup) && before==backup,"ToolTypes save preserves original bytes in backup and updates metadata");}
static struct AmiHASSMessage *exchange(struct AmiHASSClient *client,int kind,const char *id)
{struct AmiHASSMessage *m;unsigned int n;if(!amihass_send(client,kind,"SDK.Six",id,NULL,0,AMIHASS_QUERY_ONCE,NULL,NULL))return NULL;for(n=0;n<200;++n){m=amihass_reply(client);if(m)return m;Delay(2);}return NULL;}
static int sample_test(void)
{
 struct AmiHASSClient observer;struct NDHassBridge tile;struct AmiHASSMessage *m;struct AHControls c;const char *ids[]={"fan.amihass_sample_bedroom","cover.amihass_sample_garage","cover.amihass_sample_garage","climate.amihass_sample_hallway"};const char *actions[]={"Fan speed 65","Position 70","Close cover","Temperature 22.5"};unsigned int i,k;FILE *f;BPTR lock;int ok=0;memset(&observer,0,sizeof(observer));memset(&tile,0,sizeof(tile));
 m=exchange(&observer,AMIHASS_STOP,NULL);if(m)FreeVec(m);amihass_client_close(&observer);for(i=0;i<200 && present(AMIHASS_PORT);++i)Delay(2);if(present(AMIHASS_PORT))return check(0,"stop cloned service");
 lock=CreateDir("ENVARC:AmiHASS");if(lock)UnLock(lock);f=fopen("ENVARC:AmiHASS/AmiHASS.cfg","w");if(!f)return 0;fputs("AMIHASSCFG1\n\n30 0 0 1 60 1 1\n\nQATests:\nQATests:\nSYS:WBStartup\n",f);fclose(f);
 if(!run("QATests:AmiHASSService SERVICE",1))return 0;for(i=0;i<250 && !present(AMIHASS_PORT);++i)Delay(2);m=exchange(&observer,AMIHASS_STATUS,NULL);if(!m)goto done;i=m->status.connected && !strcmp(m->item.state,AMIHASS_SERVER_SAMPLE) && nd_hass_features(m)==ND_HASS_FEATURE_CURRENT;FreeVec(m);if(!check(i,"sample service required; no physical device actions"))goto done;
 if(!check(run("QATests:HASS PICKER-SELFTEST",0),"native paged room chooser selects stable Study ID"))goto done;
 for(k=0;k<4;++k){nd_hass_bridge_close(&tile);for(i=0;i<250;++i){nd_hass_bridge_poll(&tile,"HASS.Six",ids[k],5);if(tile.result==AMIHASS_OK && !tile.client.pending)break;Delay(2);}if(i==250)goto done;nd_hass_bridge_action(&tile,"HASS.Six",ids[k],actions[k]);if(tile.result!=AMIHASS_WAIT)goto done;
  for(i=0;i<250;++i){nd_hass_bridge_poll(&tile,"HASS.Six",ids[k],10);if(tile.result==AMIHASS_OK && !tile.client.pending && amihass_control_matches(&tile.controls,tile.entity.state,actions[k]))break;Delay(2);}if(!check(i<250,actions[k]))goto done;
  m=exchange(&observer,AMIHASS_QUERY,ids[k]);if(!m)goto done;amihass_controls_items(m->items,m->count,&c);i=m->result==AMIHASS_OK && amihass_control_matches(&c,m->item.state,actions[k]);FreeVec(m);if(!check(i,"independent SDK query confirms sample state"))goto done;
 }
 ok=1;
done:nd_hass_bridge_close(&tile);m=exchange(&observer,AMIHASS_STOP,NULL);if(m)FreeVec(m);amihass_client_close(&observer);return ok;
}
static int screen_480(void)
{struct DimensionInfo dims;struct DisplayInfo display;struct NDScreenModePreset prefs;unsigned char bytes[62];ULONG id=INVALID_ID;FILE *f;int found=0;
 GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!GfxBase)return 0;
 while((id=NextDisplayInfo(id))!=INVALID_ID){if(ModeNotAvailable(id)||!GetDisplayInfoData(NULL,(UBYTE *)&dims,sizeof(dims),DTAG_DIMS,id)||!GetDisplayInfoData(NULL,(UBYTE *)&display,sizeof(display),DTAG_DISP,id)||display.NotAvailable||!(display.PropertyFlags&DIPF_IS_WB))continue;
  if(dims.Nominal.MaxX-dims.Nominal.MinX+1==640 && dims.Nominal.MaxY-dims.Nominal.MinY+1==480){memset(&prefs,0,sizeof(prefs));prefs.display_id=id;prefs.width=640;prefs.height=480;prefs.depth=dims.MaxDepth<8 ? dims.MaxDepth:8;prefs.control=1;found=1;break;}}
 CloseLibrary((struct Library *)GfxBase);GfxBase=NULL;if(!found||nd_scrmode_encode(&prefs,bytes,sizeof(bytes))!=sizeof(bytes))return check(0,"640x480 native display mode available");
 f=fopen("QATests:mode480.prefs","wb");if(!f)return 0;found=fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes);if(fclose(f)||!found)return 0;
 if(!run("SYS:Prefs/ScreenMode FROM QATests:mode480.prefs USE",0))return check(0,"apply disposable 640x480 screen mode");Delay(50);return 1;}
static int screen_check(void)
{struct Screen *screen;int ok=0;IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);if(!IntuitionBase)return 0;screen=LockPubScreen("Workbench");if(screen){fprintf(report,"Screen=%dx%d depth=%d font=%d; CPU flags=%lu; Fast=%lu Chip=%lu\n",screen->Width,screen->Height,screen->RastPort.BitMap->Depth,screen->Font->ta_YSize,(unsigned long)SysBase->AttnFlags,AvailMem(MEMF_FAST),AvailMem(MEMF_CHIP));fflush(report);ok=required_height ? screen->Width==640 && screen->Height==required_height:screen->Width>=640 && screen->Height>=256;UnlockPubScreen(NULL,screen);}CloseLibrary((struct Library *)IntuitionBase);IntuitionBase=NULL;return check(ok,"native public screen meets the requested QA geometry");}
static int runtime_config(int monitors)
{FILE *f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)return 0;fputs("SIDE=BOTTOM\nROWS=1\nREMOVABLE_MEDIA=NO\nREFRESH_MODE=LOW_MEMORY\nPLUGIN=CPU|QATests:Release04/NovaDock/Plugins/Activity|CPU|METER|ON|YES\nPLUGIN=RAM|QATests:Release04/NovaDock/Plugins/Memory|RAM|METER|ON|YES\nPLUGIN=CLOCK|QATests:Release04/NovaDock/Plugins/Clock|CLOCK|METER|ON|YES\n",f);if(monitors)fputs("PLUGIN=WATCH|QATests:Release04/NovaDock/Plugins/Watch|WATCH|METER|ON|YES\nPLUGIN=LOGS|QATests:Release04/NovaDock/Plugins/Logs|LOGS|METER|ON|YES\nOPTION.WATCH.DRAWER=QATests:RuntimeWatch\nOPTION.WATCH.INTERVAL=2\nOPTION.LOGS.FILE=QATests:runtime.log\nOPTION.LOGS.INTERVAL=2\n",f);return fclose(f)==0;}
static int wait_dock(int count)
{struct NDDiagnosticsSnapshot d;unsigned int i;for(i=0;i<300;++i){if(!count){if(!present(ND_CONTROL_PORT))return 1;}else if(nd_control_diagnostics(&d)==ND_CONTROL_OK && d.plugin_count==count && !d.watchdog_restarts)return 1;Delay(5);}return 0;}
static int runtime_tasks(struct Task *tasks[6],int capture)
{static const char *ports[]={ND_CONTROL_PORT,ND_WAKE_PORT_PREFIX "CPU",ND_WAKE_PORT_PREFIX "RAM",ND_WAKE_PORT_PREFIX "CLOCK",ND_WAKE_PORT_PREFIX "WATCH",ND_WAKE_PORT_PREFIX "LOGS"};
 unsigned int i,j;struct MsgPort *port;struct List *lists[2];struct Node *node;int live=0;
 if(capture){Forbid();for(i=0;i<6;++i){port=FindPort((STRPTR)ports[i]);tasks[i]=port ? port->mp_SigTask:NULL;}Permit();for(i=0;i<6;++i)if(!tasks[i])return 0;return 1;}
 lists[0]=&SysBase->TaskReady;lists[1]=&SysBase->TaskWait;
 Disable();for(j=0;j<2;++j)for(node=lists[j]->lh_Head;node->ln_Succ;node=node->ln_Succ)for(i=0;i<6;++i)if(node==(struct Node *)tasks[i])live=1;Enable();return live;
}
static int runtime_cycles(void)
{unsigned int k,i;ULONG baseline=0,after=0;BPTR dir;FILE *f;struct NDControlPluginHealthMessage h;struct NDPanelField field={"Value","test",32};const char *buttons[]={"Close"};struct NDPanel *panel;struct Task *tasks[6];
 dir=CreateDir("QATests:RuntimeWatch");if(dir)UnLock(dir);dir=CreateDir("ENV:NovaDock");if(dir)UnLock(dir);f=fopen("QATests:runtime.log","w");if(!f)return 0;fputs("WARNING sample log\n",f);fclose(f);
 for(k=0;k<4;++k){if(!runtime_config(1)||!run("QATests:Release04/NovaDock/NovaDock",1)||!wait_dock(5))return check(0,"dock starts with five plugins");
  for(i=0;i<150;++i){if(nd_control_plugin_health("WATCH",&h)==ND_CONTROL_OK && h.online)break;Delay(5);}if(i==150)return check(0,"WATCH starts on constrained machine");
  for(i=0;i<150 && !runtime_tasks(tasks,1);++i)Delay(5);if(i==150)return check(0,"all owned plugin tasks registered");panel=nd_panel_open("Native release open/close",&field,1,buttons,1);if(!panel)return check(0,"settings panel opens during monitor activity");Delay(2);nd_panel_event(panel);nd_panel_close(panel);
  for(i=0;i<3;++i){if(nd_control_reload()!=ND_CONTROL_OK)return 0;Delay(15);}
  if(!runtime_config(0)||nd_control_reload()!=ND_CONTROL_OK||!wait_dock(3))return check(0,"removing monitor plugins drains their workers");
  if(nd_control_send(ND_CONTROL_HIDE)!=ND_CONTROL_OK||nd_control_send(ND_CONTROL_SHOW)!=ND_CONTROL_OK||nd_control_send(ND_CONTROL_QUIT)!=ND_CONTROL_OK||!wait_dock(0))return check(0,"hide/show/shutdown remains responsive");for(i=0;i<300 && runtime_tasks(tasks,0);++i)Delay(5);if(i==300)return check(0,"all owned tasks exit after shutdown");Delay(100);after=AvailMem(MEMF_PUBLIC);fprintf(report,"Runtime cycle %u: public free=%lu\n",k,after);fflush(report);if(!k)baseline=after;
 }return check(after+65536UL>=baseline,"four startup/reload/remove/window/shutdown cycles have no accumulating memory loss");}
int main(int argc,char **argv)
{
 BPTR marker;struct NDPanel *p,*q;struct NDPanelField field={"Value","37",ND_PANEL_PERCENT};const char *b[]={"Close"};struct NDList *list;const char *nine[]={"Copy line","Open file","Mark read","Clear history","Check now","Pause view","Warnings/errors","Export...","Close"};const char *rows[]={"First","Last"};int result=10;
 if(argc==3 && !strcmp(argv[1],"TASK"))return nd_task_worker(argv[2],work);
 marker=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!marker)return 20;UnLock(marker);report=fopen("QATests:six-results.txt","w");if(!report)return 20;Delay(250);nd_control_send(ND_CONTROL_QUIT);Delay(100);
 if(argc==2 && (!strcmp(argv[1],"SCREEN480") || !strcmp(argv[1],"RUNTIME480"))){required_height=480;if(!screen_480())goto done;}
 if(!screen_check())goto done;
 if(argc==2 && (!strcmp(argv[1],"RUNTIME") || !strcmp(argv[1],"RUNTIME480"))){result=runtime_cycles() ? 0:10;goto done;}
 if(!check(run("QATests:Calc SELFTEST",0),"native CALC decimal and hex evaluation"))goto done;
 p=nd_panel_open("First native panel",&field,1,b,1);q=nd_panel_open("Second native panel",&field,1,b,1);if(!p || !q){nd_panel_close(p);nd_panel_close(q);goto done;}nd_panel_close(q);nd_panel_close(NULL);if(!check(nd_panel_percent_get(p,0)==37,"closing one panel keeps the other panel functional")){nd_panel_close(p);goto done;}nd_panel_close(p);
 list=nd_list_open("LOGS nine-button viewer",nine,9);if(!check(list!=NULL,"native nine-button filtered viewer opens"))goto done;nd_list_rows(list,rows,2);nd_list_follow(list,1);nd_list_button(list,5,"Resume view");nd_list_follow(list,0);if(!check(!nd_list_following(list),"tail following can be paused")){nd_list_close(list);goto done;}nd_list_close(list);
 if(!lifecycle() || !icon_test())goto done;
 if(!check(nd_text_export("QATests:export.txt","line one\nline two\n",18)==1 && nd_text_export("QATests:export.txt","replace",7)==-4,"LOGS export creates a file and refuses overwrite"))goto done;
 if(!check(run("QATests:HASS CONTROL-SELFTEST",0),"native HASS light, climate, fan and cover layouts"))goto done;
 if(!sample_test() || !runtime_cycles())goto done;result=0;
done:fclose(report);return result;
}
