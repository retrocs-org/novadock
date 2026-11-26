#ifndef ND_PLUGIN_LABELS_H
#define ND_PLUGIN_LABELS_H
#include <string.h>
#include <ctype.h>
/* Display names use uppercase ASCII; file paths, IDs and live data keep
 * their original bytes. This also covers third-party and custom tile names. */
static void nd_plugin_name_upper(char *name)
{
    while(*name){if(*name>='a' && *name<='z')*name=(char)(*name-'a'+'A');++name;}
}
static int nd_plugin_name_equal(const char *a,const char *b)
{
    while(*a && *b){
        unsigned char x=(unsigned char)*a++,y=(unsigned char)*b++;
        if(x>='a' && x<='z')x=(unsigned char)(x-'a'+'A');
        if(y>='a' && y<='z')y=(unsigned char)(y-'a'+'A');
        if(x!=y)return 0;
    }
    return !*a && !*b;
}
/* Upgrade only historical bundled names; user-defined labels are retained. */
static const char *nd_plugin_label_upgrade(const char *id,const char *label)
{
    static const struct {const char *id,*old,*name;} aliases[]={
        {"ARCHIVE","Extract","ARCHIVE"},{"ARCHIVE","Archiver","ARCHIVE"},
        {"SCREENMODE","Screen Mode","SMODE"},{"SCREENMODE","MODES","SMODE"},
        {"SCREENSHOT","Screenshot","SHOT"},{"SCREENSHOT","SCREENSHOW","SHOT"},
        {"SCREENSWITCHER","Screen Switcher","SCREENS"},
        {"WHDLOAD","WHDLoad Games","WHDLoad"},{"DOPUS4","DOpus 4","DOpus4"},
        {"RUNNINGAPPS","Running Apps","APPS"},
        {"MUSICLIB","Music Library","MUSIC"}
    };
    unsigned int i;
    for(i=0;i<sizeof(aliases)/sizeof(aliases[0]);++i){
        if(strcmp(id,aliases[i].id))continue;
        if(nd_plugin_name_equal(label,aliases[i].old))return aliases[i].name;
    }
    return NULL;
}
#endif
