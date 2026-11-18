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
int nd_text_copy(const char *text)
{struct MsgPort *port=CreateMsgPort();struct IOClipReq *io=NULL;unsigned char *data=NULL;unsigned long size;int ok=0;
 if(!text || strlen(text)>11000)goto done;if(port)io=(struct IOClipReq *)CreateIORequest(port,sizeof(*io));data=AllocVec(strlen(text)+32,MEMF_PUBLIC);
 if(io && data && !OpenDevice("clipboard.device",0,(struct IORequest *)io,0)){size=nd_clip_encode(text,data,strlen(text)+32);io->io_ClipID=0;io->io_Offset=0;io->io_Command=CMD_WRITE;io->io_Data=data;io->io_Length=size;if(size && !DoIO((struct IORequest *)io)){io->io_Command=CMD_UPDATE;io->io_Data=NULL;io->io_Length=0;ok=!DoIO((struct IORequest *)io);}CloseDevice((struct IORequest *)io);}
 done:if(data)FreeVec(data);if(io)DeleteIORequest((struct IORequest *)io);if(port)DeleteMsgPort(port);return ok;}
int nd_text_export(const char *path,const char *text,unsigned int length)
{char temp[320];struct DateStamp d;BPTR lock,file;int ok;if(!path || !strchr(path,':') || strlen(path)>239 || length>11000)return 0;lock=Lock((STRPTR)path,ACCESS_READ);if(lock){UnLock(lock);return -4;}DateStamp(&d);sprintf(temp,"%s.ndtmp-%08lx-%08lx",path,(ULONG)FindTask(NULL),(ULONG)d.ds_Tick);lock=Lock(temp,ACCESS_READ);if(lock){UnLock(lock);return 0;}file=Open(temp,MODE_NEWFILE);if(!file)return 0;ok=Write(file,(APTR)text,length)==length;if(!Close(file))ok=0;if(ok)ok=Rename(temp,(STRPTR)path)!=0;if(!ok)DeleteFile(temp);return ok;}
