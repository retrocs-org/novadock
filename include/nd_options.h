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
int nd_script_plugin_path(const char *command);
int nd_option_validate(const struct NDOptionDef *def, const char *value);
/* Stable ordering for settings pages. Returns the number of normal rows;
 * remaining rows are advanced. Other plugins retain their original order. */
int nd_option_group_settings(const char *id,struct NDOptionDef *defs,int count);
int nd_option_collect(const struct NDConfig *cfg, const char *id,
                      struct NDOptionDef *out, int maximum);
#endif
