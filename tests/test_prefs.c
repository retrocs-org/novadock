/* Pure C89 round-trip tests for 0.1 editor settings. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#include "nd_layout.h"
int main(void)
{
    struct NDConfig original, loaded, runtime;
    struct NDLayout layout;
    struct NDItem temp;
    FILE *marker;
    char stage[128],backup[128];
    int i;
    const char *path = "tests/nd_prefs_roundtrip.cfg";
    sprintf(stage,"%s.new",path);
    sprintf(backup,"%s.bak",path);
    marker=fopen(stage,"wb");
    assert(marker!=NULL);
    fputs("stale staging file\n",marker);
    fclose(marker);
    nd_default_config(&original);
    assert(nd_parse_line(&original, "PLUGIN=New|Plugins/New|NEWID|GRAPH|OFF") == 1);
    assert(original.count == 12);
    assert(original.items[11].graph && !original.items[11].enabled);
    assert(nd_parse_line(&original, "PLUGIN=Bad|Plugins/Bad|BA1|GRAPH|MAYBE") == 0);
    assert(nd_parse_line(&original, "PLUGIN=Bad|Plugins/Bad|BA2|GRAPH|ON|EXTRA") == 0);
    assert(nd_parse_line(&original, "PLUGIN=Old|Plugins/Old|OLDID") == 1);
    assert(original.items[12].enabled && !original.items[12].graph);
    original.side = ND_SIDE_TOP;
    original.position = ND_POSITION_END;
    original.font_mode = ND_FONT_BOLD;
    original.running_apps = 0;
    original.refresh_mode = ND_REFRESH_SMOOTH;
    original.rows = 2;
    original.label_mode = ND_LABEL_HOVER;
    original.item_size = ND_ITEM_SIZE_TINY;
    original.theme = ND_THEME_GRAPHITE;
    original.tile_width = 77;
    original.tile_height = 63;
    original.gap = 2;
    original.auto_hide = 1;
    original.hide_delay = 7;
    original.always_on_top = 1;
    original.items[0].enabled = 0; /* launcher retained as a definition */
    original.items[5].enabled = 0; /* Network disabled, still in editor */
    temp = original.items[4];
    original.items[4] = original.items[3];
    original.items[3] = temp; /* user reorders items */
    assert(nd_write_config(&original, path) == 1);
    assert((marker=fopen(stage,"rb"))==NULL);
    assert((marker=fopen(backup,"rb"))==NULL);
    assert(nd_load_config(&loaded, path) == 0);
    assert(loaded.count == original.count);
    assert(loaded.side == ND_SIDE_TOP && loaded.theme == ND_THEME_GRAPHITE);
    assert(loaded.position==ND_POSITION_END && loaded.font_mode==ND_FONT_BOLD);
    assert(loaded.running_apps==0);
    assert(loaded.refresh_mode==ND_REFRESH_SMOOTH);
    assert(loaded.rows==2);
    assert(loaded.label_mode==ND_LABEL_HOVER);
    assert(loaded.item_size==ND_ITEM_SIZE_TINY);
    assert(loaded.tile_width == 77 && loaded.tile_height == 63 && loaded.gap == 2);
    assert(loaded.auto_hide && loaded.hide_delay == 7);
    assert(loaded.always_on_top == 1);
    for (i = 0; i < loaded.count; ++i) {
        assert(loaded.items[i].kind == original.items[i].kind);
        assert(strcmp(loaded.items[i].label, original.items[i].label) == 0);
        assert(strcmp(loaded.items[i].command, original.items[i].command) == 0);
        assert(loaded.items[i].enabled == original.items[i].enabled);
        assert(loaded.items[i].graph == original.items[i].graph);
    }
    /* File replacement works on second save, retains changes. */
    loaded.items[5].enabled = 1;
    assert(nd_write_config(&loaded, path) == 1);
    assert((marker=fopen(stage,"rb"))==NULL);
    assert((marker=fopen(backup,"rb"))!=NULL);
    fclose(marker);
    assert(nd_load_config(&runtime,backup)==0);
    assert(runtime.items[5].enabled==0);
    assert(nd_load_config(&original, path) == 0);
    assert(original.items[5].enabled == 1);
    /* Live presentation data must not trigger a configuration rebuild. */
    runtime=original;
    runtime.items[0].online=1;
    runtime.items[0].meter=63;
    runtime.items[0].clicks=9;
    strcpy(runtime.items[0].caption,"LIVE");
    strcpy(runtime.items[0].value,"63%");
    assert(nd_config_runtime_equal(&original,&runtime));
    runtime.tile_width++;
    assert(!nd_config_runtime_equal(&original,&runtime));
    runtime=original;
    runtime.item_size=ND_ITEM_SIZE_NORMAL;
    assert(!nd_config_runtime_equal(&original,&runtime));
    runtime = original;
    assert(nd_compact_enabled(&runtime) == 6);
    for (i = 0; i < runtime.count; ++i) assert(runtime.items[i].enabled);
    assert(nd_layout_fit(&runtime, 800, 256, 11, &layout));
    assert(layout.width <= 800);
    nd_default_config(&loaded);
    assert(nd_parse_line(&loaded, "PLUGIN=Bad|Plugins/Bad|BA3|METER|OFF") == 1);
    assert(!loaded.items[loaded.count-1].enabled);
    remove(path);
    remove(stage);
    remove(backup);
    puts("Prefs config persistence, disabled tiles, and reordering tests passed.");
    return 0;
}
