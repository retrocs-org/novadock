#ifndef ND_VOLUME_JOB_H
#define ND_VOLUME_JOB_H
#include "nd_volume_worker.h"
struct NDVolumeReading {char path[ND_COMMAND_LEN];struct NDVolumeSpace space;int valid;unsigned long sampled,error;};
struct NDVolumeBatch {unsigned int count;struct NDVolumeReading rows[ND_MAX_ITEMS];};
#endif
