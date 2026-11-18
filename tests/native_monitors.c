/* Run only on a cloned Workbench guest with a QATests marker. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_protocol.h"
#include "nd_control_client.h"
#include "nd_list.h"
long __stack=65536;
static FILE *report;
static int run(const char *command)
{BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);LONG rc;if(!in || !out)return 0;rc=SystemTags((STRPTR)command,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);if(rc==-1){Close(in);Close(out);}return rc!=-1;}
static int dock_state(int online)
{int n,live;for(n=0;n<500;n+=5){Forbid();live=FindPort(ND_CONTROL_PORT)!=NULL;Permit();if(live==online)return 1;Delay(5);}return 0;}
static int check(int ok,const char *name)
{fprintf(report,"%s: %s\n",ok ? "PASS":"FAIL",name);fflush(report);return ok;}
static int value(const char *id,const char *text)
{int n;struct NDControlPluginHealthMessage h;for(n=0;n<1000;n+=10){memset(&h,0,sizeof(h));if(nd_control_plugin_health(id,&h)==ND_CONTROL_OK && h.online && strstr(h.value,text))return 1;Delay(10);}fprintf(report,"Last %s value: %s\n",id,h.value);return 0;}
static int write_file(const char *path,const char *mode,const char *text)
{FILE *f;int ok;if(!strcmp(mode,"a")){BPTR file=Open((STRPTR)path,MODE_READWRITE);LONG n;if(!file)return check(0,"fixture append open");if(Seek(file,0,OFFSET_END)==-1){Close(file);return check(0,"fixture append seek");}n=Write(file,(APTR)text,strlen(text));Close(file);return check(n==(LONG)strlen(text),"fixture append write");}f=fopen(path,mode);if(!f)return check(0,"fixture write open");ok=fputs(text,f)>=0;if(fclose(f))ok=0;return check(ok,"fixture write");}
int main(void)
{
 BPTR l;int result=10;struct NDList *list;struct NDControlAlertsMessage alerts;const char *buttons[]={"Copy line","Open file","Mark read","Clear history","Check now","Close"};const char *rows[]={"[WARN] Native worker test","[ERROR] bounded viewer"};
 l=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!l)return 20;UnLock(l);report=fopen("QATests:monitor-results.txt","w");if(!report)return 20;
 Delay(250);nd_control_send(ND_CONTROL_QUIT);if(!check(dock_state(0),"stop dock inside disposable guest"))goto done;
 list=nd_list_open("WATCH and LOGS native viewer",buttons,6);if(!check(list!=NULL,"native six-button filtered viewer opens"))goto done;nd_list_rows(list,rows,2);Delay(25);nd_list_close(list);
 l=CreateDir("QATests:Watched");if(l)UnLock(l);l=CreateDir("ENV:NovaDock");if(l)UnLock(l);l=CreateDir("ENVARC:NovaDock");if(l)UnLock(l);
 if(!write_file("QATests:Watched/initial","w","baseline") || !write_file("QATests:follow.log","w","normal\n"))goto done;
 {const char *cfg="PLUGIN=WATCH|QATests:Watch|WATCH|METER|ON|YES\nPLUGIN=LOGS|QATests:Logs|LOGS|METER|ON|YES\nOPTION.WATCH.DRAWER=QATests:Watched\nOPTION.WATCH.INTERVAL=2\nOPTION.LOGS.FILE=QATests:follow.log\nOPTION.LOGS.INTERVAL=2\n";
  if(!write_file("ENV:NovaDock/novadock.cfg","w",cfg) || !write_file("ENVARC:NovaDock/novadock.cfg","w",cfg))goto done;}
 if(!run("QATests:NovaDock") || !dock_state(1))goto done;
 if(!check(value("WATCH","0 unread"),"WATCH native worker establishes baseline without flooding"))goto stop;
 if(!check(value("LOGS","0 unread; 1 recent"),"LOGS native worker reads initial normal line"))goto stop;
 if(!write_file("QATests:Watched/added","w","new") || !write_file("QATests:follow.log","a","WARNING native append\n"))goto stop;
 if(!check(value("WATCH","1 unread; 1 recent"),"WATCH reports a new file through dock IPC"))goto stop;
 if(!check(value("LOGS","1 unread; 2 recent"),"LOGS reports appended warning through dock IPC"))goto stop;
 if(!write_file("QATests:Watched/added","a"," changed") || !write_file("QATests:follow.log","w","ERROR truncated\n"))goto stop;
 if(!check(value("WATCH","2 unread; 2 recent"),"WATCH detects changed size/date"))goto stop;
 if(!check(value("LOGS","2 unread; 4 recent"),"LOGS detects truncation and adds marker"))goto stop;
 if(!Rename("QATests:follow.log","QATests:old.log") || !write_file("QATests:follow.log","w","ERROR replacement\n"))goto stop;
 if(!check(value("LOGS","3 unread; 6 recent"),"LOGS follows a replaced file"))goto stop;
 memset(&alerts,0,sizeof(alerts));if(!check(nd_control_alerts(&alerts)==ND_CONTROL_OK && alerts.total>=2,"worker batches reach dock ALERTS history"))goto stop;
 result=0;
stop:nd_control_send(ND_CONTROL_QUIT);dock_state(0);
done:fclose(report);return result;
}
