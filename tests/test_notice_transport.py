from pathlib import Path
import os,subprocess,tempfile
R=Path(__file__).resolve().parents[1]
s=(R/'plugin_src/common/nd_client.c').read_text();a=s.index('unsigned int nd_client_notify(');b=s.index('unsigned int nd_client_update_art_meter_quiet',a)
code=r'''#include <stdio.h>
#include <string.h>
#include <assert.h>
typedef unsigned short UWORD;
#define ND_TEXT_LEN 32
#define ND_MSG_NOTICE 4
#define ND_MSG_NOTICE_LONG 7
#define ND_EVENT_NONE 0
struct NDMessage {char caption[32],value[32];unsigned short meter;};
struct NDExtMessage {struct NDMessage base;unsigned long metadata[6];};
struct NDClient {int host_alerts,host_long_notice;union {struct {struct NDMessage base;}graph;struct NDExtMessage ext;struct {struct NDExtMessage base;char title[81],text[513];}notice;}packet;};
static int sent;
static unsigned int exchange(struct NDClient *c,unsigned short cmd,int ext){(void)ext;++sent;if(c->host_long_notice){assert(cmd==ND_MSG_NOTICE_LONG&&strlen(c->packet.notice.title)==80&&strlen(c->packet.notice.text)==512&&c->packet.notice.text[511]=='B');}else assert(cmd==ND_MSG_NOTICE&&strlen(c->packet.graph.base.caption)==31&&strlen(c->packet.graph.base.value)==31);memset(&c->packet.ext,0xff,sizeof(c->packet.ext));return 9;}
'''+s[a:b]+r'''
int main(void){struct NDClient c;struct NDExtMessage saved;char title[100],body[700];memset(&c,0,sizeof(c));memset(title,'T',99);title[99]=0;memset(body,'B',699);body[699]=0;strcpy(c.packet.graph.base.value,"Unchanged widget");saved=c.packet.ext;
 assert(!nd_client_notify(&c,title,body,0)&&!sent);c.host_alerts=1;assert(nd_client_notify(&c,title,body,1)==9&&sent==1&&!memcmp(&saved,&c.packet.ext,sizeof(saved)));
 c.host_long_notice=1;assert(nd_client_notify(&c,title,body,2)==9&&sent==2&&!memcmp(&saved,&c.packet.ext,sizeof(saved)));assert(!nd_client_notify(&c,title,body,3)&&sent==2);
 puts("Notice transport: negotiated 80/512 text, exact 31-character legacy fallback, text bounds and complete widget-prefix preservation passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.c').write_text(code);subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
