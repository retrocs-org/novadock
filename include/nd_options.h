#ifndef ND_OPTIONS_H
#define ND_OPTIONS_H
#include "nd_config.h"
/* Types: BOOL, CHOICE (comma-separated values), UINT (min,max), TEXT, FILE.
 * The schema describes UI only. No extra configuration files are created. */
const struct NDOptionDef *nd_builtin_options(int *count);
/* Built-in schema ID used for plugin ID: "COMMAND" for any tile whose
 * executable is Plugins/Command (several such tiles share one schema),
 * otherwise ID itself. */
#define ND_COMMAND_SCHEMA "COMMAND"
#define ND_SCRIPT_SCHEMA "SCRIPT"
const char *nd_option_schema_id(const struct NDConfig *cfg,const char *id);
int nd_command_plugin_path(const char *command);
int nd_games_plugin_path(const char *command);
/* 1 for WHDLoad, 2 for MUSIC, 0 for other programs. */
int nd_collection_plugin_path(const char *command);
int nd_sensor_plugin_path(const char *command);
int nd_actions_plugin_path(const char *command);
int nd_hass_plugin_path(const char *command);
/* Independent settings: collections 1/2, Sensors 3, Actions 4 and HASS 5. */
int nd_monitor_plugin_path(const char *command);
const char *nd_independent_plugin_id(const char *command);
int nd_shelf_plugin_path(const char *command);
int nd_independent_plugin_path(const char *command);
const char *nd_games_icon_path(const struct NDConfig *cfg,const char *id);
int nd_collection_art_path(const char *base,const char *choice,const char *custom,char *out,unsigned int size);
/* Only this instance's .idx/.tmp/.bak cache files; rejects unsafe IDs. */
int nd_games_forget_cache(const char *id);
int nd_script_plugin_path(const char *command);
/* Atomic editor batch: 1 success, 0 invalid, -1 capacity, -2 memory. */
int nd_option_apply(struct NDConfig *cfg,const char *id,const struct NDOptionDef *defs,
                    const char values[][ND_OPT_VALUE_LEN],int count);
int nd_option_validate(const struct NDOptionDef *def, const char *value);
/* Stable ordering for settings pages. Returns the number of normal rows;
 * remaining rows are advanced. Other plugins retain their original order. */
int nd_option_group_settings(const char *id,struct NDOptionDef *defs,int count);
int nd_option_collect(const struct NDConfig *cfg, const char *id,
                      struct NDOptionDef *out, int maximum);
#endif
