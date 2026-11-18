#ifndef ND_HASS_SELECTION_H
#define ND_HASS_SELECTION_H
#include "nd_hass.h"
#include <string.h>
#include <stdio.h>
/* Only device IDs returned by the registry belong in a scoped LIST request. */
static int nd_hass_picker_device_id(const char *id)
{
 unsigned int i;if(!id || strlen(id)!=32)return 0;
 for(i=0;i<32;++i)if(!strchr("0123456789abcdefABCDEF",id[i]))return 0;
 return 1;
}
static int nd_hass_global_type(const char *domain)
{return domain && (!strcmp(domain,"scene") || !strcmp(domain,"script") || !strcmp(domain,"group"));}
static int nd_hass_type_matches(const char *domain,const char *entity)
{unsigned int n;if(!domain || !*domain || !strcmp(domain,"All"))return 1;n=(unsigned int)strlen(domain);return !strncmp(domain,entity,n) && entity[n]=='.';}
static void nd_hass_device_name(char out[64],const char *name)
{
 unsigned int i;for(i=0;i<63 && name[i];++i)out[i]=((unsigned char)name[i]<32 || name[i]=='|') ? ' ':name[i];out[i]=0;
}
static const char *nd_hass_click_choices(const char *entity)
{
 char path[96],body[180];
 if(entity && !strncmp(entity,"cover.",6))return "Show controls,Open cover,Close cover,Stop cover";
 if(nd_hass_action(entity,"Turn off",path,body))return "Toggle,Turn on,Turn off,Show controls";
 if(nd_hass_action(entity,"Activate",path,body))return "Activate,Show controls";
 return "Show controls";
}
static const char *nd_hass_type_choices(const char *domain)
{char id[64];if(!domain || strlen(domain)>23)return "Show controls";sprintf(id,"%s.selection",domain);return nd_hass_click_choices(id);}
#endif
