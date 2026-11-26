/* Read-only SDK interoperability check in a disposable AmigaMac guest. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_hass_bridge.h"
long __stack=65536;
static int running(void)
{int live;Forbid();live=FindPort(AMIHASS_PORT)!=NULL;Permit();return live;}
static struct AmiHASSMessage *exchange(struct AmiHASSClient *c,int kind,int offset)
{
    struct AmiHASSMessage *m;int i;
    if(!amihass_send(c,kind,"NovaDock.QA",NULL,NULL,offset,0,NULL,NULL))return NULL;
    for(i=0;i<120;++i){m=amihass_reply(c);if(m)return m;Delay(5);}
    return NULL;
}
int main(void)
{
    struct NDHassBridge tile;struct AmiHASSClient observer;
    struct AmiHASSMessage *m;BPTR lock,in,out;LONG rc;int i,offset,total,result=10,owned=0;
    memset(&tile,0,sizeof(tile));memset(&observer,0,sizeof(observer));
    if(running()){puts("FAIL: this test requires its own isolated bridge.");return 10;}
    lock=Lock("ENVARC:AmiHASS/Private/connection",ACCESS_READ);
    if(lock){UnLock(lock);puts("FAIL: this test requires a guest without saved credentials.");return 10;}
    in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);
    if(!in || !out){if(in)Close(in);if(out)Close(out);return 10;}
    rc=SystemTags("Bridge:AmiHASSService SERVICE",SYS_Input,in,SYS_Output,out,
                  SYS_Asynch,TRUE,NP_StackSize,65536UL,TAG_DONE);
    if(rc<0){Close(in);Close(out);return 10;}owned=1;
    for(i=0;i<100 && !running();++i)Delay(5);
    if(!running()){puts("FAIL: bridge did not start.");goto finished;}
    m=exchange(&observer,AMIHASS_STATUS,0);
    if(!m){puts("FAIL: no ABI 1 status reply.");goto finished;}
    rc=m->result;FreeVec(m);if(rc!=AMIHASS_OK)goto finished;
    puts("PASS: NovaDock client reads real AmiHASSService status.");
    nd_hass_bridge_close(&tile);
    for(i=0;i<120;++i){nd_hass_bridge_poll(&tile,"HASS_QA","sensor.novadock_qa",i ? 5:0);
        if(!tile.client.pending && tile.result==AMIHASS_UNCONFIGURED)break;Delay(5);}
    if(i==120){puts("FAIL: unconfigured bridge did not reach the tile.");goto finished;}
    nd_hass_bridge_action(&tile,"HASS_QA","sensor.novadock_qa","Toggle");
    nd_hass_bridge_close(&tile);
    offset=0;total=-1;
    do {
        m=exchange(&observer,AMIHASS_DIAGNOSTICS,offset);
        if(!m){puts("FAIL: no diagnostic reply from bridge.");goto finished;}
        rc=m->result==AMIHASS_OK && m->offset==offset && m->count>0 &&
           m->count<=AMIHASS_PAGE && m->total>=offset+m->count &&
           (total<0 || m->total==total);
        if(total<0)total=m->total;
        for(i=0;i<m->count && i<AMIHASS_PAGE;++i){const char *key=m->items[i].id;
            if(!strcmp(key,"ipc.actions") || !strncmp(key,"http.",5))
                if(strtoul(m->items[i].state,NULL,10)!=0)rc=0;}
        offset+=m->count;FreeVec(m);
        if(!rc){puts("FAIL: invalid diagnostic page or unexpected traffic.");goto finished;}
    }while(offset<total);
    printf("PASS: tile state and all %d diagnostic rows work; no HTTP or device actions.\n",total);
    result=0;
finished:
    nd_hass_bridge_close(&tile);
    if(owned){m=exchange(&observer,AMIHASS_STOP,0);if(m)FreeVec(m);
        for(i=0;i<100 && running();++i)Delay(5);
        if(running()){puts("FAIL: test bridge did not stop.");result=10;}}
    amihass_client_close(&observer);return result;
}
