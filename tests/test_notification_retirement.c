#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_manifest.h"
int main(void){static struct NDConfig c,back;struct NDManifest m;int n,i;char line[500];
 nd_default_config(&c);n=c.count;
 assert(nd_parse_line(&c,"PLUGIN=Chat|Plugins/AmIRC|AMIRC|METER|ON"));
 assert(nd_parse_line(&c,"PLUGIN=Rocket|Plugins/AmiRocket|AMIROCKET|METER|ON"));
 assert(nd_parse_line(&c,"PLUGIN=History|Plugins/Alerts|ALERTS|METER|ON"));
 assert(c.count==n&&c.unknown_count==3);
 assert(nd_parse_line(&c,"OPTION.AMIRC.PROGRAM=Work:Apps/AmIRC"));
 assert(nd_parse_line(&c,"OPTION.AMIROCKET.PROGRAM=Work:Apps/AmiRocket"));
 strcpy(line,"PLUGIN=Long|Work:");for(i=(int)strlen(line);i<230;++i)line[i]='a';strcpy(line+230,"|AMIRC|METER|ON");assert(nd_parse_line(&c,line));assert(c.count==n&&c.unknown_count>3);
 assert(!c.send_alerts_aminotis);assert(nd_parse_line(&c,"SEND_ALERTS_TO_AMINOTIS=YES")&&c.send_alerts_aminotis);assert(!nd_parse_line(&c,"SEND_ALERTS_TO_AMINOTIS=MAYBE"));
 assert(nd_write_config(&c,"notification-retirement.cfg"));assert(nd_load_config(&back,"notification-retirement.cfg")==0);assert(back.send_alerts_aminotis&&back.unknown_count==c.unknown_count);for(i=0;i<c.unknown_count;++i)assert(!strcmp(c.unknown[i],back.unknown[i]));
 assert(!strcmp(nd_option_get(&back,"AMIRC","PROGRAM"),"Work:Apps/AmIRC"));assert(!strcmp(nd_option_get(&back,"AMIROCKET","PROGRAM"),"Work:Apps/AmiRocket"));
 for(i=0;i<back.count;++i)assert(strcmp(back.items[i].id,"AMIRC")&&strcmp(back.items[i].id,"AMIROCKET")&&strcmp(back.items[i].id,"ALERTS"));
 nd_manifest_init(&m,"Plugins/AmIRC");assert(nd_manifest_line(&m,"NOVADOCK_ID=AMIRC"));assert(!nd_manifest_finish(&m)&&strstr(m.error,"retired"));
 assert(!nd_manifest_builtin("ALERTS",&m)&&!nd_manifest_builtin("AMIRC",&m)&&!nd_manifest_builtin("AMIROCKET",&m));
 remove("notification-retirement.cfg");remove("notification-retirement.cfg.bak");remove("notification-retirement.cfg.good");puts("Notification retirement: legacy enabled tiles inert, settings and long metadata retained, forwarding opt-in persists, retired manifests rejected.");return 0;}
