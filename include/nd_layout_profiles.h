#ifndef ND_LAYOUT_PROFILES_H
#define ND_LAYOUT_PROFILES_H

#include "nd_config.h"

#define ND_LAYOUT_NAME_LEN 32
#define ND_LAYOUT_MAX_PROFILES 32

/* Names become AmigaDOS filenames, so keep the accepted alphabet deliberate. */
int nd_layout_name_valid(const char *name);
int nd_layout_profile_path(char *path,unsigned int path_size,
                           const char *root,const char *name);

/* A named layout restores the complete visible dock behaviour: contents,
 * rows, placement and appearance. Plugin OPTION definitions/values remain
 * global so switching layouts never forks plugin configuration. */
int nd_layout_apply(struct NDConfig *target,const struct NDConfig *saved);

#endif
