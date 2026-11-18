#ifndef ND_UPDATE_H
#define ND_UPDATE_H
#define ND_UPDATE_JSON_LIMIT 131072U
#define ND_UPDATE_FILE_LIMIT 67108864UL
#define ND_UPDATE_DAY 86400UL
#define ND_UPDATE_RETRY 3600UL
#define ND_UPDATE_API "https://api.github.com/repos/retrocs-org/novadock/releases/latest"
struct NDUpdateRelease {char tag[32],version[32],file[80],url[240],checksum_url[260],digest[65];unsigned long size;};
struct NDUpdateState {unsigned long next_attempt,last_notice,checked;int result;char noticed[32],latest[32];};
struct NDUpdateSchedule {unsigned long ticks,next_due;int enabled,waiting,startup;unsigned int hours;};
/* Returns -1/0/1, or 2 for an unsupported version string. */
int nd_update_compare(const char *a,const char *b);
int nd_update_release(const char *json,struct NDUpdateRelease *release);
int nd_update_checksum(const char *text,const char *filename,char out[65]);
int nd_update_notice_due(const struct NDUpdateState *s,const char *version,unsigned long now);
/* No I/O: decides when the dock may dispatch one independent check. */
unsigned long nd_update_interval(unsigned int hours);
int nd_update_schedule(struct NDUpdateSchedule *s,int startup,int running,unsigned int hours,int network,int busy,unsigned long now,unsigned long elapsed_ticks);
int nd_update_staging_dir(const char *path);
struct NDUpdateStatus {unsigned long checked;int busy,checking,result;char version[32];};
void nd_update_status(struct NDUpdateStatus *status);
int nd_update_helper_main(int argc,char **argv);
void nd_update_tick(struct NDUpdateSchedule *s,int startup,int running,unsigned int hours,const char *directory,unsigned long elapsed_ticks);
#endif
