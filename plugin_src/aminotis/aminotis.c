/* AMINOTIS: live AmiNotis inbox tile.
 *
 * Reads the separately installed AmiNotis service (AMINOTIS.SERVICE) with
 * its v2 SDK. One client keeps an AN_WATCH parked so the tile follows the
 * inbox without polling; a second client carries LIST and user commands.
 * Both reply ports join the plugin's wait, so the dock loop never blocks
 * on the service. When the service is absent the plugin checks for its
 * port every five seconds and shows the offline artwork. */
#include <exec/types.h>
#include <proto/exec.h>
#include <stdio.h>
#include <string.h>
#include "nd_client.h"
#include "nd_panel.h"
#include "nd_option_runtime.h"
#include "nd_plugin_versions.h"
#include "sdk/aminotis.h"

#ifndef ND_PLUGIN_VERSION_AMINOTIS
#define ND_PLUGIN_VERSION_AMINOTIS "0.1.0"
#define ND_PLUGIN_DATE_AMINOTIS "26.11.2026"
#endif
const char nd_version[]=ND_PLUGIN_VER_STRING(AMINOTIS,"AmiNotis");
long __stack=32768;

/* Artwork states; tools/generate_aminotis_art.py draws the matching bank. */
#define ART_IDLE 0U
#define ART_UNREAD 1U
#define ART_QUIET 2U
#define ART_OFFLINE 3U /* sad face: service missing or too old */

#define RETRY_TICKS 250U       /* service discovery while offline */
#define COMMAND_TICKS 150U     /* bound on one LIST or command reply */
#define WATCH_INTEREST (AN_CHANGE_HISTORY|AN_CHANGE_UNREAD|AN_CHANGE_DND)
#define NEEDED_CAPS (AN_CAP_HISTORY|AN_CAP_WATCH)

enum Command {CMD_NONE,CMD_LIST,CMD_READ_ALL,CMD_QUIET,CMD_UNQUIET,CMD_OPEN};
enum Link {LINK_OFFLINE,LINK_ONLINE,LINK_OLD};

static struct ANClient watcher,commander;
static struct ANRequest request;
static struct ANResponse response;
static struct ANStatus view,watch_cursor;
static struct ANEvent page[AN_PAGE];
static unsigned int page_count,page_offset,page_more;
static enum Link link_state=LINK_OFFLINE;
static enum Command running=CMD_NONE,queued=CMD_NONE;
static int need_list=1,dirty=1;
static unsigned int command_age,retry_age=RETRY_TICKS,watch_hold;

static int service_present(void)
{int present;Forbid();present=FindPort(AN_PORT)!=NULL;Permit();return present;}
static unsigned long client_signal(const struct ANClient *c)
{return c->reply ? 1UL<<c->reply->mp_SigBit:0UL;}

/* The service replies to every parked watch when it stops normally. If its
 * port vanished without a reply, the message may sit on a freed port, so
 * it is abandoned rather than unlinked from memory we no longer own. */
static void abandon(struct ANClient *c)
{
 if(c->pending&&!service_present()){c->pending=NULL;return;}
 an_close(c);
}
static void go_offline(void)
{
 abandon(&watcher);abandon(&commander);
 link_state=LINK_OFFLINE;running=CMD_NONE;need_list=1;retry_age=0;page_count=page_more=0;dirty=1;
 memset(&view,0,sizeof(view));memset(&watch_cursor,0,sizeof(watch_cursor));
}
static unsigned long shown_count(int use_unread)
{return use_unread ? view.unread:view.badge;}

static void arm_watch(void)
{
 if(watcher.pending||link_state!=LINK_ONLINE||watch_hold)return;
 if(!(view.capabilities&AN_CAP_WATCH))return;
 an_request(&request,AN_WATCH);
 request.value=(view.capabilities&AN_CAP_CHANGE_MASK) ? WATCH_INTEREST:0;
 an_use_cursor(&request,&watch_cursor);
 if(an_submit(&watcher,&request)==AN_UNAVAILABLE)go_offline();
 /* AN_BUSY (queue full or no memory) retries on the next pass. */
}

static void start_command(void)
{
 enum Command next;int result;
 if(commander.pending)return;
 if(link_state!=LINK_ONLINE){
  if(retry_age<RETRY_TICKS)return;
  retry_age=0;if(!service_present())return;need_list=1;
 }
 next=queued!=CMD_NONE&&link_state==LINK_ONLINE ? queued:(need_list ? CMD_LIST:CMD_NONE);
 if(next==CMD_NONE)return;
 switch(next){
  case CMD_LIST:an_request(&request,AN_LIST);request.offset=page_offset;request.limit=AN_PAGE;break;
  case CMD_READ_ALL:an_request(&request,AN_READ_ALL);an_use_cursor(&request,&view);break;
  case CMD_QUIET:an_request(&request,AN_SET_DND);request.value=3600UL;break;
  case CMD_UNQUIET:an_request(&request,AN_SET_DND);request.value=0;break;
  default:an_request(&request,AN_OPEN_HISTORY);break;
 }
 result=an_submit(&commander,&request);
 if(result==AN_OK){running=next;command_age=0;if(next==queued)queued=CMD_NONE;else need_list=0;}
 else if(result==AN_UNAVAILABLE)go_offline();
}

