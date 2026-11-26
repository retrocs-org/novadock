#ifndef ND_PLUGIN_SUPPORT_H
#define ND_PLUGIN_SUPPORT_H
#include <exec/types.h>
#define ND_PLUGIN_SUPPORT_VERSION 1U
#define ND_PLUGIN_SUPPORT_PARTIAL 1U
#define ND_PLUGIN_OP_NONE 0U
#define ND_PLUGIN_OP_PUBLISH 1U
#define ND_PLUGIN_OP_ACTION 2U
#define ND_PLUGIN_OP_SETTINGS 3U
#define ND_PLUGIN_OP_OPEN 4U
#define ND_PLUGIN_OP_READ 5U
#define ND_PLUGIN_OP_WRITE 6U
#define ND_PLUGIN_OP_DIRECTORY 7U
#define ND_PLUGIN_OP_SCAN 8U
#define ND_PLUGIN_OP_CACHE 9U
#define ND_PLUGIN_OP_JOB 10U
#define ND_PLUGIN_OP_NETWORK 11U
#define ND_PLUGIN_OP_ERROR 12U
#define ND_PLUGIN_OP_COUNT 13U
struct NDPluginSupportStats {
 UWORD version,flags,operation,phase;
 ULONG heap,peak,allocations,frees,failures,events;
 ULONG opens,reads,writes,directories,read_bytes,write_bytes;
 LONG detail_a,detail_b;
};
static const char *nd_plugin_operation(unsigned int n)
{static const char *names[]={"Waiting","Publish status","User action","Settings","Open file","Read file","Write file","Read directory","Scan database","Index cache","Background job","Network","Error"};return n<ND_PLUGIN_OP_COUNT ? names[n]:"Unknown";}
#endif
