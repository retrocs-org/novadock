/* Isolated Workbench check of the ARexx host: per-target ARexx launcher
 * limits with a hung target, GET/SET/THEME/INFO, Edit-mode UNDO and
 * STATUS DETAIL.  Run only with the QATests:ISOLATED-QA-ONLY marker; it
 * redirects ENV:/ENVARC: to private drawers and never touches user settings.
 * Expects the packaged dock at QATests:NovaDock/NovaDock. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <stdio.h>
#include <string.h>
long __stack=65536;
struct RxsLib *RexxSysBase;
static struct MsgPort *hung,*live,*reply;
static int failures;

static void check(int ok,const char *what)
{
    printf("%s: %s\n",ok ? "PASS":"FAIL",what);
    if(!ok)++failures;
}
static struct MsgPort *public_port(const char *name)
{
    struct MsgPort *p=CreateMsgPort();
    if(!p)return NULL;
    p->mp_Node.ln_Name=(char *)name;p->mp_Node.ln_Pri=0;
    AddPort(p);
    return p;
}
static void drop_port(struct MsgPort *p)
{
    struct Message *m;
    if(!p)return;
    RemPort(p);
    while((m=GetMsg(p))!=NULL)ReplyMsg(m);
    DeleteMsgPort(p);
}
static int count_waiting(struct MsgPort *p)
{
    struct Node *n;int count=0;
    Forbid();
    for(n=p->mp_MsgList.lh_Head;n->ln_Succ;n=n->ln_Succ)++count;
    Permit();
    return count;
}
/* Exercise the real interpreter-to-host protocol. CreateRexxMsg() alone
 * makes an outbound command packet, not an interpreter context accepted by
 * IsRexxMsg() on every ARexx version. The disposable runner bounds the run. */
