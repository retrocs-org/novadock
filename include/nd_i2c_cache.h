#ifndef ND_I2C_CACHE_H
#define ND_I2C_CACHE_H
#include <stdio.h>
#include <string.h>
#define ND_I2C_MAX 32
#define ND_I2C_CACHE "ENV:NovaDock/Sensors/I2C.status"
struct NDI2CReading {unsigned long id,stamp;long milli;unsigned int available;char name[64],unit[16];};
struct NDI2CCache {unsigned long stamp;unsigned int count,provider;struct NDI2CReading readings[ND_I2C_MAX];};
long nd_i2c_milli(long fixed);
void nd_i2c_format(const struct NDI2CReading *r,char *out);
static int nd_i2c_key(const char *key)
{unsigned int i;if(!key || strlen(key)!=12 || strncmp(key,"I2C_",4))return 0;for(i=4;i<12;++i)if(!((key[i]>='0'&&key[i]<='9')||(key[i]>='A'&&key[i]<='F')))return 0;return strcmp(key,"I2C_00000000")!=0;}
void nd_i2c_option(unsigned long id,char *key);
int nd_i2c_add(struct NDI2CCache *c,unsigned long id,long fixed,const char *name,const char *unit,unsigned long now);
int nd_i2c_read(FILE *f,struct NDI2CCache *c);
int nd_i2c_write(FILE *f,const struct NDI2CCache *c);
int nd_i2c_load(struct NDI2CCache *c);
int nd_i2c_fresh(const struct NDI2CCache *c,unsigned long now,unsigned long age);
#endif
