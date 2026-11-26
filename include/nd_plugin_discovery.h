#ifndef ND_PLUGIN_DISCOVERY_H
#define ND_PLUGIN_DISCOVERY_H
/* Preferences-side scan of the Plugins drawer for self-describing plugins
 * (see nd_manifest.h).  Results stay in the working configuration only; a
 * new plugin reaches novadock.cfg when the user adds it and uses Use/Save. */
#include "nd_config.h"
#include "nd_manifest.h"
#define ND_DISCOVERY_MAX 8
/* Discovery has its own bounded worker payload. Configurations also need
 * space for launchers, drawers and retained definitions. */
/* Matches the dock limit: every installed plugin must be able to appear in
 * Add Item and the Plugin Manager.  A full catalogue is reported in the UI. */
#define ND_PLUGIN_CATALOG_MAX ND_MAX_ITEMS
struct NDPluginCatalogEntry {
    struct NDManifest manifest;
    char command[ND_COMMAND_LEN];
    char manifest_path[ND_COMMAND_LEN];
    int bundled;
    char problem[96]; /* nonempty only in diagnostic scans */
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
/* Refresh bypasses cached health/metadata for the next complete scan. */
void nd_plugin_catalog_refresh(void);
/* Worker-only cooperative cancellation/progress; NULL restores normal scans. */
void nd_plugin_catalog_watch(int (*keep_scanning)(void *,int),void *context);
void nd_plugin_catalog_metrics(unsigned long *hits,unsigned long *misses);
int nd_discover_plugins(struct NDConfig *cfg,struct NDDiscovery *out);
/* Plugin lists retain small summaries instead of full option schemas. */
struct NDPluginSummary {
    char id[20];
    char name[ND_LABEL_LEN];
    char command[ND_COMMAND_LEN];
    char version[ND_MANIFEST_VERSION_LEN];
    int multiple_instances;
    char problem[96];
};
/* All scans read the drawers one plugin at a time and never hold more than
 * one full manifest.  Nothing here changes the configuration. */
int nd_plugin_catalog_scan(struct NDPluginCatalogEntry *entries,int maximum);
int nd_plugin_catalog_summaries(struct NDPluginSummary *out,int maximum);
/* Manager-only inventory, including unusable manifests, missing programs and damaged artwork.
 * Invalid entries must never be merged, launched or offered by Add Item. */
int nd_plugin_catalog_inventory(struct NDPluginSummary *out,int maximum);
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
