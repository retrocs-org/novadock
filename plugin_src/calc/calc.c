#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_client.h"
#include "nd_panel.h"
#include "nd_list.h"
#include "nd_task.h"
#include "nd_text.h"
#include "nd_plugin_versions.h"
#include "nd_calc.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
struct Library *MathIeeeDoubBasBase=NULL;
const char nd_version[]=ND_PLUGIN_VER_STRING(CALC,"Calc");long __stack=65536;
static char expressions[ND_CALC_HISTORY][160],results[ND_CALC_HISTORY][48];static unsigned int count;
static int work(unsigned int action,void *data,unsigned int bytes,const char *parent)
{(void)parent;if(action!=1 || !bytes || bytes>48 || ((char *)data)[bytes-1])return 0;return nd_text_copy(data);}
static struct NDPanel *open_calc(void)
{struct NDPanelField f[]={{"Expression","",160},{"Result","Enter a calculation",0},{"Hex","",0},{"Binary","",0}};static const char *b[]={"Evaluate","Copy result","History","Close"};return nd_panel_open("Calculator",f,4,b,4);}
static void evaluate(struct NDPanel *panel,struct NDCalcResult *result)
{const char *text=nd_panel_get(panel,0);unsigned int i;if(!nd_calc_eval(text,result)){nd_panel_title(panel,result->error);nd_panel_set(panel,1,result->error);nd_panel_set(panel,2,"");nd_panel_set(panel,3,"");return;}nd_panel_title(panel,"Calculator");nd_panel_set(panel,1,result->decimal);nd_panel_set(panel,2,result->hex);nd_panel_set(panel,3,result->binary);
 if(count && !strcmp(expressions[count-1],text))return;if(count==ND_CALC_HISTORY){for(i=1;i<count;++i){strcpy(expressions[i-1],expressions[i]);strcpy(results[i-1],results[i]);}--count;}strncpy(expressions[count],text,159);strcpy(results[count++],result->decimal);}
int main(int argc,char **argv)
{
 struct NDClient dock;struct NDPanel *panel=NULL;struct NDList *list=NULL;struct NDTask job;struct NDTaskPacket *reply;struct NDCalcResult result;unsigned int event,elapsed=0,i;int action,selected;char value[32],rows[ND_CALC_HISTORY][160];const char *ptr[ND_CALC_HISTORY];static const char *buttons[]={"Reuse","Clear history","Close"};
 if(argc==3 && !strcmp(argv[1],"TASK"))return nd_task_worker(argv[2],work);
 MathIeeeDoubBasBase=OpenLibrary("mathieeedoubbas.library",0);if(!MathIeeeDoubBasBase)return 20;
 if(argc==2 && !strcmp(argv[1],"SELFTEST")){int rc;rc= nd_calc_eval("1.5+2.25",&result) && !strcmp(result.decimal,"3.75") && nd_calc_eval("0xff << 4",&result) && !strcmp(result.hex,"0x00000ff0") ? 0:10;CloseLibrary(MathIeeeDoubBasBase);return rc;}
 if(argc!=2 || !nd_client_init(&dock,argv[1])){CloseLibrary(MathIeeeDoubBasBase);return 20;}((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;memset(&job,0,sizeof(job));memset(&result,0,sizeof(result));strcpy(value,"Click to calculate");
 for(;;){
  reply=nd_task_poll(&job);if(reply){if(panel)nd_panel_title(panel,reply->result==1 ? "Result copied":"Clipboard copy failed");FreeVec(reply);}
  event=nd_client_update_art_quiet(&dock,"CALC",value,"calc",0,elapsed,250);if(event&ND_EVENT_QUIT)break;
  if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=open_calc();}
  action=nd_panel_event(panel);if(action==-1 || action==103){nd_panel_close(panel);panel=NULL;}
  else if(action==1 || action==100){evaluate(panel,&result);if(!result.error[0])sprintf(value,"%.31s",result.decimal);}
  else if(action==101){if(!result.decimal[0] || result.error[0])nd_panel_title(panel,"Evaluate a valid expression first");else if(!nd_task_start(&job,argv[0],"NovaDock.Calc.Clipboard",1,result.decimal,strlen(result.decimal)+1))nd_panel_title(panel,"Clipboard worker is busy");}
  else if(action==102){if(list)nd_list_raise(list);else list=nd_list_open("Calculation history",buttons,3);}
  for(i=0;i<count;++i){sprintf(rows[i],"%.100s = %.47s",expressions[i],results[i]);ptr[i]=rows[i];}nd_list_rows(list,ptr,count);
  action=nd_list_event(list);selected=nd_list_selected(list);if(action==-1 || action==102){nd_list_close(list);list=NULL;}
  else if(action==101)count=0;
  else if(action==100 && selected>=0 && selected<(int)count){if(!panel)panel=open_calc();if(panel){nd_panel_set(panel,0,expressions[selected]);evaluate(panel,&result);nd_panel_raise(panel);}}
  elapsed=nd_client_wait_signals(&dock,panel || list || job.port ? 10:250,nd_panel_signals(panel)|nd_list_signals(list)|nd_task_signals(&job));
 }
 nd_task_close(&job);nd_list_close(list);nd_panel_close(panel);nd_client_close(&dock);CloseLibrary(MathIeeeDoubBasBase);return 0;
}
