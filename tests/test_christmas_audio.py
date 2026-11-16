#!/usr/bin/env python3
"""Exercise actual bounded MOD streaming and device ownership with fake DMA."""
from pathlib import Path
import json,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
s=(ROOT/'plugin_src/christmas/nd_christmas_audio.h').read_text()
for include in ('<devices/audio.h>','<exec/memory.h>','<clib/alib_protos.h>'):s=s.replace('#include '+include,'')
h=r"""#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char UBYTE;typedef unsigned short UWORD;
#define MEMF_CHIP 2
#define MEMF_PUBLIC 1
#define ADALLOC_MINPREC -128
#define ADIOF_NOWAIT 64
#define ADIOF_PERVOL 16
#define AUDIONAME "audio.device"
#define CMD_WRITE 3
#define NT_MESSAGE 5
struct Node {int ln_Pri,ln_Type;void *ln_Succ,*ln_Pred;};struct Message {struct Node mn_Node;};
struct IORequest {struct Message io_Message;int io_Flags,io_Command,io_Error;};
struct IOAudio {struct IORequest ioa_Request;int ioa_AllocKey;UBYTE *ioa_Data;unsigned long ioa_Length;UWORD ioa_Period,ioa_Volume,ioa_Cycles;};
struct MsgPort {int mp_SigBit;};
static int ports,requests,buffers,opens,closes,sends,aborts,busy;static size_t chip,peak,pcm_length,cursor;
static unsigned char *pcm;static struct IORequest *active[2];static struct Message *replies[8];static int replies_count;
static struct MsgPort *CreateMsgPort(void){struct MsgPort *p=calloc(1,sizeof(*p));++ports;p->mp_SigBit=4;return p;}
static void DeleteMsgPort(struct MsgPort *p){assert(!replies_count);--ports;free(p);}
static void *CreateIORequest(struct MsgPort *p,unsigned long size){assert(p&&size==sizeof(struct IOAudio));++requests;return calloc(1,size);}
static void DeleteIORequest(struct IORequest *r){--requests;free(r);}
static void *AllocVec(unsigned long n,int flags){assert(n==16384&&(flags&MEMF_CHIP));++buffers;chip+=n;if(chip>peak)peak=chip;return malloc(n);}
static void FreeVec(void *p){--buffers;chip-=16384;free(p);}
static int OpenDevice(const char *name,int unit,struct IORequest *r,int flags){struct IOAudio *a=(struct IOAudio *)r;(void)flags;assert(!strcmp(name,AUDIONAME)&&!unit&&r->io_Message.mn_Node.ln_Pri==-128&&r->io_Flags==ADIOF_NOWAIT&&a->ioa_Length==4&&!a->ioa_AllocKey);++opens;return busy;}
static void CloseDevice(struct IORequest *r){assert(r);++closes;}
static struct Message *GetMsg(struct MsgPort *p){struct Message *m;int i;assert(p);if(!replies_count)return NULL;m=replies[0];for(i=1;i<replies_count;++i)replies[i-1]=replies[i];--replies_count;return m;}
static void completed(struct IORequest *r,int error){int i;for(i=0;i<2;++i)if(active[i]==r)active[i]=NULL;r->io_Error=error;assert(replies_count<8);replies[replies_count++]=(struct Message *)r;}
static void AbortIO(struct IORequest *r){++aborts;completed(r,-2);}
static void WaitPort(struct MsgPort *p){assert(p&&replies_count);}
static void BeginIO(struct IORequest *r){struct IOAudio *a=(struct IOAudio *)r;size_t i;int slot;
 assert(r->io_Command==CMD_WRITE&&r->io_Flags==ADIOF_PERVOL&&r->io_Message.mn_Node.ln_Type==NT_MESSAGE&&!r->io_Error);
 assert(a->ioa_Length==16384&&a->ioa_Period==256&&a->ioa_Volume<=64&&a->ioa_Cycles==1);
 for(i=0;i<a->ioa_Length;++i){assert(a->ioa_Data[i]==pcm[cursor]);cursor=(cursor+1)%pcm_length;}
 for(slot=0;slot<2;++slot)if(!active[slot]){active[slot]=r;break;}assert(slot<2);++sends;
}
"""
h+='\n#define XMAS_MUSIC_OPEN() fopen('+json.dumps(str(ROOT/'Music/WinteryLoop.mod'))+',"rb")\n'+s+r"""
int main(void){struct NDXmasAudio a;struct NDXmasModule mod;FILE *f;int i,old;
 memset(&a,0,sizeof(a));f=XMAS_MUSIC_OPEN();assert(f&&nd_xmas_module(f,&mod));pcm_length=mod.length;pcm=malloc(pcm_length);assert(pcm);assert(!fseek(f,(long)mod.offset,SEEK_SET)&&fread(pcm,1,pcm_length,f)==pcm_length);fclose(f);
 xmas_audio_update(&a,0,30,500);assert(!opens&&!sends&&!ports);
 xmas_audio_update(&a,1,30,0);assert(opens==1&&sends==2&&a.pending[0]&&a.pending[1]&&xmas_audio_signals(&a)==16);
 for(i=0;i<100;++i){completed((struct IORequest *)a.request[i%2],0);xmas_audio_update(&a,1,30,0);}assert(sends==102&&peak==32768&&a.opened);
 xmas_audio_update(&a,0,30,0);assert(!a.file&&!ports&&!requests&&!buffers&&!chip&&closes==1&&!replies_count);
 busy=1;old=opens;xmas_audio_update(&a,1,30,0);xmas_audio_update(&a,1,30,500);assert(opens==old+1&&!a.opened&&!ports&&!requests&&!buffers);
 xmas_audio_update(&a,0,30,0);busy=0;cursor=0;xmas_audio_update(&a,1,100,0);assert(a.opened);
 completed((struct IORequest *)a.request[1],-12);xmas_audio_update(&a,1,100,0);assert(!a.opened&&!ports&&!requests&&!buffers&&!replies_count);
 xmas_audio_update(&a,0,30,0);xmas_audio_update(&a,1,0,0);assert(!ports);free(pcm);return 0;}
"""
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.c').write_text(h)
 subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-I'+str(ROOT/'include'),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
assert 'SendIO(' not in s and 'WaitIO(' not in s
print('MOD audio: actual recording bytes, 100 streamed blocks/loops, 32 KB Chip RAM bound, BeginIO flags and close/error/mute cleanup passed.')
