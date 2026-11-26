/* Legacy fixture entry point for the full native support/report regression.
 * Link tests/native_support_check.c (or this file) with src/nd_trace.c,
 * src/nd_support.c, src/nd_support_log.c, src/control_client.c, src/config.c
 * and src/nd_options.c, using -DND_RUNTIME_TRACE -Iinclude and -lamiga.
 * Run only in the offline QATests fixture with its marker and built Dock drawer. */
#include "native_support_check.c"
