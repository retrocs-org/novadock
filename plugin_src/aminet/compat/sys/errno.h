#ifndef ND_AMINET_SYS_ERRNO_H
#define ND_AMINET_SYS_ERRNO_H
#ifdef __VBCC__
#include <errno.h>
#else
#include_next <sys/errno.h>
#endif
#endif