static void finish_command(void)
{
 enum Command done=running;running=CMD_NONE;
 if(done==CMD_LIST){
  if(response.result==AN_STALE){need_list=1;return;}
  if(response.result!=AN_OK||response.count>AN_PAGE){go_offline();return;}
  if((response.status.capabilities&NEEDED_CAPS)!=NEEDED_CAPS){link_state=LINK_OLD;view=response.status;retry_age=0;dirty=1;return;}
  /* A page past the end (entries were removed) steps back to the start. */
  if(!response.count&&page_offset){page_offset=0;need_list=1;}
  view=response.status;link_state=LINK_ONLINE;
  page_count=response.count;page_more=response.more!=0;
  memcpy(page,response.page.events,page_count*sizeof(page[0]));dirty=1;
  if(!watcher.pending)watch_cursor=view;
  return;
 }
 if(response.result==AN_UNAVAILABLE){go_offline();return;}
 /* Commands change the inbox; the parked watch also reports it, but a
  * service without change masks may not, so refresh either way. */
 need_list=1;
}

static void finish_watch(void)
{
 int refresh;
 if(response.result==AN_UNAVAILABLE||response.result==AN_VERSION_ERROR){go_offline();return;}
 refresh=an_watch_resync(&watch_cursor,&response)||
         !(response.status.capabilities&AN_CAP_CHANGE_MASK)||(response.id&WATCH_INTEREST);
 /* A refused watch waits before rearming so a persistent error cannot spin. */
 if(response.result==AN_OK)watch_cursor=response.status;else{memset(&watch_cursor,0,sizeof(watch_cursor));watch_hold=RETRY_TICKS;}
 if(refresh)need_list=1;
}

static void service_replies(void)
{
 if(watcher.pending&&an_receive(&watcher,&response))finish_watch();
 if(commander.pending&&an_receive(&commander,&response))finish_command();
}

static void clock_text(char *out,ANNumber stamp)
{ANNumber day=stamp%86400UL;sprintf(out,"%02lu:%02lu",(unsigned long)(day/3600UL),(unsigned long)((day/60UL)%60UL));}

static void tile_text(char *value,unsigned int *art,int use_unread)
{
 char when[8];unsigned long n=shown_count(use_unread);
 if(link_state==LINK_OFFLINE){strcpy(value,"AmiNotis not running");*art=ART_OFFLINE;return;}
 if(link_state==LINK_OLD){strcpy(value,"AmiNotis update needed");*art=ART_OFFLINE;return;}
 if(view.dnd){
  *art=ART_QUIET;
  if(view.dnd_until){clock_text(when,view.dnd_until);sprintf(value,"Quiet until %s",when);}
  else strcpy(value,"Quiet");
  if(n){char tail[16];sprintf(tail,", %lu new",n);if(strlen(value)+strlen(tail)<ND_TEXT_LEN)strcat(value,tail);}
  return;
 }
 if(n){sprintf(value,"%lu unread",n);*art=ART_UNREAD;}
 else{strcpy(value,"No new notifications");*art=ART_IDLE;}
}

#define ROWS (AN_PAGE+1)
static const char *button_labels[]={"Newer","Older","Read all","Quiet 1h","Inbox"};
static struct NDPanel *open_panel(void)
{
 struct NDPanelField fields[ROWS];unsigned int i;
 /* GadTools fixes each label's position from its text at creation, so reserve
  * the widest row label ("*HH:MM " plus 15 name characters). An empty label
  * would place later text over the value column. */
 static const char reserve[]="                      ";
 for(i=0;i<ROWS;++i){fields[i].label=reserve;fields[i].value="";fields[i].limit=0;}
 return nd_panel_open("AmiNotis",fields,ROWS,button_labels,5);
}
static void fill_panel(struct NDPanel *panel)
{
 char label[40],line[160],when[8];unsigned int i,pages,current;
 int online=link_state==LINK_ONLINE;
 if(!online){
  nd_panel_set_label(panel,0,"Service");
  nd_panel_set(panel,0,link_state==LINK_OLD ? "This AmiNotis is too old for the dock inbox.":"AmiNotis is not running.");
  for(i=1;i<ROWS;++i){nd_panel_set_label(panel,i,"");nd_panel_set(panel,i,"");}
 }else{
  pages=view.count ? (unsigned int)((view.count+AN_PAGE-1)/AN_PAGE):1U;current=page_offset/AN_PAGE+1;if(current>pages)pages=current;
  sprintf(line,"%lu unread of %lu, page %u/%u%s",(unsigned long)view.unread,(unsigned long)view.count,current,pages,view.dnd ? ", quiet":"");
  nd_panel_set_label(panel,0,"Inbox");nd_panel_set(panel,0,line);
  for(i=0;i<AN_PAGE;++i){
   if(i<page_count){
    const struct ANEvent *e=&page[i];
    clock_text(when,e->created);
    sprintf(label,"%s%s %.15s",e->unread ? "*":"",when,e->name[0] ? e->name:e->app);
    if(e->body[0])sprintf(line,"%.80s - %.70s",e->title,e->body);else sprintf(line,"%.80s",e->title);
   }else{label[0]=0;strcpy(line,i==0&&!page_offset ? "No notifications.":"");}
   nd_panel_set_label(panel,i+1,label);nd_panel_set(panel,i+1,line);
  }
 }
 nd_panel_button(panel,0,button_labels[0],online&&page_offset>0);
 nd_panel_button(panel,1,button_labels[1],online&&page_more);
 nd_panel_button(panel,2,button_labels[2],online&&view.unread>0);
 nd_panel_button(panel,3,online&&view.dnd ? "Quiet off":"Quiet 1h",online&&(view.capabilities&AN_CAP_DND));
 nd_panel_button(panel,4,button_labels[4],online&&(view.capabilities&AN_CAP_OPEN_HISTORY));
}

