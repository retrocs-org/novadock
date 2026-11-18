#ifndef ND_HASS_ART_H
#define ND_HASS_ART_H
#include <string.h>
#include <ctype.h>
static int nd_hass_icon_hint(const char *text,const char *needle)
{
 const char *p;unsigned int i;if(!text)return 0;
 for(p=text;*p;++p){for(i=0;needle[i] && p[i] && tolower((unsigned char)p[i])==needle[i];++i){}if(!needle[i])return 1;}return 0;
}
static const char *nd_hass_auto_icon(const char *entity,const char *device_class,const char *name)
{
 if(!entity)entity="";if(!device_class)device_class="";
 if(!strncmp(entity,"light.",6)){
  if(nd_hass_icon_hint(entity,"christmas") || nd_hass_icon_hint(name,"christmas"))return "ChristmasLights";
  if(!strcmp(device_class,"strip") || nd_hass_icon_hint(entity,"strip") || nd_hass_icon_hint(name,"strip"))return "LightStrip";
  if(nd_hass_icon_hint(entity,"ceiling") || nd_hass_icon_hint(name,"ceiling"))return "CeilingLight";
  if(nd_hass_icon_hint(entity,"floor") || nd_hass_icon_hint(name,"floor lamp"))return "FloorLamp";
  if(nd_hass_icon_hint(entity,"shelf") || nd_hass_icon_hint(name,"shelf"))return "ShelfLamp";
  if(nd_hass_icon_hint(entity,"group") || nd_hass_icon_hint(name,"lights"))return "LightGroup";
  return "Bulb";
 }
 if(!strncmp(entity,"switch.",7) || !strncmp(entity,"input_boolean.",14))return "Switch";
 if(!strncmp(entity,"fan.",4))return "Fan";
 if(!strncmp(entity,"media_player.",13))return "TV";
 if(!strncmp(entity,"climate.",8))return nd_hass_icon_hint(entity,"heat") || nd_hass_icon_hint(name,"heater") ? "Heater":"AC";
 if(!strncmp(entity,"lock.",5))return "FrontDoor";
 if(!strncmp(entity,"cover.",6))return !strcmp(device_class,"garage") || !strcmp(device_class,"garage_door") || nd_hass_icon_hint(entity,"garage") ? "Garage":"Door";
 if(!strncmp(entity,"sensor.",7) || !strncmp(entity,"binary_sensor.",14)){
  if(!strcmp(device_class,"temperature"))return "Thermometer";
  if(!strcmp(device_class,"humidity"))return "Moisture";
  if(!strcmp(device_class,"moisture"))return "Leak";
  if(!strcmp(device_class,"smoke"))return "FireAlarm";
  if(!strcmp(device_class,"window"))return "WindowSensor";
  if(!strcmp(device_class,"garage_door"))return "Garage";
  if(!strcmp(device_class,"door") || !strcmp(device_class,"opening"))return "DoorSensor";
  return "Sensor";
 }
 if(!strncmp(entity,"scene.",6) || !strncmp(entity,"script.",7))return "Scene";
 if(!strncmp(entity,"group.",6))return "LightGroup";
 return "HASS";
}
static const char *nd_hass_default_icon(const char *entity)
{return nd_hass_auto_icon(entity,NULL,NULL);}
#define ND_HASS_ICON_OFF 0
#define ND_HASS_ICON_ON 1
#define ND_HASS_ICON_UNKNOWN 2
/* Use confirmed state text, never the requested action, for artwork. */
static int nd_hass_visual_state(const char *entity,const char *value)
{
 char state[40];unsigned int n=0;if(!value)return ND_HASS_ICON_UNKNOWN;
 while(value[n] && value[n]!=' ' && n<sizeof(state)-1){state[n]=(char)tolower((unsigned char)value[n]);++n;}state[n]=0;
 if(!strcmp(state,"off") || !strcmp(state,"closed") || !strcmp(state,"closing") || !strcmp(state,"locked") || !strcmp(state,"standby") || !strcmp(state,"paused") || !strcmp(state,"idle"))return ND_HASS_ICON_OFF;
 if(!strcmp(state,"on") || !strcmp(state,"open") || !strcmp(state,"opening") || !strcmp(state,"unlocked") || !strcmp(state,"playing") || !strcmp(state,"heat") || !strcmp(state,"cool") || !strcmp(state,"heating") || !strcmp(state,"cooling") || !strcmp(state,"auto") || !strcmp(state,"fan_only") || !strcmp(state,"dry"))return ND_HASS_ICON_ON;
 if(entity && !strncmp(entity,"sensor.",7) && (isdigit((unsigned char)state[0]) || ((state[0]=='-' || state[0]=='+') && isdigit((unsigned char)state[1]))))return ND_HASS_ICON_ON;
 return ND_HASS_ICON_UNKNOWN;
}
/* Service and transport failures use warning artwork, preserving the reason
 * in the normal tooltip. This never changes the saved custom icon choice. */
static int nd_hass_sad_status(const char *value)
{
 if(!value)return 0;
 return !strcmp(value,"AmiHASS service is offline") ||
        !strcmp(value,"AmiHASS connection failed") ||
        !strcmp(value,"AmiHASS data is stale") ||
        !strcmp(value,"Configure AmiHASS") ||
        !strcmp(value,"Install/start AmiHASS service") ||
        !strncmp(value,"Connection/authentication",25) ||
        !strncmp(value,"Stale;",6) ||
        !strncmp(value,"Set up Home Assistant",21);
}
#endif
