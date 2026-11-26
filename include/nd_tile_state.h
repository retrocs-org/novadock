#ifndef ND_TILE_STATE_H
#define ND_TILE_STATE_H
#include "nd_plugin_health.h"
#define ND_TILE_READY 0
#define ND_TILE_LOADING 1
#define ND_TILE_OFFLINE 2
#define ND_TILE_UNAVAILABLE 3
#define ND_TILE_ERROR 4
/* Interpret explicit status messages, never ordinary zero/OFF/idle values. */
static int nd_tile_state(const char *schema,int online,const char *value)
{
    struct NDPluginHealth h;
    if(!value)value="";
    if(!strcmp(value,"NOT RESPONDING") || !strcmp(value,"MISSING PLUGIN") ||
       !strncmp(value,"FAILED",6) || (!strcmp(value,"ERROR") || !strncmp(value,"ERROR:",6) || !strncmp(value,"ERROR ",6)) || !strcmp(value,"NO AREXX"))return ND_TILE_ERROR;
    if(!strcmp(value,"STARTING") || !strcmp(value,"STOPPING") ||
       !strncmp(value,"CONNECTING",10) || !strncmp(value,"LOADING",7) ||
       !strncmp(value,"SCANNING",8) || !strncmp(value,"INDEXING",8) ||
       !strncmp(value,"FETCHING",8) || !strncmp(value,"UPDATING",8))return ND_TILE_LOADING;
    if(!online)return ND_TILE_OFFLINE;
    h=nd_plugin_health(schema,online,0,value);
    if(!strcmp(h.label,"Starting") || !strcmp(h.label,"Waiting") || !strcmp(h.label,"Changing network"))return ND_TILE_LOADING;
    if(!strcmp(h.label,"Error") || !strcmp(h.label,"Missing plugin") || !strcmp(h.label,"Connection issue") || !strcmp(h.label,"Missing ARexx"))return ND_TILE_ERROR;
    if(!strcmp(h.label,"Network offline") || !strcmp(h.label,"No player") ||
       !strcmp(h.label,"Service stopped") || !strcmp(value,"OFFLINE") || !strcmp(value,"DISCONNECTED"))return ND_TILE_OFFLINE;
    if(strcmp(h.label,"Running") || nd_health_has(value,"unavailable") ||
       !strcmp(value,"NO DATA") || !strcmp(value,"NO DEVICE") || !strcmp(value,"NO STACK"))return ND_TILE_UNAVAILABLE;
    return ND_TILE_READY;
}
static const char *nd_tile_state_name(int state)
{
    switch(state){case ND_TILE_LOADING:return "Loading";case ND_TILE_OFFLINE:return "Offline";
    case ND_TILE_UNAVAILABLE:return "Unavailable";case ND_TILE_ERROR:return "Error";default:return "Ready";}
}
#endif
