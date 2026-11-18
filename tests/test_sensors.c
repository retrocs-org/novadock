#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_sensors.h"
struct Registers {unsigned int version,temperature;int reads;};
static unsigned int read16(void *context,unsigned int offset)
{struct Registers *r=context;++r->reads;switch(offset){case 0xc0:return r->version;case 0xe0:return r->temperature;case 0xe2:return 180;case 0xe4:return 100;default:assert(0);return 0;}}
int main(void)
{
 struct NDSensors set,copy;struct Registers regs;long value;char text[100],other[100];FILE *f;int i;
 assert(nd_sensor_status_path("SENSORS",text,sizeof(text)) && nd_sensor_status_path("SENSORS_2",other,sizeof(other)) && strcmp(text,other));
 assert(!nd_sensor_status_path("../Other",text,sizeof(text)) && !nd_sensor_status_path("SENSORS",text,5));
 memset(&set,0,sizeof(set));assert(nd_sensor_number(" -0.5 C ",&value) && value==-500);
 assert(nd_sensor_number("4.92",&value) && value==4920);assert(nd_sensor_number(" 35,7 deg C\n",&value) && value==35700);
 assert(!nd_sensor_number("NaN",&value) && !nd_sensor_number("Inf",&value) && !nd_sensor_number("23.1junk",&value));
 assert(!nd_sensor_number("99999999999999999",&value) && !nd_sensor_number(".5",&value) && !nd_sensor_number("1.",&value));
 assert(nd_sensor_put(&set,0,-500,100));nd_sensor_format(0,set.value[0],text);assert(!strcmp(text,"-0.5 C"));
 assert(nd_sensor_put(&set,1,1000,100));nd_sensor_format(1,1000,text);assert(!strcmp(text,"1.00 V"));
 assert(!nd_sensor_put(&set,1,-1,100) && !nd_sensor_put(&set,0,999000,100));
 assert(!nd_sensor_put(&set,6,101000,100) && !nd_sensor_put(&set,14,0,100));
 assert(nd_sensor_warp_line(&set," MC680x0 int. temp. : 52.6 C",100));
 assert(nd_sensor_warp_line(&set,"Backup batt. voltage = 3.02 V",100));
 assert(nd_sensor_warp_line(&set,"Fan current output: 45%",100));
 assert(!nd_sensor_warp_line(&set,"WiFi password: 1234",100));
 assert(!nd_sensor_warp_line(&set,"ARM STM32H7 temp.: N/A",100));
 assert(!nd_sensor_warp_line(&set,"MC680x0 int. temp.extra: 54 C",100));
 assert(!nd_sensor_warp_line(&set,"5V supply: 99.1 V",100));
 /* Real Warp1260 diagnostic rows use dot leaders and colour controls. */
 assert(nd_sensor_warp_line(&set," MC680x0 int. temp....... : 38.4 \23333m\260C\2330m\n",100));
 assert(set.value[nd_sensor_index("WARP_CPU")]==38400);
 assert(nd_sensor_warp_line(&set," MC680x0 ext. temp....... : 35.3 \260 C",100));
 assert(nd_sensor_warp_line(&set," ARM STM32H7 temp........ : \033[34m56.6 \260C\033[0m",100));
 assert(nd_sensor_warp_line(&set," Artix FPGA temp......... : 75 \260C",100));
 assert(nd_sensor_warp_line(&set," 5V supply............... : 4.82 V",100));
 assert(nd_sensor_warp_line(&set," Backup batt. voltage.... : 3.27 V",100));
 assert(nd_sensor_warp_line(&set," Fan current output...... : 8 %",100));
 assert((set.available&0x3f80UL)==0x3f80UL);
 assert(set.value[nd_sensor_index("WARP_ARM")]==56600);
 assert(set.value[nd_sensor_index("WARP_FAN")]==8000);
 assert(!nd_sensor_warp_line(&set,"WiFi password........... : 1234",100));
 assert(!nd_sensor_warp_line(&set,"MC680x0 int. temp.extra : 54 C",100));
 assert(!nd_sensor_warp_line(&set,"5V supply........ : \033[34",100));
 assert(!nd_sensor_warp_line(&set,"5V supply........ : 4.8 V garbage",100));
 assert(!nd_sensor_warp_line(&set,"5V supply........ 4.8 V",100));
 regs.temperature=0xfff4;
 regs.version=0x0106;regs.reads=0;assert(!nd_sensor_zz_sample(&set,read16,&regs,100) && regs.reads==1);
 regs.version=0xffff;regs.reads=0;assert(!nd_sensor_zz_sample(&set,read16,&regs,100) && regs.reads==1);
 regs.version=0x0107;regs.reads=0;assert(nd_sensor_zz_sample(&set,read16,&regs,100) && regs.reads==4);
 assert(set.value[0]==-1200 && set.value[1]==1000 && set.value[2]==1800);
 /* Official 1.13 encodes decimal minor 13 as 0x0d, not BCD 0x13. */
 regs.version=0x010d;regs.temperature=523;regs.reads=0;
 assert(nd_sensor_zz_sample(&set,read16,&regs,100) && regs.reads==4);
 assert(set.value[0]==52300 && set.value[1]==1000 && set.value[2]==1800);
 assert(!strcmp(nd_sensor_defs[0].label,"ZZ9000 Zynq temperature"));
 f=tmpfile();assert(f && nd_sensor_snapshot_write(f,&set,100));rewind(f);assert(nd_sensor_snapshot_read(f,&copy,110,120) && copy.available==set.available);fclose(f);
 for(i=0;i<ND_SENSOR_COUNT;++i)if(set.available&(1UL<<i))assert(copy.value[i]==set.value[i] && copy.stamp[i]==set.stamp[i]);
 nd_sensor_expire(&copy,221,120);assert(!copy.available);
 f=tmpfile();assert(f);fputs("NDSENS2 100 1\nZZ_TEMP 50000 100\nZZ_TEMP 60000 100\n",f);rewind(f);assert(!nd_sensor_snapshot_read(f,&copy,100,120));fclose(f);
 f=tmpfile();assert(f);fputs("NDSENS2 100 3\nZZ_TEMP 50000 100\n",f);rewind(f);assert(!nd_sensor_snapshot_read(f,&copy,100,120));fclose(f);
 f=tmpfile();assert(f);fputs("NDSENS2 100 1\nZZ_TEMP 50000 100\n",f);rewind(f);assert(!nd_sensor_snapshot_read(f,&copy,99,120));rewind(f);assert(!nd_sensor_snapshot_read(f,&copy,300,120));fclose(f);
 puts("Sensors: firmware-gated read-only ZZ registers, Warp parsing, bounds, formatting, stale readings and strict snapshots passed.");return 0;
}
