#ifndef AMIHASS_CONFIG_H
#define AMIHASS_CONFIG_H
#include "amihass_core.h"
#define AMIHASS_CONFIG_FILE "ENVARC:AmiHASS/AmiHASS.cfg"
/* The access token lives apart from the settings so AmiHASS.cfg can be
 * shared for support without exposing it. */
#define AMIHASS_TOKEN_FILE "ENVARC:AmiHASS/AmiHASS.token"
struct AmiHASSConfig {
 struct AmiHASSConnection connection;char token[1025];
 char service_dir[300],prefs_dir[300],startup_dir[300];int boot,reconnect,low_memory,sample_mode;unsigned int retry_interval,background_budget;int recovered,push_updates;
 /* 1: door/cover actions from ARexx, CLI and other SDK clients run without the
  * Dashboard's confirmation. Default 0: only the Dashboard may send them. */
 int remote_security;
};
void amihass_config_defaults(struct AmiHASSConfig *config);
int amihass_config_load(struct AmiHASSConfig *config);
int amihass_config_save(const struct AmiHASSConfig *config);
int amihass_config_forget(void);
/* Remove the boot-time ENV: copies of settings and token (RAM only). */
void amihass_config_scrub_env(void);
#endif
