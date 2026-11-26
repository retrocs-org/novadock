#ifndef ND_SENSOR_ART_H
#define ND_SENSOR_ART_H
#include <ctype.h>
#include <string.h>
/* Display-only classification: never infers a measurement or a hardware source. */
static int nd_sensor_word(const char *text,const char *word)
{
 unsigned int i,j;if(!text)return 0;
 for(i=0;text[i];++i){for(j=0;word[j] && text[i+j] && tolower((unsigned char)text[i+j])==word[j];++j){}if(!word[j])return 1;}return 0;
}
static const char *nd_sensor_named_art(const char *name,const char *unit)
{
 if(nd_sensor_word(name,"fpga") || nd_sensor_word(name,"zynq"))return "SensorFPGA";
 if(nd_sensor_word(name,"ram") || nd_sensor_word(name,"memory"))return "SensorRAM";
 if(nd_sensor_word(name,"cpu") || nd_sensor_word(name,"680") || nd_sensor_word(name,"arm"))return "SensorCPU";
 if(nd_sensor_word(name,"batt"))return "SensorBattery";
 if(nd_sensor_word(name,"fan") || nd_sensor_word(unit,"rpm"))return "SensorFan";
 if(nd_sensor_word(name,"volt") || (unit && !strcmp(unit,"V")))return "SensorPower";
 if(nd_sensor_word(name,"temp") || (unit && !strcmp(unit,"C")))return "SensorTemp";
 return "Sensor";
}
static const char *nd_sensor_art(int index)
{
 static const char *art[]={"SensorFPGA","SensorPower","SensorPower","SensorTemp","SensorCPU","SensorPower","SensorBattery","SensorCPU","SensorCPU","SensorCPU","SensorFPGA","SensorPower","SensorBattery","SensorFan"};
 return index>=0 && index<14 ? art[index]:"Sensor";
}
#endif
