"""Fail closed for old bridge features; page room IDs independently of labels."""
from pathlib import Path
import os,subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nd_hass_native.h"
static struct AmiHASSMessage m,*incoming;static int sent,kind,offset;
static void FreeVec(void *p){assert(p==&m);}
void amihass_client_close(struct AmiHASSClient *c){c->pending=NULL;incoming=NULL;}
int amihass_send(struct AmiHASSClient *c,int k,const char *o,const char *e,const char *a,int at,int f,const char *d,const char *q){assert(!c->pending);(void)o;(void)e;(void)a;(void)f;(void)d;(void)q;kind=k;offset=at;++sent;c->pending=&m;return 1;}
int amihass_send_area(struct AmiHASSClient *c,int k,const char *o,const char *e,int at,int f,const char *d,const char *q,const char *area){assert(area&&!strcmp(area,"study"));return amihass_send(c,k,o,e,NULL,at,f,d,q);}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c){struct AmiHASSMessage *p=incoming;incoming=NULL;if(p)c->pending=NULL;return p;}
#include "nd_hass_picker.h"
static void answer(int flags){int i;memset(&m,0,sizeof(m));m.kind=AMIHASS_STATUS;m.result=AMIHASS_OK;m.count=3;strcpy(m.items[0].id,AMIHASS_CAP_AREAS);strcpy(m.items[1].id,AMIHASS_CAP_FILTERS);strcpy(m.items[2].id,AMIHASS_CAP_SUBSCRIPTIONS);for(i=0;i<3;++i)strcpy(m.items[i].state,flags&(1<<i) ? "1":"0");incoming=&m;}
int main(void){struct NDHassPicker p;int n,i;
 for(i=0;i<4;++i){nd_hass_picker_init(&p,1,NULL,"light");nd_hass_picker_scope(&p,"study","");nd_hass_picker_step(&p);assert(kind==AMIHASS_STATUS);n=sent;answer(i==0 ? 0:i==1 ? 1:i==2 ? 2:7);nd_hass_picker_step(&p);
 if(i<3)assert(p.result==AMIHASS_UNSUPPORTED&&!p.count&&!p.request&&sent==n);else assert(p.checked&&kind==AMIHASS_DEVICES&&sent==n+1);amihass_client_close(&p.client);}
 nd_hass_picker_init(&p,2,NULL,NULL);p.capacity=2;nd_hass_picker_step(&p);answer(7);nd_hass_picker_step(&p);assert(kind==AMIHASS_AREAS);
 memset(&m,0,sizeof(m));m.kind=AMIHASS_AREAS;m.result=AMIHASS_OK;m.count=2;m.total=3;strcpy(m.items[0].id,"study");strcpy(m.items[1].id,"kitchen");strcpy(m.items[0].name,"Same label");strcpy(m.items[1].name,"Same label");incoming=&m;nd_hass_picker_step(&p);assert(p.count==2&&p.total==3&&strcmp(p.items[0].id,p.items[1].id));
 nd_hass_picker_request(&p,"",2,1);nd_hass_picker_step(&p);answer(7);nd_hass_picker_step(&p);assert(kind==AMIHASS_AREAS&&offset==2);
 memset(&m,0,sizeof(m));m.kind=AMIHASS_AREAS;m.result=AMIHASS_OK;m.offset=2;m.count=1;m.total=3;strcpy(m.items[0].id,"house");strcpy(m.items[0].name,"House");incoming=&m;nd_hass_picker_step(&p);assert(p.count==1&&!strcmp(p.items[0].id,"house"));
 nd_hass_picker_request(&p,"",0,0);nd_hass_picker_step(&p);answer(7);m.kind=AMIHASS_LIST;nd_hass_picker_step(&p);assert(p.result==AMIHASS_FAILED&&!p.request);
 puts("Bridge capability handshake: absent/partial/foreign features fail closed; room IDs, duplicate labels and paging passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'exec').mkdir();(p/'exec/ports.h').write_text('#include <stdint.h>\ntypedef uint32_t ULONG;typedef int32_t LONG;struct MsgPort;struct Message{unsigned short mn_Length;struct MsgPort *mn_ReplyPort;};\n');(p/'t.c').write_text(s)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(p),'-I'+str(r/'plugin_src/hass'),str(p/'t.c'),str(r/'plugin_src/hass/nd_hass.c'),'-o',str(p/'test')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'test')],check=True)
