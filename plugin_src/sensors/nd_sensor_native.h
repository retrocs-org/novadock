#ifndef ND_SENSOR_NATIVE_H
#define ND_SENSOR_NATIVE_H
#include "nd_sensors.h"
unsigned long nd_sensor_now(void);
void nd_sensor_native_sample(struct NDSensors *set,unsigned long now,unsigned long rtc_age);
void nd_sensor_publish(const char *id,const struct NDSensors *set,unsigned long now);
void nd_sensor_publish_warp(const struct NDSensors *set,unsigned long now);
int nd_sensor_import_warp(struct NDSensors *set,unsigned long now);
#endif
