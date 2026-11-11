#ifndef ND_CONTROL_CLIENT_H
#define ND_CONTROL_CLIENT_H
#include "nd_control.h"
struct NDConfig;
LONG nd_control_send(UWORD action);
int nd_control_sync_settings(void);
int nd_control_write_config(const struct NDConfig *cfg,const char *path);
/* Reload normally.  If an already-running older NovaDock rejects the current
 * configuration schema, replace it with the installed binary and retry. */
LONG nd_control_reload(void);
LONG nd_control_diagnostics(struct NDDiagnosticsSnapshot *snapshot);
/* Start the dock asynchronously; 1 if it was launched (not yet registered). */
int nd_control_launch_dock(void);
#endif
