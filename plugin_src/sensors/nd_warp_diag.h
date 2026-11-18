#ifndef ND_WARP_DIAG_H
#define ND_WARP_DIAG_H
#include "nd_sensors.h"
/* Recovered from WarpTool/cswarp.library 30.74. This is the library result,
 * NOT the ARM mailbox frame. Keep the experimental caller version-gated. */
#define ND_WARP_DIAG_SIZE 428
int nd_warp_diag_supported(unsigned int major,unsigned int revision);
int nd_warp_diag_decode(const unsigned char *data,unsigned int size,
                       struct NDSensors *set,unsigned long now);
#endif
