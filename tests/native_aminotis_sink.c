/* Disposable protocol-v2 receiver; mirror the SDK's atomic message ownership. */
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "../src/aminotis_sdk/aminotis_wire.h"
long __stack=32768;
int main(void)
{
 struct MsgPort *p;struct ANMessage *m;FILE *f;ULONG serial=0;char log[800];BPTR guard=Lock("QATests:ISOLATED-QA-ONLY",ACCESS_READ);
 if(!guard)return 20;UnLock(guard);p=CreateMsgPort();if(!p)return 20;
 p->mp_Node.ln_Name=AN_PORT;Forbid();if(FindPort(AN_PORT)){Permit();DeleteMsgPort(p);return 20;}AddPort(p);Permit();
 for(;;){ULONG bits=Wait((1UL<<p->mp_SigBit)|SIGBREAKF_CTRL_C);if(bits&SIGBREAKF_CTRL_C)break;
  for(;;){log[0]=0;Forbid();m=(struct ANMessage *)GetMsg(p);if(!m){Permit();break;}
   memset(&m->response,0,sizeof(m->response));m->response.result=AN_INVALID;
   if(m->magic==AN_MAGIC&&m->version==AN_VERSION&&m->message.mn_Length==sizeof(*m)){
    m->response.result=AN_OK;m->response.status.capabilities=AN_CAP_HISTORY;
    if(m->request.operation==AN_POST){m->response.id=++serial;sprintf(log,"%s|%s|%s|%s|%lu\n",m->request.event.app,m->request.event.source,m->request.event.title,m->request.event.body,m->request.event.severity);}
    else if(m->request.operation!=AN_STATUS)m->response.result=AN_INVALID;
   }
   m->phase=AN_REPLIED;ReplyMsg(&m->message);Permit();
   if(log[0]){f=fopen("QATests:notis-received.log","a");if(f){fputs(log,f);fclose(f);}}
  }
 }
 Forbid();RemPort(p);while((m=(struct ANMessage *)GetMsg(p))!=NULL){m->response.result=AN_UNAVAILABLE;ReplyMsg(&m->message);}Permit();DeleteMsgPort(p);return 0;
}
