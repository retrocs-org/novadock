/* Countdown and non-modal alarm; no filesystem work in the timer loop. */
#include <exec/types.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <stdio.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_panel.h"
#include "nd_timer.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(TIMER,"Timer");
long __stack=16384;
struct Device *TimerBase=NULL;
static struct NDPanel *show_timer(struct NDPanel *panel,unsigned long seconds,const char *status)
{
    struct NDPanelField fields[2];char number[16];
    static const char *buttons[]={"Start","Pause","Reset","Close"};
    if(panel){nd_panel_raise(panel);return panel;}
    sprintf(number,"%lu",seconds);
    fields[0].label="Seconds";fields[0].value=number;fields[0].limit=8;
    fields[1].label="Status";fields[1].value=status;fields[1].limit=0;
    return nd_panel_open("NovaDock Timer",fields,2,buttons,4);
}
int main(int argc,char **argv)
{
    struct NDClient client;struct NDPanel *panel=NULL;struct NDCountdown timer;
    struct MsgPort *port;struct timerequest *request;struct EClockVal now;
    struct NDOptionString options[3];
    char duration[16],message[80],beep[8],value[32],status[96],number[16];
    unsigned long rate,previous,delta,seconds=300;unsigned int event,elapsed=0,wait,meter;
    int action,reread=1,clock_open=0,result=20;
    if(argc<2)return 20;
    port=CreateMsgPort();if(!port)return 20;
    request=(struct timerequest *)CreateIORequest(port,sizeof(*request));
    if(request && !OpenDevice("timer.device",UNIT_MICROHZ,(struct IORequest *)request,0)){
        clock_open=1;TimerBase=(struct Device *)request->tr_node.io_Device;
    }
    if(!clock_open || !nd_client_init(&client,argv[1]))goto cleanup;
    rate=ReadEClock(&now);previous=now.ev_lo;if(!rate){nd_client_close(&client);goto cleanup;}
    nd_countdown_reset(&timer,300);strcpy(status,"Ready");
    for(;;){
        if(reread){
            options[0].key="SECONDS";options[0].fallback="300";options[0].out=duration;options[0].limit=sizeof(duration);
            options[1].key="MESSAGE";options[1].fallback="Time is up";options[1].out=message;options[1].limit=sizeof(message);
            options[2].key="BEEP";options[2].fallback="YES";options[2].out=beep;options[2].limit=sizeof(beep);
            nd_plugin_option_strings(argv[1],options,3);
            if(!nd_countdown_seconds(duration,&seconds))seconds=300;
            if(!timer.started)nd_countdown_reset(&timer,seconds);
            reread=0;
        }
        ReadEClock(&now);delta=(unsigned long)(now.ev_lo-previous);previous=now.ev_lo;
        if(nd_countdown_advance(&timer,delta,rate)){
            strcpy(status,message);panel=show_timer(panel,timer.duration,status);
            if(!strcmp(beep,"YES"))nd_panel_beep(panel);
        }
        if(timer.expired)strcpy(value,"TIME IS UP");
        else sprintf(value,"%02lu:%02lu:%02lu",timer.remaining/3600UL,(timer.remaining/60UL)%60UL,timer.remaining%60UL);
        if(timer.running)sprintf(status,"Running: %s",value);
        else if(!timer.expired)strcpy(status,timer.started ? "Paused":"Ready");
        nd_panel_set(panel,1,status);
        meter=(unsigned int)((timer.duration-timer.remaining)*100UL/timer.duration);
        event=nd_client_update_quiet(&client,timer.running ? "TIMER":timer.expired ? "ALARM":timer.started ? "PAUSED":"TIMER",value,meter,elapsed,500U);
        if(event&ND_EVENT_QUIT)break;
        if(event&ND_EVENT_SETTINGS)reread=1;
        if(event&ND_EVENT_CLICK)panel=show_timer(panel,timer.started ? timer.duration:seconds,status);
        action=nd_panel_event(panel);
        if(action==100){
            if(timer.running)nd_panel_title(panel,"Timer is already running");
            else if(timer.started && !timer.expired){nd_countdown_start(&timer);nd_panel_title(panel,"NovaDock Timer");}
            else if(nd_countdown_seconds(nd_panel_get(panel,0),&seconds)){
                nd_countdown_reset(&timer,seconds);nd_countdown_start(&timer);nd_panel_title(panel,"NovaDock Timer");
            }else nd_panel_title(panel,"Enter a duration from 1 to 86400 seconds");
            /* Start/resume only counts time after this action. */
            ReadEClock(&now);previous=now.ev_lo;
        }else if(action==101){timer.running=0;nd_panel_title(panel,"NovaDock Timer");}
        else if(action==102){
            if(nd_countdown_seconds(nd_panel_get(panel,0),&seconds)){nd_countdown_reset(&timer,seconds);nd_panel_title(panel,"NovaDock Timer");}
            else nd_panel_title(panel,"Enter a duration from 1 to 86400 seconds");
        }else if(action==-1 || action==103){nd_panel_close(panel);panel=NULL;}
        wait=panel ? 5U:timer.running ? 50U:500U;
        elapsed=nd_client_wait(&client,wait);
    }
    nd_panel_close(panel);nd_client_close(&client);result=0;
cleanup:
    if(clock_open)CloseDevice((struct IORequest *)request);
    if(request)DeleteIORequest((struct IORequest *)request);
    DeleteMsgPort(port);return result;
}
