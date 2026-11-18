/* SPDX-License-Identifier: GPL-3.0-or-later
 * Standalone, read-only i2csensors.library v2 provider. The dock and sensor
 * plugin consume a bounded text cache; they do not link to this library.
 * ABI: upstream i2csensors.doc and public FD, vectors -30/-36/-66.
 */
#include <exec/types.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>
#include "nd_i2c_cache.h"
#include "nd_plugin_versions.h"
long __stack=32768;
#define I2C_BASE 0x812c0000UL
LONG nd_i2c_count(__reg("a6") struct Library *,__reg("d0") ULONG)="\tjsr\t-30(a6)";
LONG nd_i2c_value(__reg("a6") struct Library *,__reg("d0") ULONG,__reg("d1") ULONG,__reg("a1") STRPTR *,__reg("a2") STRPTR *)="\tjsr\t-36(a6)";
ULONG nd_i2c_id(__reg("a6") struct Library *,__reg("d0") ULONG,__reg("d1") ULONG)="\tjsr\t-66(a6)";
const char nd_version[]=ND_PLUGIN_VER_STRING(SENSORI2C,"SensorsI2C");
static struct NDI2CCache cache,previous;
int main(void)
{
 static const ULONG types[]={0x10,0x11,0x12,0x13,0x14,0x16,0x17,0x19};
 struct MsgPort *guard;struct Library *lib;struct DateStamp d;unsigned long now,id;unsigned int t,reads=0,i;LONG n,index,v;STRPTR name,unit;BPTR lock;FILE *f;int ok=0;
 ((struct Process *)FindTask(NULL))->pr_WindowPtr=(APTR)-1;
 guard=CreateMsgPort();if(!guard)return 20;guard->mp_Node.ln_Name="NovaDock.Sensors.I2C";
 Forbid();if(FindPort(guard->mp_Node.ln_Name)){Permit();DeleteMsgPort(guard);return 0;}AddPort(guard);Permit();
 DateStamp(&d);now=(unsigned long)d.ds_Days*86400UL+(unsigned long)d.ds_Minute*60UL+(unsigned long)d.ds_Tick/50UL;
 (void)nd_i2c_load(&previous);cache.stamp=now;cache.provider=0;
 lib=OpenLibrary("i2csensors.library",2);
 if(lib){cache.provider=1;for(t=0;t<sizeof(types)/sizeof(types[0]) && reads<64;++t){
  n=nd_i2c_count(lib,I2C_BASE+types[t]);if(n<0)n=0;if(n>64)n=64;
  for(index=0;index<n && reads<64;++index){++reads;id=nd_i2c_id(lib,I2C_BASE+types[t],(ULONG)index);if(!id)continue;name=unit=NULL;
   v=nd_i2c_value(lib,I2C_BASE+0x18,id,&unit,&name);
   /* Both NULL is the documented read error. Zero is a valid reading. */
   if(name || unit)nd_i2c_add(&cache,id,v,name,unit,now);
  }
 }CloseLibrary(lib);}
 /* Successful current readings take priority over older unavailable devices. */
 for(i=0;i<previous.count && cache.count<ND_I2C_MAX;++i){unsigned int j;for(j=0;j<cache.count;++j)if(cache.readings[j].id==previous.readings[i].id)break;if(j==cache.count){cache.readings[cache.count]=previous.readings[i];cache.readings[cache.count++].available=0;}}
 lock=CreateDir("ENV:NovaDock");if(lock)UnLock(lock);lock=CreateDir("ENV:NovaDock/Sensors");if(lock)UnLock(lock);
 f=fopen(ND_I2C_CACHE ".tmp","w");if(f){ok=nd_i2c_write(f,&cache);if(fclose(f))ok=0;}
 if(ok){DeleteFile(ND_I2C_CACHE ".bak");lock=Lock(ND_I2C_CACHE,ACCESS_READ);if(lock){UnLock(lock);if(!Rename(ND_I2C_CACHE,ND_I2C_CACHE ".bak"))ok=0;}
  if(ok && !Rename(ND_I2C_CACHE ".tmp",ND_I2C_CACHE)){Rename(ND_I2C_CACHE ".bak",ND_I2C_CACHE);ok=0;}}
 if(!ok)DeleteFile(ND_I2C_CACHE ".tmp");Forbid();RemPort(guard);Permit();DeleteMsgPort(guard);return ok ? 0:10;
}