static LONG rexx(const char *command,char *result,unsigned int size)
{
    FILE *f;long rc=-1;
    if(result&&size)result[0]=0;
    f=fopen("QATests:command.rexx","w");if(!f)return -1;
    fprintf(f,"/* isolated NovaDock control test */\noptions results\naddress NOVADOCK '%s'\nn=rc\nanswer=''\nif symbol('RESULT')='VAR' then answer=result\nf=open('qa','QATests:rexx-result','W')\ncall writeln 'qa',n\ncall writeln 'qa',answer\ncall close 'qa'\n",command);
    fclose(f);
    if(SystemTags("SYS:Rexxc/RX QATests:command.rexx",TAG_DONE)!=0)return -1;
    f=fopen("QATests:rexx-result","r");if(!f)return -1;
    if(fscanf(f,"%ld",&rc)==1){
        int c;while((c=fgetc(f))!='\n'&&c!=EOF);
        if(result&&size&&fgets(result,(int)size,f))result[strcspn(result,"\r\n")]=0;
    }
    fclose(f);printf("ARexx %s -> RC %ld %s\n",command,rc,result?result:"");
    return rc;
}
static int dock_running(void){int up;Forbid();up=FindPort((STRPTR)"NOVADOCK")!=NULL;Permit();return up;}
static int write_config(void)
{
    FILE *f=fopen("ENV:NovaDock/novadock.cfg","w");
    if(!f)return 0;
    fputs("# NovaDock QA - ARexx control\nSIDE=BOTTOM\nTOOLTIPS=NO\nAUTO_HIDE=NO\n",f);
    fputs("LAUNCH=Hung|QAHUNG||ON|AREXX|PING|||NO|NO||||10|NO\n",f);
    fputs("LAUNCH=Live|QALIVE||ON|AREXX|PING|||NO|NO||||10|NO\n",f);
    fputs("LAUNCH=Shell|SYS:System/Shell||ON|AUTO||||NO|NO||||10|NO\n",f);
    return fclose(f)==0;
}
int main(void)
{
    char text[600];BPTR marker;int i;LONG rc;struct Message *m;
    marker=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!marker)return 10;UnLock(marker);
    RexxSysBase=(struct RxsLib *)OpenLibrary("rexxsyslib.library",36);if(!RexxSysBase)return 10;
    reply=CreateMsgPort();hung=public_port("QAHUNG");live=public_port("QALIVE");
    if(!reply||!hung||!live)goto done;
    SystemTags("QATests:NovaDock/NovaDock QUIT",TAG_DONE);Delay(100);
    SystemTags("MakeDir RAM:RexxEnv RAM:RexxEnv/NovaDock RAM:RexxEnvArc",TAG_DONE);
    SystemTags("Assign ENV: RAM:RexxEnv",TAG_DONE);SystemTags("Assign ENVARC: RAM:RexxEnvArc",TAG_DONE);
    SystemTags("Assign NovaDock: QATests:NovaDock",TAG_DONE);
    if(!write_config())goto done;
    {BPTR in=Open("NIL:",MODE_OLDFILE),out=Open("NIL:",MODE_NEWFILE);
     if(!in||!out||SystemTags("QATests:NovaDock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0)goto done;}
    for(i=0;i<300&&!dock_running();++i)Delay(5);
    check(dock_running(),"dock started with its ARexx host");
    if(!dock_running())goto done;
    Delay(100);

    /* A hung target holds at most two requests; other targets still work. */
    for(i=0;i<4;++i)(void)rexx("OPEN 1",text,sizeof(text));
    Delay(25);
    check(count_waiting(hung)==2,"a target that never answers receives at most two actions");
    (void)rexx("OPEN 2",text,sizeof(text));
    for(i=0;i<100&&!count_waiting(live);++i)Delay(2);
    check(count_waiting(live)==1,"a hung target does not block actions for another target");
    while((m=GetMsg(live))!=NULL)ReplyMsg(m);
    /* An answer frees the slot again. */
    if((m=GetMsg(hung))!=NULL)ReplyMsg(m);
    Delay(25);(void)rexx("OPEN 1",text,sizeof(text));Delay(25);
    check(count_waiting(hung)==2,"an answered request frees its slot for the same target");

    /* Settings. */
    check(rexx("GET SIDE",text,sizeof(text))==0&&!strcmp(text,"BOTTOM"),"GET SIDE returns BOTTOM");
    check(rexx("SET side top",text,sizeof(text))==0&&!strcmp(text,"TOP"),"SET SIDE TOP applies and returns TOP");
    Delay(50);
    check(rexx("GET SIDE",text,sizeof(text))==0&&!strcmp(text,"TOP"),"GET SIDE follows SET");
    check(rexx("SET SIDE SIDEWAYS",text,sizeof(text))==10,"an invalid value is refused with RC 10");
    check(rexx("THEME NOSUCHTHEME",text,sizeof(text))==10,"an unknown theme is refused");
    check(rexx("GET NOSUCHKEY",text,sizeof(text))==5,"an unknown key returns RC 5");
    rc=rexx("SAVE",text,sizeof(text));
    check(rc==0&&(!strcmp(text,"OK")||!strcmp(text,"QUEUED")),"SAVE answers OK or QUEUED");

    /* Profiles and the new appearance settings use private ENV:/ENVARC:. */
    Delay(100);
    check(rexx("PROFILE SAVE QA",text,sizeof(text))==0,"PROFILE SAVE in private ENVARC");
    Delay(100);
    check(rexx("PROFILE LIST",text,sizeof(text))==0&&strstr(text,"QA"),"PROFILE LIST includes saved profile");
    check(rexx("SET SIDE BOTTOM",text,sizeof(text))==0,"change SIDE before profile restore");
    Delay(100);
    check(rexx("PROFILE USE QA",text,sizeof(text))==0,"PROFILE USE restores saved configuration");
    Delay(100);
    check(rexx("GET SIDE",text,sizeof(text))==0&&!strcmp(text,"TOP"),"profile restore includes SIDE");
    check(rexx("PROFILE DELETE QA",text,sizeof(text))==0,"PROFILE DELETE removes private profile");
    check(rexx("SET HOVER_ANIMATION YES",text,sizeof(text))==0,"animated magnification setting applies");
    Delay(100);
    check(rexx("GET HOVER_ANIMATION",text,sizeof(text))==0&&!strcmp(text,"YES"),"animated magnification setting round-trips");
    check(rexx("SET DOCK_BACKGROUND WALLPAPER",text,sizeof(text))==0,"wallpaper background setting applies");
    Delay(100);
    check(rexx("GET DOCK_BACKGROUND",text,sizeof(text))==0&&!strcmp(text,"WALLPAPER"),"wallpaper background setting round-trips");

    /* Tile details. */
    check(rexx("INFO 1",text,sizeof(text))==0&&!strncmp(text,"LAUNCHER|Hung|QAHUNG|",21),"INFO 1 describes the first launcher");
    check(rexx("INFO 9",text,sizeof(text))==5,"INFO beyond the last tile returns RC 5");

    /* Edit-mode undo. */
    check(rexx("UNDO",text,sizeof(text))==5,"UNDO with nothing to undo returns RC 5");
    (void)rexx("UNLOCK",text,sizeof(text));
    check(rexx("REMOVE 3",text,sizeof(text))==0,"REMOVE 3 in Edit mode");
    Delay(50);
    check(rexx("INFO 3",text,sizeof(text))==5,"the removed tile is gone");
    check(rexx("UNDO",text,sizeof(text))==0,"UNDO restores the removal");
    Delay(50);
    check(rexx("INFO 3",text,sizeof(text))==0&&!strncmp(text,"LAUNCHER|Shell|",15),"the restored tile is back in place");
    /* Any other settings change ends the undo step (it would be reverted). */
    (void)rexx("REMOVE 3",text,sizeof(text));Delay(50);
    check(rexx("SET tooltips yes",text,sizeof(text))==0,"SET in Edit mode");
    Delay(50);
    check(rexx("UNDO",text,sizeof(text))==5,"a later settings change ends the undo step");
    check(rexx("GET option.nosuch.key",text,sizeof(text))==5,"an unknown plugin option returns RC 5");
    (void)rexx("LOCK",text,sizeof(text));
    check(rexx("UNDO",text,sizeof(text))==5,"locking the dock ends the undo step");

    /* Diagnostics. */
    check(rexx("STATUS DETAIL",text,sizeof(text))==0&&strstr(text,"HOVER_PARTIAL=")&&strstr(text,"TIMER_HZ="),"STATUS DETAIL reports paint and timer counters");
    printf("%s\n",text);

done:
    /* Answer the held requests so the dock can free them, then stop it. */
    if(hung)while((m=GetMsg(hung))!=NULL)ReplyMsg(m);
    if(dock_running())(void)rexx("QUIT",text,sizeof(text));
    for(i=0;i<200&&dock_running();++i)Delay(5);
    drop_port(hung);drop_port(live);
    if(reply)DeleteMsgPort(reply);
    if(RexxSysBase)CloseLibrary((struct Library *)RexxSysBase);
    puts(failures ? "FAIL: ARexx control checks failed." : "PASS: ARexx control checks complete.");
    return failures ? 10 : 0;
}
