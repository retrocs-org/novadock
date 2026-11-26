#ifndef ND_RETIRED_PLUGINS_H
#define ND_RETIRED_PLUGINS_H
#include <string.h>
/* These three integrations move to the independent AmiNotis service.
 * Retain their settings and source for migration, but never launch them. */
static int nd_plugin_retired(const char *id)
{
 return id && (!strcmp(id,"ALERTS") || !strcmp(id,"AMIRC") || !strcmp(id,"AMIROCKET"));
}
#endif
