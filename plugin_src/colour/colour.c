#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <string.h>
#include "nd_client.h"
#include "nd_panel.h"
#include "nd_list.h"
#include "nd_task.h"
#include "nd_text.h"
#include "nd_option_runtime.h"
#include "nd_colour.h"
#include "nd_plugin_versions.h"
#include "../screenshot/nd_cybergraphics.h"
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;struct Library *CyberGfxBase=NULL;
const char nd_version[]=ND_PLUGIN_VER_STRING(COLOUR,"Colour");long __stack=65536;
#define PALETTE "ENVARC:NovaDock/Colours/Palette"
#include "nd_colour_job.h"
static int sample(unsigned long *out)
{struct Screen *screen=NULL;int x,y,depth,front=0,ok=0;LONG pen;ULONG colours[3];unsigned char pixel[3];
 IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);if(!IntuitionBase||!GfxBase)goto done;screen=LockPubScreen("Workbench");if(!screen)goto done;Forbid();front=IntuitionBase->FirstScreen==screen;Permit();if(!front)goto done;x=screen->MouseX;y=screen->MouseY;if(x<0||y<0||x>=screen->Width||y>=screen->Height)goto done;depth=GetBitMapAttr(screen->RastPort.BitMap,BMA_DEPTH);
 if(depth>8){CyberGfxBase=OpenLibrary("cybergraphics.library",41);if(CyberGfxBase&&nd_cgx_bitmap(screen->RastPort.BitMap)&&nd_cgx_rgb_pixel(pixel,&screen->RastPort,x,y)==1){*out=((unsigned long)pixel[0]<<16)|((unsigned long)pixel[1]<<8)|pixel[2];ok=1;}}
 else if(!(screen->ViewPort.Modes&HAM)){pen=ReadPixel(&screen->RastPort,x,y);if(pen>=0){int half=(screen->ViewPort.Modes&EXTRA_HALFBRITE)&&pen>=32;if(half)pen-=32;GetRGB32(screen->ViewPort.ColorMap,pen,1,colours);pixel[0]=(unsigned char)(colours[0]>>24);pixel[1]=(unsigned char)(colours[1]>>24);pixel[2]=(unsigned char)(colours[2]>>24);if(half){pixel[0]/=2;pixel[1]/=2;pixel[2]/=2;}*out=((unsigned long)pixel[0]<<16)|((unsigned long)pixel[1]<<8)|pixel[2];ok=1;}}
 done:if(CyberGfxBase)CloseLibrary(CyberGfxBase);CyberGfxBase=NULL;if(screen)UnlockPubScreen(NULL,screen);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);GfxBase=NULL;IntuitionBase=NULL;return ok;}
static int work(unsigned int action,void *data,unsigned int bytes,const char *parent)
{struct ColourJob *j=data;char hex[8],rgb[32];int r;(void)parent;if(bytes!=sizeof(*j)||!memchr(j->name,0,sizeof(j->name)))return 0;if(action==1)return sample(&j->rgb);if(action==2){nd_colour_text(j->rgb,hex,rgb);return nd_text_copy(j->copy_rgb ? rgb:hex);}if(action==3){r=nd_colour_read(PALETTE,&j->palette);if(r==-1){memset(&j->palette,0,sizeof(j->palette));return 1;}return r;}
 if(action==4){BPTR lock;lock=CreateDir("ENVARC:NovaDock");if(lock)UnLock(lock);lock=CreateDir("ENVARC:NovaDock/Colours");if(lock)UnLock(lock);j->problem[0]=0;r=nd_colour_save(PALETTE,&j->palette);if(!r){strncpy(j->problem,nd_colour_save_problem(),sizeof(j->problem)-1);j->problem[sizeof(j->problem)-1]=0;}return r;}return 0;}
