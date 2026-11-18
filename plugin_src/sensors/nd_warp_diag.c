/* Sensor-only decoder for the independently inspected 30.74 library result.
 * Byte loads avoid host endian/alignment/float ABI assumptions. */
#include "nd_warp_diag.h"
static unsigned long be32(const unsigned char *p)
{return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}
int nd_warp_diag_supported(unsigned int major,unsigned int revision)
{return major==30 && revision==74;}
/* Rounded milli-units without an FPU or 64-bit multiply. A 24-bit significand
 * times 125 fits in 31 bits; shifting three fewer bits accounts for *1000. */
static int f32_milli(unsigned long bits,long *out)
{
 unsigned int exp=(unsigned int)((bits>>23)&255UL);unsigned long scaled,n;
 int shift;
 if(exp==255)return 0;
 if(!exp){*out=0;return 1;}
 shift=147-(int)exp;
 if(shift<0)return 0;
 scaled=((bits&0x7fffffUL)|0x800000UL)*125UL;
 if(shift>=32)n=0;
 else if(!shift)n=scaled;
 else n=(scaled>>shift)+((scaled>>(shift-1))&1UL);
 *out=(long)n;if(bits&0x80000000UL)*out=-*out;return 1;
}
static long rounded(long value,long quantum)
{return value<0 ? -(((-value+quantum/2)/quantum)*quantum):((value+quantum/2)/quantum)*quantum;}
int nd_warp_diag_decode(const unsigned char *data,unsigned int size,struct NDSensors *set,unsigned long now)
{
 static const struct {unsigned int offset;const char *key;long quantum;} fields[]={
  {44,"WARP_CPU",100},{48,"WARP_EXT",100},{52,"WARP_ARM",100},
  {60,"WARP_5V",10},{64,"WARP_BATT",10}};
 unsigned int i;unsigned long raw;long milli;int count=0;
 if(!data || size!=ND_WARP_DIAG_SIZE || !set)return 0;
 set->available&=~0x3f80UL;
 for(i=0;i<sizeof(fields)/sizeof(fields[0]);++i)
  if(f32_milli(be32(data+fields[i].offset),&milli))
   count+=nd_sensor_put(set,nd_sensor_index(fields[i].key),rounded(milli,fields[i].quantum),now);
 /* FPGA is signed whole Celsius; fan is the low byte of the wire value. */
 raw=be32(data+56);
 if(raw<=200UL || raw>=0xffffffc4UL){
  milli=raw<=200UL ? (long)raw:-((long)(0xffffffffUL-raw))-1L;
  count+=nd_sensor_put(set,nd_sensor_index("WARP_FPGA"),milli*1000L,now);
 }
 count+=nd_sensor_put(set,nd_sensor_index("WARP_FAN"),(long)data[37]*1000L,now);
 return count;
}
