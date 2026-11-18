/* Bounded native reader for the dock's session notification history. */
#include <exec/types.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_client.h"
#include "nd_panel.h"
#include "nd_control_client.h"
#include "nd_option_runtime.h"
#include "nd_plugin_versions.h"
const char nd_version[]=ND_PLUGIN_VER_STRING(ALERTS,"Alerts");long __stack=32768;
static struct NDControlAlertsMessage page;
static struct NDPanel *open_history(void)
{struct NDPanelField fields[ND_ALERT_PAGE];static const char *buttons[]={"Previous","Next","Mark read","Clear","Close"};unsigned int i;for(i=0;i<ND_ALERT_PAGE;++i){fields[i].label="";fields[i].value="";fields[i].limit=0;}return nd_panel_open("NovaDock Alerts",fields,ND_ALERT_PAGE,buttons,5);}
int main(int argc,char **argv)
{
 struct NDClient client;struct NDPanel *panel=NULL;unsigned int elapsed=0,event,previous=0,i;int action,sound=0;char badge[8],value[32],line[128],label[32],option[8];unsigned long through=0;
 if(argc<2 || !nd_client_init(&client,argv[1]))return 20;memset(&page,0,sizeof(page));
 for(;;){
  if(nd_control_alerts(&page)!=ND_CONTROL_OK){page.total=page.unread=page.count=0;strcpy(value,"History unavailable");}
  else{sprintf(value,"%u unread / %u alerts",page.unread,page.total);through=page.through;}
  if(sound && page.unread>previous)nd_panel_beep(panel);previous=page.unread;
  sprintf(badge,"%u",page.unread);event=nd_client_set_badge(&client,page.unread ? badge:"");event|=nd_client_update_art_quiet(&client,"ALERTS",value,"alerts",page.unread ? 1:0,elapsed,250);
  if(event&ND_EVENT_QUIT)break;
  if(event&ND_EVENT_SETTINGS || !elapsed){nd_plugin_option_string(argv[1],"SOUND","NO",option,sizeof(option));sound=!strcmp(option,"YES");}
  if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=open_history();}
  if(panel){for(i=0;i<ND_ALERT_PAGE;++i){if(i<page.count){struct NDAlertEntry *e=&page.entries[i];sprintf(label,"%s%02lu:%02lu %.15s",e->read ? "":"*",e->seconds/3600UL,(e->seconds/60UL)%60UL,e->source);sprintf(line,"%s - %.80s",e->title,e->text);}else{label[0]=0;strcpy(line,i==0 ? "No notifications in this dock session.":"");}nd_panel_set_label(panel,i,label);nd_panel_set(panel,i,line);}}
  while(panel && (action=nd_panel_event(panel))!=0){
   if(action==-1 || action==104){nd_panel_close(panel);panel=NULL;}
   else if(action==100){if(page.offset>=ND_ALERT_PAGE)page.offset-=ND_ALERT_PAGE;}
   else if(action==101){if(page.offset+ND_ALERT_PAGE<page.total)page.offset+=ND_ALERT_PAGE;}
   else if(action==102){page.operation=1;page.through=through;nd_control_alerts(&page);page.operation=0;}
   else if(action==103){page.operation=2;nd_control_alerts(&page);page.operation=0;page.offset=0;}
  }
  elapsed=nd_client_wait_signals(&client,panel ? 50:250,nd_panel_signals(panel));
 }
 nd_panel_close(panel);nd_client_close(&client);return 0;
}
