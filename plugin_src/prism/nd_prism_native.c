#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <devices/sana2.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "nd_prism.h"
/* SANA-II Revision 7 wireless command/tag assignments. */
#define ND_S2_GETNETWORKS 0xc011
#define ND_S2_SSID (TAG_USER+0)
#define ND_S2_CHANNEL (TAG_USER+7)
#define ND_S2_SIGNAL (TAG_USER+8)
static ULONG tag_value(const struct TagItem *tag,ULONG key,ULONG fallback)
{
 unsigned int limit=0;
 while(tag && limit++<128){
  if(tag->ti_Tag==TAG_DONE)break;
  if(tag->ti_Tag==TAG_MORE){tag=(const struct TagItem *)tag->ti_Data;continue;}
  if(tag->ti_Tag==TAG_SKIP){if(tag->ti_Data>128)return fallback;tag+=tag->ti_Data+1;continue;}
  if(tag->ti_Tag==key)return tag->ti_Data;++tag;
 }
 return fallback;
}
int nd_prism_scan(unsigned long unit,struct NDPrismScan *scan)
{
 struct MsgPort *port=NULL;struct IOSana2Req *io=NULL;APTR pool=NULL;
 struct TagItem params[]={{TAG_DONE,0}};struct TagItem **results;
 const char *ssid;char name[33];unsigned int i,j;int opened=0,result=ND_PRISM_OPEN_FAILED;
 if(!scan || unit>255)return ND_PRISM_INVALID;memset(scan,0,sizeof(*scan));
 port=CreateMsgPort();if(!port)goto finished;
 io=(struct IOSana2Req *)CreateIORequest(port,sizeof(*io));if(!io)goto finished;
 io->ios2_BufferManagement=NULL;
 if(OpenDevice("prism2.device",unit,(struct IORequest *)io,0) &&
    OpenDevice("DEVS:Networks/prism2.device",unit,(struct IORequest *)io,0))goto finished;
 opened=1;result=ND_PRISM_SCAN_FAILED;pool=CreatePool(MEMF_PUBLIC|MEMF_CLEAR,4096,2048);if(!pool)goto finished;
 io->ios2_Req.io_Command=ND_S2_GETNETWORKS;io->ios2_Data=pool;io->ios2_StatData=params;
 /* DoIO may wait for hardware; this function runs only in the worker. No
  * S2_ONLINE/OFFLINE/CONFIGINTERFACE is issued while scanning. */
 if(DoIO((struct IORequest *)io) || io->ios2_Req.io_Error)goto finished;
 results=(struct TagItem **)io->ios2_StatData;if(!results && io->ios2_DataLength)goto finished;
 for(i=0;i<io->ios2_DataLength && i<256;++i){
  ssid=(const char *)tag_value(results[i],ND_S2_SSID,0);if(!ssid)continue;
  for(j=0;j<32 && ssid[j];++j)name[j]=ssid[j];name[j]=0;
  if(j==32 && ssid[j])continue;
  nd_prism_add_network(scan,name,(LONG)tag_value(results[i],ND_S2_SIGNAL,(ULONG)-200),tag_value(results[i],ND_S2_CHANNEL,0));
 }
 result=ND_PRISM_OK;
finished:
 if(opened)CloseDevice((struct IORequest *)io);if(pool)DeletePool(pool);
 if(io)DeleteIORequest((struct IORequest *)io);if(port)DeleteMsgPort(port);return result;
}
static void yield_cpu(void){Delay(1);}
int nd_prism_save(const char *ssid,const char *password,int open_network)
{
 static const char *paths[]={"ENVARC:Sys/Wireless.prefs","ENV:Sys/Wireless.prefs"};
 return nd_prism_save_paths(paths,ssid,password,open_network,yield_cpu);
}
