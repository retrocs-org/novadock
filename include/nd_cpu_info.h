#ifndef ND_CPU_INFO_H
#define ND_CPU_INFO_H
static const char *nd_system_cpu(unsigned long flags)
{return flags&128UL ? "68060":flags&8UL ? "68040":flags&4UL ? "68030":flags&2UL ? "68020":flags&1UL ? "68010":"68000";}
#endif
