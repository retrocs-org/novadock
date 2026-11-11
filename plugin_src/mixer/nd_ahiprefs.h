#ifndef ND_AHIPREFS_H
#define ND_AHIPREFS_H
/* Portable access to AHI's preferences file (ENV:Sys/ahi.prefs): an IFF
 * FORM PREF holding one AHIU chunk per unit.  ahi.device re-reads it each
 * time ahi.device is opened for low-level use and applies ahiup_OutputVolume
 * (a 16.16 Fixed value) when a program allocates the default audio mode. */
#define ND_AHI_UNIT_LOWLEVEL 255U   /* AHI_NO_UNIT: low-level programs */
#define ND_AHI_FIXED_ONE 0x10000UL  /* 1.0 = full volume */
#define ND_AHI_PREFS_MAX 4096UL     /* larger files are not AHI prefs */

/* Output volume stored for the low-level unit, else unit 0, else the first
 * unit found.  Returns 1 and sets *FIXED when the file has an AHIU chunk. */
int nd_ahi_prefs_volume(const unsigned char *buffer,unsigned long length,
                        unsigned long *fixed);
/* Sets the output volume of every unit.  Returns the number of AHIU
 * chunks changed (0: not an AHI preferences file). */
int nd_ahi_prefs_set_volume(unsigned char *buffer,unsigned long length,
                            unsigned long fixed);
/* 0..100 percent of full volume, rounded; values above 1.0 show as 100. */
unsigned int nd_ahi_percent(unsigned long fixed);
unsigned long nd_ahi_fixed(unsigned int percent);
#endif
