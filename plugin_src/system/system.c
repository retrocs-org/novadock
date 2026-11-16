/* Read-only CPU, Workbench library version and boot-relative E-clock time. */
#include <exec/types.h>
#include <exec/execbase.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <stdio.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_panel.h"
#include "nd_system_info.h"
struct Device *TimerBase=NULL;
const char nd_version[]=ND_PLUGIN_VER_STRING(SYSTEM,"System");
long __stack=16384;
int main(int argc,char **argv)
{
 struct NDClient client;struct NDPanel *panel=NULL;struct MsgPort *port;
 struct timerequest *timer=NULL;struct Library *wb;struct EClockVal clock;
 struct NDPanelField fields[3];static const char *buttons[]={"Close"};
 const char *cpu;char workbench[48],uptime[48],value[32],display[16];
 unsigned long rate,seconds,rotate=0;unsigned int event,elapsed=0;int reread=1,opened=0,action,which,result=20;
 if(argc<2)return 20;port=CreateMsgPort();if(!port)return 20;
 timer=(struct timerequest *)CreateIORequest(port,sizeof(*timer));
 if(!timer || OpenDevice("timer.device",UNIT_ECLOCK,(struct IORequest *)timer,0))goto done;
 opened=1;TimerBase=(struct Device *)timer->tr_node.io_Device;
 if(!nd_client_init(&client,argv[1]))goto done;
 cpu=nd_system_cpu(SysBase->AttnFlags);wb=OpenLibrary("workbench.library",0);
 if(wb){sprintf(workbench,"%u.%u",wb->lib_Version,wb->lib_Revision);CloseLibrary(wb);}else strcpy(workbench,"Unavailable");
 for(;;){
  if(reread){nd_plugin_option_string(argv[1],"DISPLAY","Rotate",display,sizeof(display));reread=0;}
  rate=ReadEClock(&clock);seconds=nd_system_seconds(clock.ev_hi,clock.ev_lo,rate);nd_system_uptime(seconds,uptime);
  which=!strcmp(display,"CPU") ? 0:!strcmp(display,"Workbench") ? 1:!strcmp(display,"Uptime") ? 2:(int)((rotate/250UL)%3UL);
  if(which==0)sprintf(value,"CPU %s",cpu);else if(which==1)sprintf(value,"WB %.20s",workbench);else sprintf(value,"%lu d %02lu:%02lu",seconds/86400UL,(seconds/3600UL)%24UL,(seconds/60UL)%60UL);
  event=nd_client_update_quiet(&client,"SYSTEM",value,ND_NO_METER,elapsed,500U);
  if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS)reread=1;
  if((event&ND_EVENT_CLICK) && !panel){
   fields[0].label="CPU";fields[0].value=cpu;fields[0].limit=0;
   fields[1].label="Workbench";fields[1].value=workbench;fields[1].limit=0;
   fields[2].label="Uptime";fields[2].value=uptime;fields[2].limit=0;
   panel=nd_panel_open("NovaDock System",fields,3,buttons,1);
  }else if(event&ND_EVENT_CLICK)nd_panel_raise(panel);
  if(panel)nd_panel_set(panel,2,uptime);
  action=nd_panel_event(panel);if(action==-1 || action==100){nd_panel_close(panel);panel=NULL;}
  elapsed=nd_client_wait_signals(&client,50U,nd_panel_signals(panel));rotate=(rotate+elapsed)%750UL;
 }
 nd_panel_close(panel);nd_client_close(&client);result=0;
done:
 if(opened)CloseDevice((struct IORequest *)timer);if(timer)DeleteIORequest((struct IORequest *)timer);DeleteMsgPort(port);return result;
}
