/* Disposable Workbench integration: real dock, journal, Shelf and absent I2C. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "nd_client.h"
#include "nd_control_client.h"
#include "nd_list.h"
#define main core_checks
#include "test_collections_alerts_volume.c"
#undef main
long __stack=65536;
static FILE *report;
static int run(const char *command,int async)
{BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc;if(!in || !out)return 0;rc=SystemTags((STRPTR)command,SYS_Input,in,SYS_Output,out,SYS_Asynch,async,NP_StackSize,65536UL,TAG_DONE);if(rc==-1){Close(in);Close(out);}return async ? rc!=-1:rc==0;}
static int dock_state(int online)
{int n,live;for(n=0;n<500;n+=5){Forbid();live=(FindPort(ND_CONTROL_PORT)!=NULL) && (FindPort(ND_PORT_NAME)!=NULL);if(!online)live=(FindPort(ND_CONTROL_PORT)!=NULL) || (FindPort(ND_PORT_NAME)!=NULL);Permit();if(live==online)return 1;Delay(5);}return 0;}
static int log_check(int ok,const char *name)
{fprintf(report,"%s: %s\n",ok ? "PASS":"FAIL",name);fflush(report);return ok;}
int main(void)
{
 BPTR marker,lock;FILE *f;struct NDClient client;struct NDControlAlertsMessage page;struct NDControlPluginHealthMessage health;struct NDShelf saved;struct NDList *list;const char *buttons[]={"Open","Refresh","Close"};const char *rows[]={"First","Second","Third"};int i,result=10;
 marker=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!marker)return 20;UnLock(marker);report=fopen("QATests:feature-results.txt","w");if(!report)return 20;
 Delay(250);nd_control_send(ND_CONTROL_QUIT);if(!log_check(dock_state(0),"bundled dock stopped only inside disposable guest"))goto done;
 if(!log_check(core_checks()==0,"32-bit volume math, journal bounds, shelf backups and stable sensor settings"))goto done;
 list=nd_list_open("Collection UI check",buttons,3);if(!log_check(list!=NULL,"native filtered list window opens"))goto done;nd_list_rows(list,rows,3);Delay(25);nd_list_close(list);
 if(!log_check(run("QATests:SensorsI2C",0),"isolated sensor helper returns without installed I2C library"))goto done;
 if(!log_check(nd_i2c_load(&sensors) && !sensors.provider,"absent provider publishes unavailable status, not a zero reading"))goto done;
 lock=CreateDir("ENV:NovaDock");if(lock)UnLock(lock);lock=CreateDir("ENVARC:NovaDock");if(lock)UnLock(lock);
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f)goto done;
 fputs("PLUGIN=ALERTS|QATests:Alerts|ALERTS|METER|ON|NO\nPLUGIN=SHELF|QATests:Shelf|SHELF|METER|ON|YES\nPLUGIN=Probe|C:True|QANOTICE|METER|ON|NO\nOPTION.SHELF.DRAWER=QATests:Storage\nOPTION.ALERTS.SOUND=NO\n",f);fclose(f);
 f=fopen("ENVARC:NovaDock/novadock.cfg","w");if(!f)goto done;fputs("PLUGIN=ALERTS|QATests:Alerts|ALERTS|METER|ON|NO\nPLUGIN=SHELF|QATests:Shelf|SHELF|METER|ON|YES\nPLUGIN=Probe|C:True|QANOTICE|METER|ON|NO\nOPTION.SHELF.DRAWER=QATests:Storage\nOPTION.ALERTS.SOUND=NO\n",f);fclose(f);
 if(!run("QATests:NovaDock",1) || !dock_state(1))goto done;
 if(!nd_client_init(&client,"QANOTICE"))goto stop;
 {unsigned int response=nd_client_update(&client,"QA","Ready",ND_NO_METER);fprintf(report,"Diagnostic: update=%u hostABI=%u alerts=%u\n",response,client.host_abi,client.host_alerts);fflush(report);}
 if(!log_check(client.host_alerts,"host advertises gated notification capability")){nd_client_close(&client);goto stop;}
 nd_client_update_art_quiet(&client,"TREE","off","ChristmasLights",0,50,500);
 nd_client_set_badge(&client,"*");
 if(!log_check(client.host_artwork && client.host_artwork_meta && client.host_alerts,"badge reply preserves negotiated host capabilities")){nd_client_close(&client);goto stop;}
 nd_client_update_art_quiet(&client,"TREE","on","ChristmasLights",1,50,500);
 if(!log_check(client.art_mode==2 && client.packet.ext.reserved==1 && (client.packet.ext.flags&ND_EXT_HAS_ARTWORK),"confirmed on state sends illuminated artwork after badge exchange")){nd_client_close(&client);goto stop;}
 nd_client_notify(&client,"QA completed","Native notification",0);
 if(!log_check(client.host_artwork && client.host_artwork_meta,"notification reply retains artwork capabilities")){nd_client_close(&client);goto stop;}
 nd_client_set_badge(&client,"");nd_client_update_art_quiet(&client,"TREE","off","ChristmasLights",0,50,500);
 if(!log_check(client.art_mode==2 && client.packet.ext.reserved==0 && (client.packet.ext.flags&ND_EXT_HAS_ARTWORK),"confirmed off state sends unlit artwork after badge removal")){nd_client_close(&client);goto stop;}
 memset(&page,0,sizeof(page));
 if(!log_check(nd_control_alerts(&page)==ND_CONTROL_OK && page.unread==1 && page.count==1 && !strcmp(page.entries[0].title,"QA completed"),"notice reaches actual dock journal")){nd_client_close(&client);goto stop;}
 page.operation=1;if(!log_check(nd_control_alerts(&page)==ND_CONTROL_OK && !page.unread,"acknowledgement clears unread count")){nd_client_close(&client);goto stop;}
 nd_client_close(&client);
 for(i=0;i<250;i+=5){Forbid();lock=FindPort("NovaDock.Shelf.SHELF")!=NULL;Permit();if(lock)break;Delay(5);}if(!lock)goto stop;
 f=fopen("QATests:Original","w");if(!f)goto stop;fputs("Never delete me",f);fclose(f);
 if(!run("QATests:Shelf SHELF PUT QATests:Original",0))goto stop;
 lock=CreateDir("T:NovaDockShelfDrop-native");if(lock)UnLock(lock);f=fopen("T:NovaDockShelfDrop-native/items","w");if(!f)goto stop;fputs("QATests:Original\nRAM:\nSYS:Prefs\n",f);fclose(f);
 if(!log_check(run("QATests:Shelf SHELF HANDLELIST T:NovaDockShelfDrop-native/items",0),"multi-item drop handled by one resident Shelf"))goto stop;
 f=fopen("QATests:Storage/SHELF.shelf","r");if(!f)goto stop;i=nd_shelf_read(f,&saved);fclose(f);if(!log_check(i && saved.count==3,"three shortcuts saved, duplicate drop ignored"))goto stop;
 lock=Lock("QATests:Original",ACCESS_READ);if(!log_check(lock!=0,"drop leaves original file intact"))goto stop;if(lock)UnLock(lock);
 nd_control_send(ND_CONTROL_QUIT);if(!dock_state(0))goto done;Delay(50);
 if(!run("QATests:NovaDock",1) || !dock_state(1))goto done;
 for(i=0;i<500;i+=25){memset(&health,0,sizeof(health));if(nd_control_plugin_health("SHELF",&health)==ND_CONTROL_OK && health.online && !strcmp(health.value,"3 shortcuts"))break;Delay(25);}
 if(!log_check(i<500,"Shelf restores collection after dock restart"))goto stop;
 memset(&page,0,sizeof(page));if(!log_check(nd_control_alerts(&page)==ND_CONTROL_OK && !page.total,"notification history resets on dock restart"))goto stop;
 result=0;
stop:nd_control_send(ND_CONTROL_QUIT);dock_state(0);
done:fclose(report);return result;
}
