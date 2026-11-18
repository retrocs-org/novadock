#ifndef AMIHASS_CONFIG_H
#define AMIHASS_CONFIG_H
#include "amihass_core.h"
#define AMIHASS_CONFIG_FILE "ENVARC:AmiHASS/AmiHASS.cfg"
struct AmiHASSConfig {
 struct AmiHASSConnection connection;char token[1025];
 char service_dir[300],prefs_dir[300],startup_dir[300];int boot,reconnect,low_memory,sample_mode;unsigned int retry_interval;
};
void amihass_config_defaults(struct AmiHASSConfig *config);
int amihass_config_load(struct AmiHASSConfig *config);
int amihass_config_save(const struct AmiHASSConfig *config);
int amihass_config_forget(void);
#endif
