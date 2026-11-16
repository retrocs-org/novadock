#include <devices/audio.h>
#include <exec/memory.h>
#include <clib/alib_protos.h>
#include "nd_christmas_music.h"
struct NDXmasAudio {
 struct MsgPort *port;struct IOAudio *request[2];UBYTE *wave[2];FILE *file;
 struct NDXmasModule module;unsigned long cursor;int opened,pending[2],attempted,volume;
};
#ifndef XMAS_MUSIC_OPEN
static FILE *xmas_music_open(void)
{
 BPTR parent,dir;char path[320];FILE *file=NULL;
 dir=GetProgramDir();parent=dir ? ParentDir(dir):0;
 if(parent){if(NameFromLock(parent,path,sizeof(path))&&AddPart(path,"Music/WinteryLoop.mod",sizeof(path)))file=fopen(path,"rb");UnLock(parent);}
 if(!file)file=fopen("NovaDock:Music/WinteryLoop.mod","rb");return file;
}
#define XMAS_MUSIC_OPEN() xmas_music_open()
#endif
static void xmas_audio_replies(struct NDXmasAudio *a)
{struct Message *m;int i;if(!a->port)return;while((m=GetMsg(a->port))!=NULL)for(i=0;i<2;++i)if(m==(struct Message *)a->request[i])a->pending[i]=0;}
static void xmas_audio_close(struct NDXmasAudio *a)
{
 int i;xmas_audio_replies(a);
 for(i=0;i<2;++i)if(a->pending[i])AbortIO((struct IORequest *)a->request[i]);
 while(a->pending[0]||a->pending[1]){WaitPort(a->port);xmas_audio_replies(a);}
 if(a->opened)CloseDevice((struct IORequest *)a->request[0]);
 for(i=0;i<2;++i){if(a->request[i])DeleteIORequest((struct IORequest *)a->request[i]);if(a->wave[i])FreeVec(a->wave[i]);}
 if(a->port)DeleteMsgPort(a->port);if(a->file)fclose(a->file);memset(a,0,sizeof(*a));
}
static int xmas_audio_fill(struct NDXmasAudio *a,int index)
{
 unsigned int left=ND_XMAS_AUDIO_BLOCK,take,at=0;
 while(left){
  if(a->cursor==a->module.length)a->cursor=0;
  take=a->module.length-a->cursor<left ? (unsigned int)(a->module.length-a->cursor):left;
  if(fseek(a->file,(long)(a->module.offset+a->cursor),SEEK_SET)||fread(a->wave[index]+at,1,take,a->file)!=take)return 0;
  a->cursor+=take;left-=take;at+=take;
 }
 a->request[index]->ioa_Request.io_Command=CMD_WRITE;
 a->request[index]->ioa_Request.io_Flags=ADIOF_PERVOL;
 a->request[index]->ioa_Request.io_Message.mn_Node.ln_Type=NT_MESSAGE;
 a->request[index]->ioa_Request.io_Error=0;
 a->request[index]->ioa_Data=a->wave[index];a->request[index]->ioa_Length=ND_XMAS_AUDIO_BLOCK;
 a->request[index]->ioa_Period=ND_XMAS_AUDIO_PERIOD;a->request[index]->ioa_Volume=(UWORD)((a->volume*64+50)/100);a->request[index]->ioa_Cycles=1;
 BeginIO((struct IORequest *)a->request[index]);a->pending[index]=1;return 1;
}
static int xmas_audio_open(struct NDXmasAudio *a,int volume)
{
 static UBYTE channels[4]={1,2,4,8};int i;
 a->attempted=1;a->volume=volume;a->file=XMAS_MUSIC_OPEN();
 if(!a->file||!nd_xmas_module(a->file,&a->module))goto failed;
 a->port=CreateMsgPort();if(!a->port)goto failed;
 for(i=0;i<2;++i){a->request[i]=(struct IOAudio *)CreateIORequest(a->port,sizeof(*a->request[i]));a->wave[i]=AllocVec(ND_XMAS_AUDIO_BLOCK,MEMF_CHIP|MEMF_PUBLIC);if(!a->request[i]||!a->wave[i])goto failed;}
 a->request[0]->ioa_Request.io_Message.mn_Node.ln_Pri=ADALLOC_MINPREC;
 a->request[0]->ioa_Request.io_Flags=ADIOF_NOWAIT;
 a->request[0]->ioa_AllocKey=0;a->request[0]->ioa_Data=channels;a->request[0]->ioa_Length=4;
 if(OpenDevice(AUDIONAME,0,(struct IORequest *)a->request[0],0))goto failed;
 a->opened=1;*a->request[1]=*a->request[0];
 a->request[1]->ioa_Request.io_Message.mn_Node.ln_Succ=NULL;a->request[1]->ioa_Request.io_Message.mn_Node.ln_Pred=NULL;
 if(!xmas_audio_fill(a,0)||!xmas_audio_fill(a,1))goto failed;return 1;
failed:xmas_audio_close(a);a->attempted=1;return 0;
}
static void xmas_audio_update(struct NDXmasAudio *a,int active,int volume,unsigned int elapsed)
{
 struct Message *m;int i;
 if(elapsed){} /* timing is driven by completed audio blocks */
 if(!active||volume<=0){xmas_audio_close(a);return;}
 if(volume>100)volume=100;
 if(!a->attempted){(void)xmas_audio_open(a,volume);return;}
 if(!a->opened)return;a->volume=volume;
 while((m=GetMsg(a->port))!=NULL){
  for(i=0;i<2;++i)if(m==(struct Message *)a->request[i])break;
  if(i==2)continue;a->pending[i]=0;
  if(a->request[i]->ioa_Request.io_Error||!xmas_audio_fill(a,i)){xmas_audio_close(a);a->attempted=1;return;}
 }
}
static unsigned long xmas_audio_signals(const struct NDXmasAudio *a)
{return a->port ? 1UL<<a->port->mp_SigBit:0;}
