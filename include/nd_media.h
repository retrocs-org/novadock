#ifndef ND_MEDIA_H
#define ND_MEDIA_H
/* Removable-media tiles: portable model and policy.  The AmigaOS side scans
 * the DOS volume list; this module decides which volumes qualify and turns
 * them into runtime-only drawer tiles appended after the configured items. */
#include "nd_config.h"

#define ND_MEDIA_MAX 4
#define ND_MEDIA_NAME_LEN 32
#define ND_MEDIA_DRIVE_LEN 16
#define ND_MEDIA_DEVICE_LEN 32
/* exec device names whose units hold removable media.  Override with
 * OPTION.MEDIA.DEVICES=a.device,b.device in novadock.cfg. */
#define ND_MEDIA_DEFAULT_DEVICES \
    "trackdisk.device,mfm.device,carddisk.device,compactflash.device," \
    "usbscsi.device,massstorage.device,diskimage.device"

struct NDMediaVolume {
    char volume[ND_MEDIA_NAME_LEN];   /* volume name without colon */
    char drive[ND_MEDIA_DRIVE_LEN];   /* DOS device with colon, e.g. "DF0:" */
    char device[ND_MEDIA_DEVICE_LEN]; /* exec device, e.g. trackdisk.device */
    unsigned long unit;
};

/* 1 if DEVICE appears (case-insensitively) in the comma-separated LIST, or
 * in the default list when LIST is NULL or empty. */
int nd_media_device_removable(const char *device,const char *list);
/* Sort by drive then volume and remove exact duplicates; returns new count. */
int nd_media_normalize(struct NDMediaVolume *media,int count);
/* 1 if two normalised lists describe the same set. */
int nd_media_same(const struct NDMediaVolume *a,int a_count,
                  const struct NDMediaVolume *b,int b_count);
/* Append transient drawer tiles for MEDIA to a compacted runtime CFG.
 * Volumes already pinned as drawers, and SYSTEM_VOLUME (the volume holding
 * SYS:, without colon; may be NULL), are skipped.  Returns tiles added. */
int nd_media_append(struct NDConfig *cfg,const struct NDMediaVolume *media,
                    int count,const char *system_volume);
/* Number of leading non-transient items in a runtime configuration. */
int nd_media_persistent_count(const struct NDConfig *cfg);
#endif
