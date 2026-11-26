from pathlib import Path
import os,subprocess,tempfile
R=Path(__file__).resolve().parents[1]
s=(R/'prefs/novadockprefs.c').read_text();a=s.index('static void refresh_update_status(');b=s.index('static int open_editor(',a)
code=r'''#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "nd_update.h"
typedef unsigned long ULONG;
#define GTTX_Text 1
#define GA_Disabled 2
#define TAG_DONE 0
#define FORMAT_DOS 0
struct DateTime {struct {unsigned long ds_Days,ds_Minute,ds_Tick;}dat_Stamp;int dat_Format;char *dat_StrDate,*dat_StrTime;};
struct Prefs {int update_page;void *win,*g_update_last,*g_update_status,*g_update_now;unsigned int update_requested;unsigned long update_previous;int update_rendered,update_disabled;char update_status[160],update_last[80];};
static struct NDUpdateStatus value;static int writes,dates;
void nd_update_status(struct NDUpdateStatus *out){*out=value;}
static int DateToStr(struct DateTime *dt){++dates;strcpy(dt->dat_StrDate,"date");sprintf(dt->dat_StrTime,"%lu",dt->dat_Stamp.ds_Tick);return 1;}
static void GT_SetGadgetAttrs(void *g,void *w,void *x,int tag,...){(void)g;(void)w;(void)x;(void)tag;++writes;}
'''+s[a:b]+r'''
int main(void){struct Prefs p;int i;memset(&p,0,sizeof(p));p.win=&p;p.update_page=1;
 refresh_update_status(&p);assert(writes==3&&!dates);
 writes=0;for(i=0;i<100;++i)refresh_update_status(&p);assert(!writes&&!dates);
 value.checked=100;value.result=1;refresh_update_status(&p);assert(writes==2&&dates==1&&p.update_previous==100);
 writes=0;for(i=0;i<100;++i)refresh_update_status(&p);assert(!writes&&dates==1);
 value.busy=value.checking=1;refresh_update_status(&p);assert(writes==2&&p.update_disabled);
 writes=0;refresh_update_status(&p);assert(!writes);
 value.busy=value.checking=0;value.result=2;strcpy(value.version,"0.7");refresh_update_status(&p);assert(writes==2&&!p.update_disabled&&strstr(p.update_status,"0.7"));
 writes=0;p.update_requested=5;refresh_update_status(&p);assert(p.update_requested==5&&writes==2);
 writes=0;refresh_update_status(&p);assert(!writes&&p.update_requested==5);
 value.checked=101;refresh_update_status(&p);assert(!p.update_requested&&dates==2&&writes==3);
 p.update_rendered=0;writes=0;refresh_update_status(&p);assert(writes==3);
 puts("Update page: 200 unchanged refreshes cause zero redraws; status/button/date changes, pending checks and reopen repaint passed.");return 0;}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'check.c').write_text(code)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wno-deprecated-declarations','-fsanitize=address,undefined','-I'+str(R/'include'),str(p/'check.c'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],check=True)
