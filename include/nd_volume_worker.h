#ifndef ND_VOLUME_WORKER_H
#define ND_VOLUME_WORKER_H
#include "nd_config.h"
#include "nd_volume.h"
#define ND_VOLUME_UNKNOWN 0
#define ND_VOLUME_FRESH 1
#define ND_VOLUME_STALE 2
#define ND_VOLUME_UNAVAILABLE 3
struct NDVolumeStatus {unsigned long age_seconds,error;int state;};
int nd_volume_meters_status(const char *path,struct NDVolumeStatus *status);
/* No monitored-path filesystem calls in these parent APIs. One independently loaded worker
 * owns its copy of a bounded, deduplicated batch and may outlive dock exit. */
void nd_volume_meters_begin(void);
void nd_volume_meters_add(const char *path);
int nd_volume_meters_submit(const char *program);
int nd_volume_meters_poll(void);
int nd_volume_meters_cached(const char *path,struct NDVolumeSpace *space);
unsigned long nd_volume_meters_signals(void);
void nd_volume_meters_close(void);
int nd_volume_worker(const char *parent);
#endif
