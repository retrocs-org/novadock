#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_plugin_labels.h"
int main(void)
{
    assert(!strcmp(nd_plugin_label_upgrade("ARCHIVE","Extract"),"ARCHIVE"));
    assert(!strcmp(nd_plugin_label_upgrade("SCREENMODE","Screen Mode"),"SMODE"));
    assert(!strcmp(nd_plugin_label_upgrade("SCREENSHOT","Screenshot"),"SHOT"));
    assert(!strcmp(nd_plugin_label_upgrade("SCREENSWITCHER","Screen Switcher"),"SCREENS"));
    assert(!strcmp(nd_plugin_label_upgrade("WHDLOAD","WHDLoad Games"),"WHDLoad"));
    assert(!strcmp(nd_plugin_label_upgrade("DOPUS4","DOpus 4"),"DOpus4"));
    assert(!strcmp(nd_plugin_label_upgrade("RUNNINGAPPS","Running Apps"),"APPS"));
    assert(!nd_plugin_label_upgrade("ARCHIVE","My backups"));
    assert(!nd_plugin_label_upgrade("THIRDPARTY","Extract"));
    assert(!nd_plugin_label_upgrade("ARCHIVE","ARCHIVE"));
    assert(!strcmp(nd_plugin_label_upgrade("MUSICLIB","Music Library"),"MUSIC"));
    assert(!nd_plugin_label_upgrade("MUSICLIB","SID tunes"));
    puts("Plugin labels migrate old defaults and preserve custom names and IDs.");return 0;
}
