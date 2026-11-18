#ifndef ND_SENSOR_STATUS_H
#define ND_SENSOR_STATUS_H
#include "nd_sensors.h"
#include "nd_i2c_cache.h"
/* Missing/expired values are never formatted as current measurements. */
static const char *nd_sensor_reading_status(int available,unsigned long stamp,unsigned long now,unsigned long age)
{if(stamp && (now<stamp || now-stamp>age))return "Stale reading";return available ? "Ready":"Device unavailable";}
static const char *nd_sensor_summary(const struct NDSensors *s,const struct NDI2CCache *c,unsigned long selected,unsigned long i2c_selected,int enabled,unsigned long now)
{
 unsigned int i;unsigned long known=s->available;int chosen=0,stale=0,unavailable=0,live=0;
 for(i=0;i<ND_SENSOR_COUNT;++i){if(s->stamp[i])known|=1UL<<i;if((selected&(1UL<<i)) && (known&(1UL<<i))){chosen=1;if(s->available&(1UL<<i))live=1;else if(s->stamp[i] && (now<s->stamp[i] || now-s->stamp[i]>90))stale=1;else unavailable=1;}}
 if(enabled)for(i=0;i<c->count;++i)if(i2c_selected&(1UL<<i)){chosen=1;if(c->provider && c->readings[i].available && c->stamp && now>=c->stamp && now-c->stamp<=60 && now>=c->readings[i].stamp && now-c->readings[i].stamp<=60)live=1;else if(c->readings[i].stamp && (now<c->readings[i].stamp || now-c->readings[i].stamp>60))stale=1;else unavailable=1;}
 if(live)return "Ready";
 if(!chosen && (known || (enabled && c->count) || (!selected && (!enabled || !i2c_selected))))return "No channels selected";
 if(stale)return "Stale readings";
 if(unavailable)return enabled && c->count && !c->provider ? "I2C library missing":"Device unavailable";
 if(enabled && c->stamp && !c->provider)return "I2C library missing";
 return "No sensors detected";
}
#endif