static void read_options(const char *id,int *click_inbox,int *beep,int *use_unread)
{
 char click[12],sound[8],count[12];
 struct NDOptionString options[3];
 options[0].key="CLICK";options[0].fallback="PANEL";options[0].out=click;options[0].limit=sizeof(click);
 options[1].key="BEEP";options[1].fallback="NO";options[1].out=sound;options[1].limit=sizeof(sound);
 options[2].key="COUNT";options[2].fallback="BADGE";options[2].out=count;options[2].limit=sizeof(count);
 nd_plugin_option_strings(id,options,3);
 *click_inbox=!strcmp(click,"INBOX");*beep=!strcmp(sound,"YES");*use_unread=!strcmp(count,"UNREAD");
}

int main(int argc,char **argv)
{
 struct NDClient client;struct NDPanel *panel=NULL;
 unsigned int elapsed=0,event,art=ART_OFFLINE;int action,click_inbox=0,beep=0,use_unread=0,synced=0;
 unsigned long previous=0,n;char value[ND_TEXT_LEN],badge[ND_BADGE_LEN];
 ANNumber shown_revision=0;enum Link shown_link=LINK_OFFLINE;
 if(argc<2||!nd_client_init(&client,argv[1]))return 20;
 read_options(argv[1],&click_inbox,&beep,&use_unread);
 for(;;){
  service_replies();
  retry_age+=elapsed;watch_hold=watch_hold>elapsed ? watch_hold-elapsed:0;
  if(commander.pending){command_age+=elapsed;if(command_age>=COMMAND_TICKS)go_offline();}
  if(watcher.pending&&retry_age>=RETRY_TICKS){retry_age=0;if(!service_present())go_offline();}
  start_command();arm_watch();
  if(view.revision!=shown_revision||link_state!=shown_link){shown_revision=view.revision;shown_link=link_state;dirty=1;}

  n=link_state==LINK_ONLINE ? shown_count(use_unread):0;
  if(link_state==LINK_ONLINE){if(synced&&beep&&n>previous)nd_panel_beep(panel);previous=n;synced=1;}else synced=0;
  tile_text(value,&art,use_unread);
  if(n>9999UL)strcpy(badge,"9999+");else if(n)sprintf(badge,"%lu",n);else badge[0]=0;
  event=nd_client_set_badge(&client,badge);
  event|=nd_client_update_art_quiet(&client,"AMINOTIS",value,"aminotis",art,elapsed,250);
  if(event&ND_EVENT_QUIT)break;
  if(event&ND_EVENT_SETTINGS)read_options(argv[1],&click_inbox,&beep,&use_unread);
  if(event&ND_EVENT_CLICK){
   if(click_inbox&&link_state==LINK_ONLINE)queued=CMD_OPEN;
   else if(panel)nd_panel_raise(panel);
   else{panel=open_panel();page_offset=0;need_list=1;dirty=1;}
  }
  if(panel&&dirty){fill_panel(panel);dirty=0;}
  while(panel&&(action=nd_panel_event(panel))!=0){
   if(action==-1){nd_panel_close(panel);panel=NULL;if(page_offset){page_offset=0;need_list=1;}}
   else if(action==100&&page_offset>=AN_PAGE){page_offset-=AN_PAGE;need_list=1;}
   else if(action==101&&page_more){page_offset+=AN_PAGE;need_list=1;}
   else if(action==102)queued=CMD_READ_ALL;
   else if(action==103)queued=view.dnd ? CMD_UNQUIET:CMD_QUIET;
   else if(action==104)queued=CMD_OPEN;
  }
  start_command();
  elapsed=nd_client_wait_signals(&client,panel ? 50:250,
   nd_panel_signals(panel)|(watcher.pending ? client_signal(&watcher):0)|(commander.pending ? client_signal(&commander):0));
 }
 nd_panel_close(panel);
 abandon(&watcher);abandon(&commander);
 an_close(&watcher);an_close(&commander);
 nd_client_close(&client);
 return 0;
}
