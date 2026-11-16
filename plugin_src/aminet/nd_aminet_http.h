/* Adapted from Beam68. Copyright (c) 2026 Beam68 contributors.
 * MIT licence: see Aminet-parser-LICENSE.txt in the package. */
#ifndef NOVADOCK_AMINET_HTTP_H
#define NOVADOCK_AMINET_HTTP_H
/* progress(bytes,total) returns zero to cancel; total=0 means not yet known. HTTP/1.0, identity body, fixed Aminet host. */
int nd_aminet_fetch(const char *path,const char *destination,unsigned long limit,
                      unsigned long seconds,int require_length,int (*progress)(unsigned long,unsigned long),
                      char *error,unsigned int error_cap);
#endif
