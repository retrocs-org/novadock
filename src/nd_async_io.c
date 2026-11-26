/* Independent disk/decoder/launch worker; never holds a dock or screen pointer. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <datatypes/datatypes.h>
#include <datatypes/pictureclass.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <string.h>
#include <stdio.h>
#include "nd_async_io.h"
#include "nd_task.h"
#include "nd_interaction.h"
struct Library *IconBase,*WorkbenchBase,*DataTypesBase;
struct GfxBase *GfxBase;
extern int query_volumes(unsigned int,void *,unsigned int,const char *);
static void fit_picture(int w,int h,int *dw,int *dh)
{*dw=w;*dh=h;if(*dw>96){*dh=(int)((unsigned long)h*96UL/(unsigned long)w);*dw=96;}if(*dh>96){*dw=(int)((unsigned long)*dw*96UL/(unsigned long)*dh);*dh=96;}if(*dw<1)*dw=1;if(*dh<1)*dh=1;}
static struct NDIconPicture *decode_picture(const char *path)
{
 Object *object;struct NDIconPicture *out=NULL;struct BitMapHeader *header=NULL;struct BitMap *bitmap=NULL;
 struct ColorRegister *palette=NULL;PLANEPTR mask=NULL;struct RastPort rp;ULONG colors=0;int x,y,sx,sy,w,h,c;
 if(!DataTypesBase || !GfxBase)return NULL;
 object=NewDTObject((STRPTR)path,DTA_SourceType,DTST_FILE,DTA_GroupID,GID_PICTURE,PDTA_Remap,FALSE,TAG_DONE);if(!object)return NULL;
 if(!DoDTMethod(object,NULL,NULL,DTM_PROCLAYOUT,NULL,TRUE) || !GetDTAttrs(object,PDTA_BitMapHeader,(ULONG)&header,PDTA_BitMap,(ULONG)&bitmap,PDTA_ColorRegisters,(ULONG)&palette,PDTA_NumColors,(ULONG)&colors,PDTA_MaskPlane,(ULONG)&mask,TAG_DONE) || !header || !bitmap || !palette || !colors || colors>256 || bitmap->Depth>8 || !header->bmh_Width || !header->bmh_Height)goto done;
 w=header->bmh_Width;h=header->bmh_Height;out=AllocVec(sizeof(*out),MEMF_PUBLIC|MEMF_CLEAR);if(!out)goto done;
 fit_picture(w,h,&x,&y);out->width=x;out->height=y;out->colors=(unsigned short)colors;
 for(c=0;c<(int)colors;++c){out->rgb[c][0]=palette[c].red;out->rgb[c][1]=palette[c].green;out->rgb[c][2]=palette[c].blue;}
 InitRastPort(&rp);rp.BitMap=bitmap;WaitBlit();
 for(y=0;y<out->height;++y)for(x=0;x<out->width;++x){unsigned int at=y*out->width+x;sx=x*w/out->width;sy=y*h/out->height;c=ReadPixel(&rp,sx,sy);out->pixels[at]=(unsigned char)c;out->opaque[at]=(unsigned char)(c>=0 && c<(int)colors && (!mask || (mask[sy*((w+15)/16)*2+sx/8]&(0x80U>>(sx&7)))));}
 done:DisposeDTObject(object);return out;
}
static struct DiskObject *read_icon(const char *path,unsigned int fallback,int classic_new)
{
 struct DiskObject *icon=NULL;
 if(!IconBase)return NULL;
 if(path && *path){if(IconBase->lib_Version>=44){struct TagItem tags[]={{ICONGETA_GetPaletteMappedIcon,TRUE},{ICONGETA_RemapIcon,FALSE},{ICONGETA_Screen,0},{ICONGETA_GenerateImageMasks,TRUE},{TAG_DONE,0}};icon=GetIconTagList((STRPTR)path,tags);}else if(classic_new && IconBase->lib_Version>=37)icon=GetDiskObjectNew((STRPTR)path);else icon=GetDiskObject((STRPTR)path);}
 if(!icon && fallback)icon=GetDefDiskObject(fallback);return icon;
}
static int load_icon(struct NDIOJob *j)
{
 size_t n=strlen(j->path);j->icon=NULL;j->brush=NULL;j->decoded=NULL;
 if(n>5 && !strcmp(j->path+n-5,".info"))j->path[n-5]=0;
 if(j->iff){j->brush=AllocVec(sizeof(*j->brush),MEMF_PUBLIC|MEMF_CLEAR);if(j->brush && !nd_ilbm_load(j->path,j->brush)){FreeVec(j->brush);j->brush=NULL;}}
 else if(j->picture)j->decoded=decode_picture(j->path);
 else{
  if(j->preferences){BPTR l=Lock(j->path,ACCESS_READ);if(l)UnLock(l);else{strcpy(j->path,"NovaDockPrefs:NovaDockPrefs");l=Lock(j->path,ACCESS_READ);if(l)UnLock(l);else strcpy(j->path,"SYS:Prefs/NovaDockPrefs");}}
  j->icon=read_icon(j->path,0,j->fallback!=WBDISK);
 }
 if(!j->icon && !j->brush && !j->decoded)j->icon=read_icon(NULL,j->fallback,0);
 return j->icon || j->brush || j->decoded ? 1:0;
}
static int launch(struct NDIOJob *j)
{
 char command[ND_COMMAND_LEN*4+64],quoted[ND_COMMAND_LEN*2+4],leaf[ND_COMMAND_LEN+6];
 BPTR lock,in,out;int exists=0,object=0;ULONG stack=16384;LONG rc;struct DiskObject *icon;
 if(!strchr(j->path,'"') && !strchr(j->path,'\n') && !strchr(j->path,'\r')){
  lock=Lock(j->path,ACCESS_READ);if(lock){exists=object=1;UnLock(lock);}else if(strlen(j->path)+5<sizeof(leaf)){strcpy(leaf,j->path);strcat(leaf,".info");lock=Lock(leaf,ACCESS_READ);if(lock){object=1;UnLock(lock);}}
 }
 if(j->mode==ND_LAUNCH_WORKBENCH || (j->mode==ND_LAUNCH_AUTO && object && !j->arguments[0] && !j->directory[0])){
  if(WorkbenchBase && WorkbenchBase->lib_Version>=44 && OpenWorkbenchObject(j->path,TAG_DONE))return 1;if(!exists || strchr(j->path,'"'))return 0;
 }
 if(exists && IconBase){icon=GetDiskObject(j->path);if(icon){if(icon->do_Type==WBTOOL && icon->do_StackSize>(LONG)stack)stack=icon->do_StackSize;FreeDiskObject(icon);}}
 if(stack>1048576UL)stack=1048576UL;sprintf(command,"Stack %lu\n",stack);
 if(j->directory[0]){lock=Lock(j->directory,ACCESS_READ);if(!lock)return 0;UnLock(lock);if(!nd_quote_path(quoted,sizeof(quoted),j->directory))return 0;strcat(command,"CD ");strcat(command,quoted);strcat(command,"\n");}
 if(exists){if(!nd_quote_path(quoted,sizeof(quoted),j->path))return 0;}else{if(strlen(j->path)>=sizeof(quoted))return 0;strcpy(quoted,j->path);}
 if(strlen(command)+strlen(quoted)+strlen(j->arguments)+2>=sizeof(command))return 0;strcat(command,quoted);if(j->arguments[0]){strcat(command," ");strcat(command,j->arguments);}
 in=Open("NIL:",MODE_OLDFILE);out=Open("NIL:",MODE_NEWFILE);if(!in || !out){if(in)Close(in);if(out)Close(out);return 0;}
 rc=SystemTags(command,SYS_Input,in,SYS_Output,out,SYS_Asynch,TRUE,NP_StackSize,stack,TAG_DONE);if(rc<0){Close(in);Close(out);return 0;}return 1;
}
extern int nd_query_folder(struct NDFolderBatch *batch);
static int perform(unsigned int action,void *data,unsigned int bytes,const char *parent)
{
 struct NDIOJob *j=data;int result;
 if(action==1)return query_volumes(action,data,bytes,parent);
 if(action==ND_IO_FOLDER){struct NDFolderBatch *batch=data;if(bytes!=sizeof(*batch) || !memchr(batch->path,0,sizeof(batch->path)))return -1;return nd_query_folder(batch);}
 if(bytes!=sizeof(*j) || !memchr(j->path,0,sizeof(j->path)) || !memchr(j->arguments,0,sizeof(j->arguments)) || !memchr(j->directory,0,sizeof(j->directory)))return -1;
 if(action==ND_IO_ICON || action==ND_IO_LAUNCH || action==ND_IO_FOLDER_ICON)IconBase=OpenLibrary("icon.library",36);else return -1;
 if(action==ND_IO_FOLDER_ICON){j->icon=IconBase ? GetDiskObject(j->path):NULL;j->error=j->icon ? 0:(unsigned long)IoErr();return j->icon!=NULL;}
 if(action==ND_IO_ICON){GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);DataTypesBase=OpenLibrary("datatypes.library",39);result=load_icon(j);}
 else{WorkbenchBase=OpenLibrary("workbench.library",36);result=launch(j);}
 j->error=result ? 0:(unsigned long)IoErr();return result;
}
int nd_background_worker(const char *parent)
{
 int result;
 result=nd_task_worker_owned(parent,perform,nd_io_dispose);
 if(DataTypesBase)CloseLibrary(DataTypesBase);if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(WorkbenchBase)CloseLibrary(WorkbenchBase);if(IconBase)CloseLibrary(IconBase);return result;
}
