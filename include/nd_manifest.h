#ifndef ND_MANIFEST_H
#define ND_MANIFEST_H
/* Self-describing plugins.  A plugin describes itself with NOVADOCK_* lines,
 * either as tool types in its own icon (Plugins/Example.info) or in a text
 * file beside it (Plugins/Example.ndplugin).  Preferences reads these and
 * offers the plugin in Dock Bar > Available with its settings schema, so no
 * hand-editing of novadock.cfg is needed.
 *
 *   NOVADOCK_ID=EXAMPLE                 required: unique ID, letters/digits/_-
 *   NOVADOCK_NAME=Example               tile label (default: file name)
 *   NOVADOCK_VIEW=METER|GRAPH           initial view (default METER)
 *   NOVADOCK_ABI=1                      lowest dock IPC ABI it needs (1..3)
 *   NOVADOCK_MULTIPLE=YES|NO            allow repeated dock tiles (default NO)
 *   NOVADOCK_VERSION=1.0                plugin release
 *   NOVADOCK_AUTHOR=Example Author      publisher shown by Plugin Manager
 *   NOVADOCK_DESCRIPTION=What it does   short, single-line description
 *   NOVADOCK_LICENSE=MIT                licence identifier or short name
 *   NOVADOCK_WEBSITE=https://...        optional project page
 *   NOVADOCK_FEATURES=meter,click       comma-separated dock capabilities:
 *                                       meter  shows a value or progress bar
 *                                       graph  can show a history graph
 *                                       click  reacts to a click on its tile
 *                                       popup  opens a popup or window
 *                                       drop   accepts Workbench icons dropped on it
 *                                       media  player transport controls
 *                                       (settings and several tiles are
 *                                       reported from the option schema and
 *                                       NOVADOCK_MULTIPLE, not listed here)
 *   NOVADOCK_PROGRAM=Example            installed executable file name
 *   NOVADOCK_OPTION=KEY|Label|TYPE|choices|default    repeatable
 *
 * TYPE is BOOL, CHOICE (comma-separated), UINT (min,max) or TEXT, exactly as
 * for OPTIONDEF lines.  Unknown NOVADOCK_* keys are ignored for forward
 * compatibility; lines without the NOVADOCK_ prefix are ignored too. */
#include "nd_config.h"

#define ND_MANIFEST_MAX_OPTIONS 12
#define ND_MANIFEST_SUFFIX ".ndplugin"
#define ND_MANIFEST_DOCK_ABI 5 /* highest IPC ABI this dock understands */
#define ND_MANIFEST_VERSION_LEN 16
#define ND_MANIFEST_AUTHOR_LEN 48
#define ND_MANIFEST_DESCRIPTION_LEN 160
#define ND_MANIFEST_LICENSE_LEN 24
#define ND_MANIFEST_WEBSITE_LEN 128
#define ND_MANIFEST_FEATURES_LEN 128
#define ND_MANIFEST_PROGRAM_LEN 64

struct NDManifest {
    char id[20];
    char name[ND_LABEL_LEN];
    int graph;
    int abi;
    int multiple_instances;
    int option_count;
    char version[ND_MANIFEST_VERSION_LEN];
    char author[ND_MANIFEST_AUTHOR_LEN];
    char description[ND_MANIFEST_DESCRIPTION_LEN];
    char license[ND_MANIFEST_LICENSE_LEN];
    char website[ND_MANIFEST_WEBSITE_LEN];
    char features[ND_MANIFEST_FEATURES_LEN];
    char program[ND_MANIFEST_PROGRAM_LEN];
    struct NDOptionDef options[ND_MANIFEST_MAX_OPTIONS];
    char error[64]; /* first problem found, for diagnostics */
};

/* FILE_NAME (the executable's name) supplies the default label. */
void nd_manifest_init(struct NDManifest *m,const char *file_name);
/* 1 accepted, -1 not a NOVADOCK line (ignored), 0 invalid (see error). */
int nd_manifest_line(struct NDManifest *m,const char *line);
/* 1 if the manifest is complete and usable by this dock. */
int nd_manifest_finish(struct NDManifest *m);
/* Strict package validation. Third-party .ndpkg imports require the complete
 * public metadata contract even though legacy installed manifests remain
 * readable for backwards compatibility. */
int nd_manifest_package_ready(struct NDManifest *m);
/* Merge into a FULL configuration (not a compacted runtime copy):
 *   ND_MANIFEST_ADDED     new plugin entry added, disabled, with its schema
 *   ND_MANIFEST_UPDATED   known plugin: option schema refreshed from manifest
 *   ND_MANIFEST_UNCHANGED nothing to do (also for bundled plugin IDs)
 *   ND_MANIFEST_REJECTED  invalid, conflicting or no room
 * COMMAND is the executable path stored in the PLUGIN line. Existing option
 * values are never touched. */
#define ND_MANIFEST_REJECTED (-1)
#define ND_MANIFEST_UNCHANGED 0
#define ND_MANIFEST_ADDED 1
#define ND_MANIFEST_UPDATED 2
int nd_manifest_merge(struct NDConfig *cfg,const struct NDManifest *m,
                      const char *command);
/* 1 if ID belongs to a plugin bundled with NovaDock (built-in schema). */
int nd_manifest_bundled_id(const char *id);
/* Bundled plugins carry no manifest file; this fills M with their public
 * metadata (name, version, author, description, features, instance policy).
 * Returns 0 for any other ID. */
int nd_manifest_builtin(const char *id,struct NDManifest *m);
/* Rules for importing a third-party package from its manifest file
 * MANIFEST_FILE (name only, e.g. "Hello.ndplugin"): complete metadata
 * (nd_manifest_package_ready), NOVADOCK_PROGRAM equal to the manifest's
 * base name and a plain file name, and an ID not used by a bundled plugin.
 * 1 = acceptable; otherwise 0 with the reason in WHY. */
int nd_package_check(struct NDManifest *m,const char *manifest_file,
                     char *why,unsigned int why_size);
/* Human-readable capability summary, e.g. "Meter, graph, click; several
 * tiles; settings".  Unknown feature words are shown as written. */
void nd_manifest_describe(const char *features,int multiple,int settings,
                          char *out,unsigned int size);
#endif
