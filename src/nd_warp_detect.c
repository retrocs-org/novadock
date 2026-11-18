/* WarpTool 30.74 locates its control device using these Autoconfig IDs.
 * Presence alone does not identify a 1260 or authorize a library call. */
#define ExpansionBase NDWarpExpansionBase
#include <exec/types.h>
#include <libraries/expansion.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include "nd_warp_detect.h"
struct ExpansionBase *ExpansionBase=NULL;
int nd_warp_control_present(void)
{
 int present=0;
 ExpansionBase=(struct ExpansionBase *)OpenLibrary("expansion.library",0);
 if(ExpansionBase){present=FindConfigDev(NULL,0x1400,60)!=NULL;
  CloseLibrary((struct Library *)ExpansionBase);ExpansionBase=NULL;}
 return present;
}

int nd_prism_driver_present(void)
{
 struct Process *self=(struct Process *)FindTask(NULL);APTR old;BPTR lock;int present;
 Forbid();present=FindName(&SysBase->DeviceList,"prism2.device")!=NULL;Permit();
 if(present)return 1;
 old=self->pr_WindowPtr;self->pr_WindowPtr=(APTR)-1;
 lock=Lock("DEVS:Networks/prism2.device",ACCESS_READ);
 self->pr_WindowPtr=old;if(lock)UnLock(lock);return lock!=0;
}
