#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "nd_plugin_health.h"
static void check(const char *id,int online,int stale,const char *value,const char *label,const char *help)
{struct NDPluginHealth h=nd_plugin_health(id,online,stale,value);assert(!strcmp(h.label,label));assert(strstr(h.help,help));}
int main(void)
{
 check("HASS",1,0,"off","Running","responding");
 check("HASS",1,0,"AmiHASS service is offline","Service stopped","AmiHASSPrefs");
 check("HASS",1,0,"Configure AmiHASS","Needs setup","token");
 check("HASS",1,0,"Entity unavailable","Unavailable","no controls");
 check("HASS",1,0,"AmiHASS connection failed","Connection issue","TLS");
 check("HASS",1,0,"AmiHASS data is stale","Connection issue","logs");
 check("HASS",1,0,"SELECT ENTITY","Needs setup","Device");
 check("HASS",1,0,"Waiting for AmiHASS","Waiting","status");
 check("MUSIC",1,0,"OFFLINE","No player","ARexx");
 check("MUSIC",0,0,"OFFLINE","Offline","libraries");
 check("CPU",0,1,"NOT RESPONDING","Not responding","Diagnostics");
 check("WEATHER",1,0,"NO DATA","No forecast","curl");
 check("WEATHER",1,0,"SET LOCATION","Needs setup","city");
 check("SENSORS",1,0,"NO SENSORS","No hardware data","drivers");
 check("SENSORS",1,0,"NO SELECTION","Needs setup","channels");
 check("SENSORS",1,0,"0.0 C","Running","responding");
 check("WHDLOAD",1,0,"FOLDER MISSING","Needs setup","scan drawer");
 check("MUSICLIB",1,0,"NO ITEMS","Running","responding");
 check("NET",1,0,"OFFLINE","Network offline","Online");
 check("NET",1,0,"STARTING","Changing network","Wait");
 check("ARCHIVE",1,0,"Idle","Running","responding");
 check("CPU",0,0,"MISSING PLUGIN","Missing plugin","Reinstall");
 check("CPU",0,0,"STARTING","Starting","first reading");
 check("CPU",0,0,"FAILED TO START","Error","Diagnostics");
 puts("Plugin health distinguishes runtime/setup/dependency failures, stopped processes and healthy idle/zero states.");return 0;
}
