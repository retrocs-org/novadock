#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include "nd_async_io.h"
void nd_io_dispose(void *data,unsigned int bytes)
{struct NDIOJob *job=data;if(bytes!=sizeof(*job))return;if(job->icon && IconBase)FreeDiskObject(job->icon);if(job->brush)FreeVec(job->brush);if(job->decoded)FreeVec(job->decoded);job->icon=NULL;job->brush=NULL;job->decoded=NULL;}
