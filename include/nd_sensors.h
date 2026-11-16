#ifndef ND_SENSORS_H
#define ND_SENSORS_H
#include <stdio.h>
#define ND_SENSOR_COUNT 14
#define ND_SENSOR_ALL ((1UL<<ND_SENSOR_COUNT)-1UL)
#define ND_SENSOR_DIRECTORY "ENV:NovaDock/Sensors"
#define ND_SENSOR_WARP_STATUS ND_SENSOR_DIRECTORY "/Warp.status"
#define ND_SENSOR_CHANNELS(X) \
 X(ZZ_TEMP,"ZZ9000 FPGA temperature","ZZ FPGA","C",-60000,200000) \
 X(ZZ_CORE,"ZZ9000 core voltage","ZZ core","V",0,20000) \
 X(ZZ_AUX,"ZZ9000 auxiliary voltage","ZZ aux","V",0,20000) \
 X(RTC_TEMP,"RTC internal temperature","RTC temp","C",-60000,200000) \
 X(RTC_CPU,"RTC external CPU probe","CPU probe","C",-60000,200000) \
 X(RTC_5V,"RTC 5V supply","RTC 5V","V",0,20000) \
 X(RTC_BATT,"RTC battery charge","RTC batt","%",0,100000) \
 X(WARP_CPU,"Warp CPU internal temperature","Warp CPU","C",-60000,200000) \
 X(WARP_EXT,"Warp CPU external temperature","Warp ext","C",-60000,200000) \
 X(WARP_ARM,"Warp ARM temperature","Warp ARM","C",-60000,200000) \
 X(WARP_FPGA,"Warp FPGA temperature","Warp FPGA","C",-60000,200000) \
 X(WARP_5V,"Warp 5V supply","Warp 5V","V",0,20000) \
 X(WARP_BATT,"Warp battery voltage","Warp batt","V",0,20000) \
 X(WARP_FAN,"Warp fan output","Warp fan","%",0,100000)
struct NDSensorDef {const char *key,*label,*short_label,*unit;long minimum,maximum;};
extern const struct NDSensorDef nd_sensor_defs[ND_SENSOR_COUNT];
struct NDSensors {unsigned long available,sampled,stamp[ND_SENSOR_COUNT];long value[ND_SENSOR_COUNT];};
int nd_sensor_index(const char *key);
int nd_sensor_number(const char *text,long *milli);
int nd_sensor_put(struct NDSensors *set,int index,long milli,unsigned long now);
void nd_sensor_expire(struct NDSensors *set,unsigned long now,unsigned long max_age);
void nd_sensor_format(int index,long milli,char *out);
int nd_sensor_zz_sample(struct NDSensors *set,unsigned int (*read16)(void *,unsigned int),void *context,unsigned long now);
int nd_sensor_warp_line(struct NDSensors *set,const char *line,unsigned long now);
int nd_sensor_snapshot_read(FILE *file,struct NDSensors *set,unsigned long now,unsigned long max_age);
int nd_sensor_snapshot_write(FILE *file,const struct NDSensors *set,unsigned long now);
int nd_sensor_status_path(const char *id,char *out,unsigned int size);
unsigned long nd_sensor_available_for(const char *id,unsigned long now);
#endif
