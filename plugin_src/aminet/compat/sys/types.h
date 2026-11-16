#ifndef ND_AMINET_SYS_TYPES_H
#define ND_AMINET_SYS_TYPES_H
/* The NDK socket prototypes need ssize_t; vbcc's ISO C library lacks it. */
#ifdef __VBCC__
#include <stddef.h>
typedef long ssize_t;
typedef unsigned long __uint32_t;
#else
#include_next <sys/types.h>
#endif
#endif
