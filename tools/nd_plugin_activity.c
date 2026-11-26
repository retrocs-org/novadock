/* Live, local plugin resource view. Measurement does not enable log capture. */
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_control_client.h"
#include "nd_panel.h"
static struct NDControlPluginResourcesMessage activity;
static void activity_rows(struct NDPanel *panel,char text[8][128],char labels[8][32])
{
 unsigned int i;char ram[24],cpu[24];struct NDPluginResourceRow *row;
 for(i=0;i<8;++i){text[i][0]=labels[i][0]=0;}
 strcpy(labels[0],"Reading");strcpy(text[0],"Tracked RAM | sampled CPU | latest operation");
 for(i=0;i<activity.count && i<6;++i){row=&activity.rows[i];sprintf(labels[i+1],"%.15s",row->label);
  if(row->stats.version==1)sprintf(ram,"%luK%s",(row->stats.heap+1023UL)/1024UL,row->stats.flags&1?"+":"");else strcpy(ram,"--");
  if(row->cpu_valid){ULONG n=row->cpu_samples ? row->cpu_hits*1000UL/row->cpu_samples:0;sprintf(cpu,"~%lu.%lu%%",n/10UL,n%10UL);}else strcpy(cpu,"--");
  sprintf(text[i+1],"%s | %s | %.20s%s",ram,cpu,row->stats.version==1?nd_plugin_operation(row->stats.operation):"No resource data",row->stale?" (stale)":!row->online?" (offline)":"");
 }
 strcpy(labels[7],"Sample");
 if(activity.sample_remaining)sprintf(text[7],"Measuring... %lu seconds left",(activity.sample_remaining+49UL)/50UL);
 else sprintf(text[7],"Estimates; RAM excludes libraries, stack and code.");
 if(panel)for(i=0;i<8;++i){nd_panel_set_label(panel,i,labels[i]);nd_panel_set(panel,i,text[i]);}
 if(panel)nd_panel_button(panel,2,"Sample 5s",activity.sample_remaining==0);
}
int nd_plugin_activity(void)
{
 struct NDPanel *panel;struct NDPanelField fields[8];char text[8][128],labels[8][32];unsigned int i,ticks=0;int action,done=0;
 static const char *buttons[]={"Previous","Next","Sample 5s","Refresh","Close"};
 memset(&activity,0,sizeof(activity));activity.operation=1;
 if(nd_control_plugin_resources(&activity)!=ND_CONTROL_OK)return 20;activity.operation=0;activity_rows(NULL,text,labels);
 for(i=0;i<8;++i){fields[i].label=labels[i];fields[i].value=text[i];fields[i].limit=0;}
 panel=nd_panel_open("NovaDock - Plugin activity",fields,8,buttons,5);if(!panel)return 20;activity_rows(panel,text,labels);
 while(!done){while((action=nd_panel_event(panel))!=0){
  if(action==-1 || action==104){done=1;break;}
  if(action==100 && activity.offset>=6)activity.offset-=6;
  if(action==101 && activity.offset+6<activity.total)activity.offset+=6;
  if(action==102 && !activity.sample_remaining)activity.operation=1;
  if(nd_control_plugin_resources(&activity)==ND_CONTROL_OK){activity.operation=0;activity_rows(panel,text,labels);}ticks=0;
 }
 if(++ticks>=10){if(nd_control_plugin_resources(&activity)==ND_CONTROL_OK)activity_rows(panel,text,labels);ticks=0;}
 Delay(5);
 }nd_panel_close(panel);return 0;
}
