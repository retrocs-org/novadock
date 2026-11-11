#ifndef ND_AMIRC_H
#define ND_AMIRC_H
#define ND_AMIRC_STATUS 0
#define ND_AMIRC_NICK 1
#define ND_AMIRC_CHANNEL 2
#define ND_AMIRC_SERVER 3
struct NDAmIRCConfig {
    char program[256];
    char port[64];
    unsigned int poll_ticks;
    int display;
    int alerts,clear_on_focus;
};
void nd_amirc_defaults(struct NDAmIRCConfig *cfg);
int nd_amirc_apply_option(void *context,const char *key,const char *value);
int nd_amirc_port_name(const char *name);
void nd_amirc_clean(char *out,unsigned int size,const char *in);
const char *nd_amirc_display_query(int display);
void nd_amirc_status(char *caption,unsigned int caption_size,char *value,
                     unsigned int value_size,int present,int connected,
                     int away,int display,const char *detail,int unresponsive);
int nd_amirc_connection_indicator(int present,int connected,int unresponsive);
int nd_amirc_quote(char *out,unsigned int size,const char *path);
#define ND_AMIRC_ALERT_PRIVATE 1
#define ND_AMIRC_ALERT_HIGHLIGHT 2
#define ND_AMIRC_ALERT_PORT "NOVADOCK.AMIRC"
struct NDAmIRCAlerts {unsigned int count;};
int nd_amirc_alert_command(const char *command,const char *host,int enabled);
void nd_amirc_alert_add(struct NDAmIRCAlerts *alerts);
void nd_amirc_badge(char *out,unsigned int size,unsigned int count,int connected);
#endif
