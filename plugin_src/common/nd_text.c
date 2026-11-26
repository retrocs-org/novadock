#include <exec/types.h>
#include <exec/memory.h>
#include <devices/clipboard.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_text.h"
#include "../clipboard/nd_clipboard.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
int nd_text_copy(const char *text)
{struct MsgPort *port=CreateMsgPort();struct IOClipReq *io=NULL;unsigned char *data=NULL;unsigned long size;int ok=0;
 if(!text || strlen(text)>11000)goto done;if(port)io=(struct IOClipReq *)CreateIORequest(port,sizeof(*io));data=AllocVec(strlen(text)+32,MEMF_PUBLIC);
 if(io && data && !OpenDevice("clipboard.device",0,(struct IORequest *)io,0)){size=nd_clip_encode(text,data,strlen(text)+32);io->io_ClipID=0;io->io_Offset=0;io->io_Command=CMD_WRITE;io->io_Data=data;io->io_Length=size;if(size && !DoIO((struct IORequest *)io)){io->io_Command=CMD_UPDATE;io->io_Data=NULL;io->io_Length=0;ok=!DoIO((struct IORequest *)io);}CloseDevice((struct IORequest *)io);}
 done:if(data)FreeVec(data);if(io)DeleteIORequest((struct IORequest *)io);if(port)DeleteMsgPort(port);return ok;}
static unsigned long export_serial;
static int export_problem(LONG error)
{
 if(error==ERROR_DISK_WRITE_PROTECTED||error==ERROR_WRITE_PROTECTED||error==ERROR_DELETE_PROTECTED||error==ERROR_READ_PROTECTED)return ND_TEXT_PROTECTED;
 if(error==ERROR_DISK_FULL)return ND_TEXT_FULL;
 if(error==ERROR_OBJECT_EXISTS)return ND_TEXT_EXISTS;
 if(error==ERROR_OBJECT_NOT_FOUND||error==ERROR_DIR_NOT_FOUND||error==ERROR_OBJECT_WRONG_TYPE||error==ERROR_INVALID_COMPONENT_NAME)return ND_TEXT_PATH;
 return ND_TEXT_IO;
}
const char *nd_text_export_problem(int result)
{
 switch(result){
 case ND_TEXT_EXISTS:return "File already exists; choose a different name";
 case ND_TEXT_PROTECTED:return "Destination is write-protected; choose a writable drawer";
 case ND_TEXT_FULL:return "Destination is full; free space or choose another drawer";
 case ND_TEXT_PATH:return "Invalid destination; use an existing volume and drawer";
 default:return "File could not be saved; check destination and retry";
 }
}
int nd_text_export(const char *path,const char *text,unsigned int length)
{
 char temp[320],*name;struct DateStamp date;BPTR lock,file;LONG error;int i,result=ND_TEXT_IO;
 if(!path||!text||!strchr(path,':')||strlen(path)>239||length>11000||!*FilePart((STRPTR)path))return ND_TEXT_PATH;
 lock=Lock((STRPTR)path,ACCESS_READ);if(lock){UnLock(lock);return ND_TEXT_EXISTS;}
 error=IoErr();if(error!=ERROR_OBJECT_NOT_FOUND)return export_problem(error);
 strcpy(temp,path);name=FilePart(temp);DateStamp(&date);
 /* Same drawer/volume for atomic rename; every temporary component fits FFS. */
 for(i=0;i<8;++i){sprintf(name,"ndt%08lx%08lx.tmp",(ULONG)FindTask(NULL),((ULONG)date.ds_Tick)^++export_serial);lock=Lock(temp,ACCESS_READ);if(lock){UnLock(lock);continue;}error=IoErr();if(error!=ERROR_OBJECT_NOT_FOUND)return export_problem(error);break;}
 if(i==8)return ND_TEXT_IO;
 file=Open(temp,MODE_NEWFILE);if(!file)return export_problem(IoErr());
 if(Write(file,(APTR)text,length)!=(LONG)length){error=IoErr();Close(file);result=export_problem(error);}
 else if(!Close(file))result=export_problem(IoErr());
 else if(!Rename(temp,(STRPTR)path))result=export_problem(IoErr());
 else return 1;
 DeleteFile(temp);return result;
}
