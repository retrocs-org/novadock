#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_control_client.h"

static struct NDConfig barrier_config;
static LONG barrier_rexx(const char *command,char *result,unsigned int size)
{
    FILE *f;long rc=-1;
    if(result&&size)result[0]=0;
    f=fopen("QATests:barrier-command.rexx","w");if(!f)return -1;
    fprintf(f,"/* isolated NovaDock control test */\noptions results\naddress NOVADOCK '%s'\nn=rc\nanswer=''\nif symbol('RESULT')='VAR' then answer=result\nf=open('qa','QATests:barrier-result','W')\ncall writeln 'qa',n\ncall writeln 'qa',answer\ncall close 'qa'\n",command);
    fclose(f);
    if(SystemTags("SYS:Rexxc/RX QATests:barrier-command.rexx",TAG_DONE)!=0)return -1;
    f=fopen("QATests:barrier-result","r");if(!f)return -1;
    if(fscanf(f,"%ld",&rc)==1){
        int c;while((c=fgetc(f))!='\n'&&c!=EOF);
        if(result&&size&&fgets(result,(int)size,f))result[strcspn(result,"\r\n")]=0;
    }
    fclose(f);printf("ARexx %s -> RC %ld %s\n",command,rc,result?result:"");
    return rc;
}

static int barrier_present(void){int up;Forbid();up=FindPort(ND_CONTROL_PORT)!=NULL;Permit();return up;}
int nd_native_rexx_save_barrier(void){FILE *f;BPTR in,out,guard;int i,k,reproduced=0;LONG rc;char text[600];
 guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);if(!guard){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}UnLock(guard);setvbuf(stdout,NULL,_IONBF,0);
 SystemTags("MakeDir RAM:RaceEnv RAM:RaceEnv/NovaDock RAM:RaceArc",TAG_DONE);SystemTags("Assign ENV: RAM:RaceEnv",TAG_DONE);SystemTags("Assign ENVARC: RAM:RaceArc",TAG_DONE);SystemTags("Assign NovaDock: QATests:Dock",TAG_DONE);
 f=fopen("ENV:NovaDock/novadock.cfg","w");if(!f){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}fputs("SIDE=BOTTOM\nGAP=5\nDRAG_BAR=NO\nAUTO_HIDE=NO\nTOOLTIPS=NO\nLAUNCH=Shell|NewShell||ON|AUTO\n",f);fclose(f);
 in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);if(SystemTags("QATests:Dock/NovaDock",SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE)<0){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}
 for(i=0;i<400&&!barrier_present();++i)Delay(1);if(!barrier_present()){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}Delay(50);
 for(k=0;k<8;++k){
  if(barrier_rexx("SET GAP 5",text,sizeof(text)) || barrier_rexx("STATUS",text,sizeof(text))){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}
  Delay(3+k*2);
  if(barrier_rexx("SET GAP 6",text,sizeof(text))){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}
  for(i=0;i<150;++i){rc=nd_control_send(ND_CONTROL_SAVE_LOCK);if(rc==ND_CONTROL_OK)break;Delay(1);}printf("LOCK attempts=%d rc=%ld\n",i,rc);if(i==150){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}
  if(nd_load_config(&barrier_config,"ENV:NovaDock/novadock.cfg")!=0){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}barrier_config.gap=13;
  if(!nd_write_config(&barrier_config,"ENV:NovaDock/novadock.cfg")){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}
  if(nd_control_send(ND_CONTROL_RELOAD)!=ND_CONTROL_OK){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}
  if(barrier_rexx("GET GAP",text,sizeof(text))){printf("FAIL at line %d DOS %ld\n",__LINE__,IoErr());return 0;}
  printf("EXTERNAL-SAVE iteration=%d expected=13 actual=%s\n",k,text);
  if(strcmp(text,"13"))++reproduced;
  nd_control_send(ND_CONTROL_SAVE_UNLOCK);
 }
 nd_control_send(ND_CONTROL_QUIT);for(i=0;i<300&&barrier_present();++i)Delay(1);
 printf("REPRODUCED lost Preferences save %d of 8 attempts\n",reproduced);return reproduced==0 && i<300;
}

#ifdef ND_NATIVE_BARRIER_STANDALONE
long __stack=65536;
int main(void){return nd_native_rexx_save_barrier() ? 0:10;}
#endif
