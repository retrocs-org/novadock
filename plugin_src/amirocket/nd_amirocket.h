#ifndef ND_AMIROCKET_H
#define ND_AMIROCKET_H
struct NDRocketConfig {char program[256],port[64];int badge;};
struct NDRocketStatus {unsigned long unread,alerts,servers,connected_servers;int connected,iconified;char epoch[80];};
void nd_rocket_defaults(struct NDRocketConfig *cfg);
int nd_rocket_option(void *context,const char *key,const char *value);
int nd_rocket_port(const char *name);
int nd_rocket_parse(struct NDRocketStatus *out,const char *snapshot);
void nd_rocket_badge(char *out,unsigned long unread,int enabled,int fresh);
#endif
