#ifndef ND_PLUGIN_DISCOVERY_H
#define ND_PLUGIN_DISCOVERY_H
/* Preferences-side scan of the Plugins drawer for self-describing plugins
 * (see nd_manifest.h).  Results stay in the working configuration only; a
 * new plugin reaches novadock.cfg when the user adds it and uses Use/Save. */
#include "nd_config.h"
#include "nd_manifest.h"
#define ND_DISCOVERY_MAX 8
#define ND_PLUGIN_CATALOG_MAX ND_MAX_ITEMS
struct NDPluginCatalogEntry {
    struct NDManifest manifest;
    char command[ND_COMMAND_LEN];
    char manifest_path[ND_COMMAND_LEN];
    int bundled;
};
struct NDDiscovery {
    int added;
    int updated;
    int rejected;
    char added_ids[ND_DISCOVERY_MAX][20];
    char added_names[ND_DISCOVERY_MAX][ND_LABEL_LEN];
    char problem[96]; /* first rejected plugin and why */
};
extern struct NDDiscovery nd_prefs_discovery;
int nd_discover_plugins(struct NDConfig *cfg,struct NDDiscovery *out);
/* What a plugin list needs from each installed plugin: ~300 bytes instead
 * of a full 3.5 KB catalogue entry. */
struct NDPluginSummary {
    char id[20];
    char name[ND_LABEL_LEN];
    char command[ND_COMMAND_LEN];
    char version[ND_MANIFEST_VERSION_LEN];
    int multiple_instances;
};
/* All scans read the drawers one plugin at a time and never hold more than
 * one full manifest.  Nothing here changes the configuration. */
int nd_plugin_catalog_scan(struct NDPluginCatalogEntry *entries,int maximum);
int nd_plugin_catalog_summaries(struct NDPluginSummary *out,int maximum);
/* 1 and the full entry (manifest included) if plugin ID is installed. */
int nd_plugin_catalog_lookup(const char *id,struct NDPluginCatalogEntry *out);
/* Read a text manifest (Plugins/<name>.ndplugin): 1 read, 0 invalid. */
int nd_manifest_read_file(const char *path,struct NDManifest *m);
/* Drawer that plugin packages are installed into: the first existing of
 * PROGDIR:Plugins, SYS:Utilities/NovaDock/Plugins and Bin:NovaDock/Plugins
 * (the drawers scanned for plugins), resolved to a full path.  0 if none. */
int nd_plugin_install_drawer(char *out,int size);
/* 1 if ID was added by this session's scan (shown as "New"). */
int nd_discovery_is_new(const char *id);
#endif
