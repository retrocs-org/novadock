from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'prefs/novadockpluginprefs.c').read_text();a=s.index('static int hass_capabilities_step(');b=s.index('/* Keep room summaries compact',a)
code=r"""
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nd_hass_native.h"
#include "nd_hass_actions.h"
struct PluginPrefs {struct NDHassClient hass_client;struct AHControls hass_controls;char hass_target[96],entity[96],label[32];int hass_known,hass_attempts;unsigned int hass_wait;};
static struct NDHassMessage packet,*incoming;static int sends=0,closed=0,timeout=0;
static int hass_settings(struct PluginPrefs *p){(void)p;return 1;}
static const char *hass_edit(struct PluginPrefs *p,const char *key){assert(!strcmp(key,"ENTITY"));return p->entity;}
static void FreeVec(void *p){assert(p==&packet);}
void amihass_client_close(struct AmiHASSClient *c){++closed;c->pending=NULL;incoming=NULL;}
int amihass_send(struct AmiHASSClient *c,int kind,const char *owner,const char *entity,const char *action,int offset,int refresh,const char *domain,const char *filter){assert(kind==AMIHASS_QUERY&&!strcmp(owner,"HASS.Prefs")&&strstr(entity,".amihass_sample_")&&!action&&!offset&&refresh==AMIHASS_QUERY_ONCE&&!domain&&!filter);assert(!c->pending);++sends;c->pending=&packet;return 1;}
struct AmiHASSMessage *amihass_reply(struct AmiHASSClient *c){struct AmiHASSMessage *m=incoming;incoming=NULL;if(timeout){timeout=0;c->pending=NULL;return NULL;}if(m)c->pending=NULL;return m;}
"""+s[a:b]+r"""
static void answer(int result,const char *entity,int caps){memset(&packet,0,sizeof(packet));packet.kind=AMIHASS_QUERY;packet.result=result;strcpy(packet.item.id,entity);if(caps){packet.count=3;strcpy(packet.items[0].id,"control.lock");strcpy(packet.items[1].id,"control.unlock");strcpy(packet.items[2].id,"control.lock_open");strcpy(packet.items[0].state,"yes");strcpy(packet.items[1].state,"yes");strcpy(packet.items[2].state,"yes");}incoming=&packet;}
int main(void){struct PluginPrefs p;char choices[128];int n,i;memset(&p,0,sizeof(p));strcpy(p.entity,"lock.amihass_sample_front_door");strcpy(p.label,"Unsaved label");assert(hass_capabilities_step(&p)&&p.hass_client.pending&&sends==1&&!p.hass_known);nd_hass_action_choices(p.entity,&p.hass_controls,p.hass_known==1,choices);assert(!strcmp(choices,"Show controls"));answer(AMIHASS_WAIT,p.entity,0);hass_capabilities_step(&p);n=sends;for(i=0;i<18;++i)hass_capabilities_step(&p);assert(sends==n);while(!p.hass_client.pending)hass_capabilities_step(&p);answer(AMIHASS_OK,p.entity,1);assert(hass_capabilities_step(&p)&&p.hass_known==1&&p.hass_controls.unlock);nd_hass_action_choices(p.entity,&p.hass_controls,1,choices);assert(strstr(choices,"Unlock"));n=sends;for(i=0;i<100;++i)hass_capabilities_step(&p);assert(sends==n&&!strcmp(p.label,"Unsaved label"));
strcpy(p.entity,"lock.amihass_sample_pin");assert(hass_capabilities_step(&p)&&p.hass_client.pending&&!p.hass_known&&!p.hass_controls.unlock);answer(AMIHASS_OK,p.entity,0);assert(hass_capabilities_step(&p)&&p.hass_known==1&&!p.hass_controls.unlock);nd_hass_action_choices(p.entity,&p.hass_controls,1,choices);assert(!strcmp(choices,"Show controls"));
strcpy(p.entity,"lock.amihass_sample_offline");hass_capabilities_step(&p);timeout=1;assert(hass_capabilities_step(&p)&&p.hass_known==-1&&!p.hass_client.pending);n=sends;for(i=0;i<100;++i)hass_capabilities_step(&p);assert(sends==n);
strcpy(p.entity,"lock.amihass_sample_wrong");hass_capabilities_step(&p);answer(AMIHASS_OK,"lock.amihass_sample_other",1);assert(hass_capabilities_step(&p)&&p.hass_known==-1&&!p.hass_controls.unlock);
printf("HASS Preferences: asynchronous read-only capability queries, bounded retry, unchanged edits, unsupported/PIN and stale-target filtering passed.\n");return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'exec').mkdir();(p/'exec/ports.h').write_text('typedef unsigned long ULONG;typedef long LONG;struct MsgPort;struct Message{unsigned short mn_Length;struct MsgPort *mn_ReplyPort;};\n');(p/'check.c').write_text(code)
 cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(p),'-I'+str(ROOT/'plugin_src/hass'),str(p/'check.c'),str(ROOT/'plugin_src/hass/nd_hass.c'),'-o',str(p/'check')]
 if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
 subprocess.run(cmd,check=True);subprocess.run([str(p/'check')],check=True)