static struct NDPanel *details(struct ColourJob *j,const char *status)
{char hex[8],rgb[32];struct NDPanelField f[5];const char *b[]={"Pick (3s)",j->copy_rgb ? "Copy RGB":"Copy hex","Save colour","Palette"};nd_colour_text(j->rgb,hex,rgb);f[0].label="Hex colour";f[0].value=hex;f[0].limit=8;f[1].label="RGB";f[1].value=rgb;f[1].limit=0;f[2].label="Palette name";f[2].value=j->name;f[2].limit=32;f[3].label="Status";f[3].value=status;f[3].limit=0;f[4].label="Preview";f[4].value="";f[4].limit=ND_PANEL_SWATCH;{struct NDPanel *p=nd_panel_open("COLOUR - Workbench screen sampler",f,5,b,4);nd_panel_swatch(p,4,j->rgb);return p;}}
static void update_panel(struct NDPanel *p,struct ColourJob *j,const char *status,int replace)
{char hex[8],rgb[32];if(!p)return;nd_colour_text(j->rgb,hex,rgb);if(replace){nd_panel_replace_string(p,0,hex);nd_panel_replace_string(p,2,j->name);}else nd_panel_sync_string(p,0,hex);nd_panel_set(p,1,rgb);nd_panel_set(p,3,status);nd_panel_swatch(p,4,j->rgb);nd_panel_button(p,1,j->copy_rgb ? "Copy RGB":"Copy hex",1);}
static void copy_option(struct ColourJob *j,const char *id)
{char format[8];struct NDOptionString f={"COPY_FORMAT","HEX",format,sizeof(format)};nd_plugin_option_strings(id,&f,1);j->copy_rgb=!strcmp(format,"RGB");}
int main(int argc,char **argv)
{struct NDClient dock;struct NDTask task;struct NDTaskPacket *reply;struct ColourJob j,candidate;struct NDPanel *panel=NULL;struct NDList *list=NULL;unsigned int event,elapsed=0,countdown=0,i;int action,selected,loaded=0,palette_dirty=1;unsigned long shown_rgb=~0UL;char hex[8],rgb[32],status[128]="Pick samples under the pointer after 3 seconds",rows[ND_COLOUR_MAX][80];const char *ptr[ND_COLOUR_MAX],*buttons[]={"Use","Remove","Close"};
 if(argc==3&&!strcmp(argv[1],"TASK"))return nd_task_worker(argv[2],work);if(argc==2&&!strcmp(argv[1],"SELFTEST")){unsigned long colour;return nd_colour_hex("#abcdef",&colour)&&colour==0xabcdefUL ? 0:10;}
 if(argc==2&&!strcmp(argv[1],"SAMPLE-SELFTEST")){unsigned long colour;return sample(&colour)&&colour<=0xffffffUL ? 0:10;}
 if(argc!=2||!nd_client_init(&dock,argv[1]))return 20;memset(&task,0,sizeof(task));memset(&j,0,sizeof(j));j.rgb=0x8fbccd;copy_option(&j,argv[1]);((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 for(;;){if(!loaded&&!task.port){if(nd_task_start(&task,argv[0],"NovaDock.Colour.Worker",3,&j,sizeof(j)))loaded=1;}
  reply=nd_task_poll(&task);if(reply){if(reply->result==1&&reply->bytes==sizeof(j)){if(reply->action==1)j.rgb=((struct ColourJob *)reply->data)->rgb;else if(reply->action==3||reply->action==4){j.palette=((struct ColourJob *)reply->data)->palette;palette_dirty=1;}strcpy(status,reply->action==1 ? "Sampled Workbench pixel":reply->action==2 ? (j.copy_rgb ? "RGB copied":"Hex copied"):reply->action==4 ? "Palette saved with backup":"Palette loaded");if(panel){if(reply->action==1)nd_panel_title(panel,"COLOUR - Workbench screen sampler");update_panel(panel,&j,status,0);}}else{strcpy(status,reply->action==4&&reply->bytes==sizeof(j)&&((struct ColourJob *)reply->data)->problem[0] ? ((struct ColourJob *)reply->data)->problem:reply->action==1 ? "Sampling failed: front Workbench, indexed or supported RTG required":"Palette/copy operation failed; previous palette retained");if(panel)nd_panel_set(panel,3,status);}FreeVec(reply);}
  if(shown_rgb!=j.rgb){nd_colour_text(j.rgb,hex,rgb);shown_rgb=j.rgb;}event=nd_client_update_art_quiet(&dock,"COLOUR",hex,"colour",0,elapsed,250);if(event&ND_EVENT_QUIT)break;if(event&ND_EVENT_SETTINGS){if(panel){nd_colour_hex(nd_panel_get(panel,0),&j.rgb);strncpy(j.name,nd_panel_get(panel,2),31);j.name[31]=0;}copy_option(&j,argv[1]);if(panel)update_panel(panel,&j,status,0);}if(event&ND_EVENT_CLICK){if(panel)nd_panel_raise(panel);else panel=details(&j,status);}
  if(countdown){if(countdown>elapsed)countdown-=elapsed;else{countdown=0;if(!task.port)nd_task_start(&task,argv[0],"NovaDock.Colour.Worker",1,&j,sizeof(j));}if(panel){char title[96];sprintf(title,"Point at a Workbench colour - sampling in %u s",(countdown+49)/50);nd_panel_title(panel,title);}}
  action=nd_panel_event(panel);if(action==-1){nd_panel_close(panel);panel=NULL;countdown=0;}else if((action==1||action>=100)&&panel){if(task.port){nd_panel_set(panel,3,"Wait for the current operation");}else if(!nd_colour_hex(nd_panel_get(panel,0),&j.rgb)){nd_panel_set(panel,3,"Enter #RRGGBB, using six hexadecimal digits");}else{
    if(action>=101 && countdown){countdown=0;nd_panel_title(panel,"COLOUR - Workbench screen sampler");}
    strncpy(j.name,nd_panel_get(panel,2),31);j.name[31]=0;nd_colour_text(j.rgb,hex,rgb);nd_panel_set(panel,1,rgb);nd_panel_swatch(panel,4,j.rgb);
    if(action==100){countdown=countdown ? 0:150;nd_panel_set(panel,3,countdown ? "Move the pointer; no click needed":"Pick cancelled");if(!countdown)nd_panel_title(panel,"COLOUR - Workbench screen sampler");}
    else if(action==101)nd_task_start(&task,argv[0],"NovaDock.Colour.Worker",2,&j,sizeof(j));
    else if(action==102){candidate=j;if(!candidate.name[0])strcpy(candidate.name,hex);if(nd_colour_add(&candidate.palette,candidate.name,candidate.rgb))nd_task_start(&task,argv[0],"NovaDock.Colour.Worker",4,&candidate,sizeof(candidate));else nd_panel_set(panel,3,"Palette full (32) or invalid name");}
    else if(action==103){if(list)nd_list_raise(list);else {list=nd_list_open("Saved colours",buttons,3);palette_dirty=1;}}
   }}
  if(list&&palette_dirty){for(i=0;i<j.palette.count;++i){sprintf(rows[i],"%-31.31s #%06lX",j.palette.colours[i].name,j.palette.colours[i].rgb);ptr[i]=rows[i];}nd_list_rows(list,ptr,j.palette.count);palette_dirty=0;}action=nd_list_event(list);selected=nd_list_selected(list);
  if(action==-1||action==102){nd_list_close(list);list=NULL;}else if(action==100&&selected>=0&&selected<(int)j.palette.count){j.rgb=j.palette.colours[selected].rgb;strcpy(j.name,j.palette.colours[selected].name);if(!panel)panel=details(&j,"Saved colour selected");else update_panel(panel,&j,"Saved colour selected",1);}
  else if(action==101&&!task.port&&selected>=0&&selected<(int)j.palette.count){candidate=j;if(nd_colour_remove(&candidate.palette,selected))nd_task_start(&task,argv[0],"NovaDock.Colour.Worker",4,&candidate,sizeof(candidate));}
  nd_panel_button(panel,0,countdown ? "Cancel pick":"Pick (3s)",!task.port);
  elapsed=nd_client_wait_signals(&dock,countdown||task.port ? 5:250,nd_panel_signals(panel)|nd_list_signals(list)|nd_task_signals(&task));
 }nd_task_close(&task);nd_list_close(list);nd_panel_close(panel);nd_client_close(&dock);return 0;
}
