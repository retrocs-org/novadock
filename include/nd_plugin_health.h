#ifndef ND_PLUGIN_HEALTH_H
#define ND_PLUGIN_HEALTH_H
#include <string.h>
#include <ctype.h>
struct NDPluginHealth {const char *label,*help;};
static int nd_health_has(const char *value,const char *word)
{
 const char *a,*b;if(!value)return 0;
 for(;*value;++value){a=value;b=word;while(*a && *b && tolower((unsigned char)*a)==tolower((unsigned char)*b)){++a;++b;}if(!*b)return 1;}return 0;
}
/* Runtime failures differ from an absent/unresponsive plugin process. A
 * switched-off light, idle archive, or empty collection is still healthy. */
static struct NDPluginHealth nd_plugin_health(const char *schema,int online,int stale,const char *value)
{
 struct NDPluginHealth h;h.label="Running";h.help="Plugin is responding. Edit its tile settings to change its configuration.";
 if(stale || !strcmp(value,"NOT RESPONDING")){h.label="Not responding";h.help="The plugin process stopped replying. Check Diagnostics; restart NovaDock after saving.";return h;}
 if(!online){h.label="Offline";h.help="No plugin heartbeat. Check the executable and its required libraries; see Diagnostics.";return h;}
 if(!strcmp(schema,"HASS")){
  if(nd_health_has(value,"service is offline")){h.label="Service stopped";h.help="Install/start AmiHASSService using AmiHASSPrefs, then refresh this status.";}
  else if(nd_health_has(value,"Configure AmiHASS")){h.label="Needs setup";h.help="Set the server and token in AmiHASSPrefs. Credentials belong to the bridge.";}
  else if(nd_health_has(value,"SELECT ENTITY")){h.label="Needs setup";h.help="Edit this HASS tile and select its Type and Device.";}
  else if(nd_health_has(value,"unavailable")){h.label="Unavailable";h.help="Check the selected entity in Home Assistant; no controls are sent while unavailable.";}
  else if(nd_health_has(value,"failed") || nd_health_has(value,"stale")){h.label="Connection issue";h.help="Check AmiHASSPrefs status and logs for network, server, token, curl or TLS errors.";}
  else if(nd_health_has(value,"Waiting") || nd_health_has(value,"busy")){h.label="Waiting";h.help="AmiHASS is updating. Use its Preferences status/logs if the wait persists.";}
 }else if(!strcmp(schema,"NET")){
  if(!strcmp(value,"OFFLINE")){h.label="Network offline";h.help="The plugin is running. Use Online to connect, or open Network details to inspect the stack.";}
  else if(!strcmp(value,"STARTING") || !strcmp(value,"STOPPING")){h.label="Changing network";h.help="Wait for the current operation. Open Network details if it does not finish.";}
 }else if(!strcmp(schema,"MUSIC")){
  if(!strcmp(value,"NO AREXX")){h.label="Missing ARexx";h.help="Install/restart rexxsyslib.library and RexxMast, then restart this plugin.";}
  else if(!strcmp(value,"OFFLINE")){h.label="No player";h.help="Start a supported music player and check its ARexx port in MusicPlayer settings.";}
 }else if(!strcmp(schema,"WEATHER")){
  if(!strcmp(value,"SET LOCATION")){h.label="Needs setup";h.help="Set a city and country in Weather settings.";}
  else if(!strcmp(value,"NO DATA")){h.label="No forecast";h.help="Check Weather settings, TCP/IP and HTTPS-capable curl/Wget; open Weather for status.";}
 }else if(!strcmp(schema,"SENSORS")){
  if(!strcmp(value,"NO SENSORS")){h.label="No hardware data";h.help="Check supported hardware, its drivers and configured sensor provider tools.";}
  else if(!strcmp(value,"NO SELECTION")){h.label="Needs setup";h.help="Edit this SENSOR instance and select the detected channels to display.";}
 }else if(!strcmp(schema,"WHDLOAD") || !strcmp(schema,"MUSICLIB")){
  if(nd_health_has(value,"folder") || nd_health_has(value,"drawer") || !strcmp(value,"NO FORMATS SELECTED")){
   h.label="Needs setup";h.help="Edit this collection: choose a readable scan drawer and enable the desired formats.";}
 }
 return h;
}
#endif
