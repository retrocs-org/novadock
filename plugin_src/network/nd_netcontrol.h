#ifndef ND_NETCONTROL_H
#define ND_NETCONTROL_H
#include "nd_network.h"
#include "nd_roadshow.h"
#define NC_PROBE 0
#define NC_ONLINE 1
#define NC_OFFLINE 2
#define NC_QUIESCE_FAILED 1001
#define NC_LOG_FAILED 1002
#define NC_REQUEST_BUSY 0
#define NC_REQUEST_QUEUE 1
#define NC_REQUEST_RESTART 2
struct NCConfig {struct NDNetConfig traffic;char stack[16],interface_name[16];unsigned int timeout;int auto_online;};
struct NCInterfaces {char active[16],pending[16],selected[16];};
struct NCDeviceStatus {char device[ND_NET_DEVICE_LEN];unsigned long unit;int have_device,have_unit;};
void nc_status_line(struct NCInterfaces *interfaces,const char *line);
void nc_device_status_line(struct NCDeviceStatus *status,const char *line);
int nc_device_status_apply(struct NDNetConfig *traffic,const struct NCDeviceStatus *status);
int nc_startup_online(int *pending,const struct NCConfig *c,int installed,int online);
int nc_control_request(int busy,int action,int timed_out,int stopping,int requested);
unsigned long nc_job_deadline(const struct NCConfig *config,int action);
void nc_sampling_mode(struct NCConfig *c,int override_enabled);
void nc_defaults(struct NCConfig *c);
int nc_option(void *context,const char *key,const char *value);
int nc_interface_file(struct NDNetConfig *traffic,const char *path);
typedef int (*NCExecute)(void *context,const char *command);
int nc_quiesce_command(const struct NCConfig *c,char *out,unsigned int size);
int nc_disconnect(const struct NCConfig *c,NCExecute execute,void *context);
int nc_reconnect_command(const struct NCConfig *c,char *out,unsigned int size);
int nc_reconnect(const struct NCConfig *c,int dhcp,NCExecute execute,void *context);
int nc_command(const struct NCConfig *c,int action,char *out,unsigned int size);
#endif
