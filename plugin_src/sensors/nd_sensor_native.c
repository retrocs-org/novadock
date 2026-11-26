/* Register identities/units verified against MNT firmware 1.13 and ZZTop.
 * RTC data uses the documented AK_Sensors 1.3 ENV interface. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <libraries/configvars.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include <string.h>
#include <stdio.h>
#include "nd_sensor_native.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
struct ExpansionBase *ExpansionBase=NULL;
unsigned long nd_sensor_now(void)
{struct DateStamp d;DateStamp(&d);return (unsigned long)d.ds_Days*86400UL+(unsigned long)d.ds_Minute*60UL+(unsigned long)d.ds_Tick/50UL;}
static unsigned int read_zz(void *context,unsigned int offset)
{return *(volatile UWORD *)((volatile UBYTE *)context+offset);}
static void sample_zz(struct NDSensors *set,unsigned long now)
{
 struct ConfigDev *dev;int product;
 set->available&=~7UL;
 ExpansionBase=(struct ExpansionBase *)OpenLibrary("expansion.library",0);if(!ExpansionBase)return;
 for(product=3;product<=4;++product){
  dev=FindConfigDev(NULL,0x6d6e,product);if(!dev || !dev->cd_BoardAddr || dev->cd_BoardSize<0xe6)continue;
  if(nd_sensor_zz_sample(set,read_zz,dev->cd_BoardAddr,now))break;
 }
 CloseLibrary((struct Library *)ExpansionBase);ExpansionBase=NULL;
}
static void sample_rtc(struct NDSensors *set,unsigned long now,unsigned long max_age)
{
 static const char *const variables[]={"AK_Internal_temp_C","AK_CPU_temp_C","AK_5V_supply_V","AK_RTC_battery_percent"};
 int i;char path[80],value[64];long milli;BPTR lock,file;LONG length;struct FileInfoBlock *fib;unsigned long stamp;
 fib=AllocDosObject(DOS_FIB,NULL);if(!fib)return;
 for(i=0;i<4;++i){
  set->available&=~(1UL<<(i+3));sprintf(path,"ENV:%s",variables[i]);lock=Lock(path,ACCESS_READ);if(!lock)continue;
  if(!Examine(lock,fib) || fib->fib_DirEntryType>=0 || fib->fib_Size<=0 || fib->fib_Size>62){UnLock(lock);continue;}
  stamp=(unsigned long)fib->fib_Date.ds_Days*86400UL+(unsigned long)fib->fib_Date.ds_Minute*60UL+(unsigned long)fib->fib_Date.ds_Tick/50UL;UnLock(lock);
  if(stamp>now || now-stamp>max_age)continue;
  file=Open(path,MODE_OLDFILE);if(!file)continue;length=Read(file,value,63);Close(file);if(length<=0 || length>62)continue;value[length]=0;
  if(!memchr(value,0,(size_t)length) && nd_sensor_number(value,&milli))nd_sensor_put(set,i+3,milli,stamp);
 }
 FreeDosObject(DOS_FIB,fib);
}
void nd_sensor_native_sample(struct NDSensors *set,unsigned long now,unsigned long rtc_age)
{struct Process *self=(struct Process *)FindTask(NULL);APTR old=self->pr_WindowPtr;self->pr_WindowPtr=(APTR)-1;sample_zz(set,now);sample_rtc(set,now,rtc_age);self->pr_WindowPtr=old;}
static void publish_file(const char *path,const struct NDSensors *set,unsigned long now)
{
 BPTR lock;FILE *f;int ok,backed=0;char tmp[120],old[120];
 sprintf(tmp,"%s.tmp",path);sprintf(old,"%s.old",path);
 lock=CreateDir("ENV:NovaDock");if(lock)UnLock(lock);lock=CreateDir(ND_SENSOR_DIRECTORY);if(lock)UnLock(lock);
 f=fopen(tmp,"w");if(!f)return;ok=nd_sensor_snapshot_write(f,set,now);if(fclose(f))ok=0;
 if(!ok){remove(tmp);return;}
 lock=Lock((STRPTR)path,ACCESS_READ);if(lock){UnLock(lock);DeleteFile(old);backed=Rename((STRPTR)path,old)!=0;if(!backed){remove(tmp);return;}}
 if(!Rename(tmp,(STRPTR)path)){if(backed)Rename(old,(STRPTR)path);remove(tmp);}else if(backed)DeleteFile(old);
}
void nd_sensor_publish(const char *id,const struct NDSensors *set,unsigned long now)
{char path[100];if(nd_sensor_status_path(id,path,sizeof(path)))publish_file(path,set,now);}
void nd_sensor_publish_warp(const struct NDSensors *set,unsigned long now)
{
 struct NDSensors warp=*set;BPTR lock;
 lock=CreateDir("ENV:NovaDock");if(lock)UnLock(lock);lock=CreateDir(ND_SENSOR_DIRECTORY);if(lock)UnLock(lock);
 lock=CreateDir(ND_SENSOR_DIRECTORY "/Warp.lock");if(!lock)return;UnLock(lock);
 warp.available&=~0x7fUL;publish_file(ND_SENSOR_WARP_STATUS,&warp,now);DeleteFile(ND_SENSOR_DIRECTORY "/Warp.lock");
}
int nd_sensor_import_warp(struct NDSensors *set,unsigned long now)
{
 FILE *f=fopen(ND_SENSOR_WARP_STATUS,"r");struct NDSensors warp;int i,ok;if(!f)return 0;
 ok=nd_sensor_snapshot_read(f,&warp,now,90);fclose(f);if(!ok)return 0;nd_sensor_expire(&warp,now,90);
 set->available&=0x7fUL;for(i=7;i<ND_SENSOR_COUNT;++i)if(warp.available&(1UL<<i))nd_sensor_put(set,i,warp.value[i],warp.stamp[i]);
 return now>=warp.sampled && now-warp.sampled<30;
}
