#ifndef ND_PLUGIN_LABELS_H
#define ND_PLUGIN_LABELS_H
#include <string.h>
#include <ctype.h>
/* Upgrade only historical bundled names; user-defined labels are retained. */
static const char *nd_plugin_label_upgrade(const char *id,const char *label)
{
    static const struct {const char *id,*old,*name;} aliases[]={
        {"ARCHIVE","Extract","ARCHIVE"},{"ARCHIVE","Archiver","ARCHIVE"},
        {"SCREENMODE","Screen Mode","SMODE"},{"SCREENMODE","MODES","SMODE"},
        {"SCREENSHOT","Screenshot","SHOT"},{"SCREENSHOT","SCREENSHOW","SHOT"},
        {"SCREENSWITCHER","Screen Switcher","SCREENS"},
        {"WHDLOAD","WHDLoad Games","WHDLoad"},{"DOPUS4","DOpus 4","DOpus4"},
        {"RUNNINGAPPS","Running Apps","APPS"}
    };
    unsigned int i;const char *a,*b;
    for(i=0;i<sizeof(aliases)/sizeof(aliases[0]);++i){
        if(strcmp(id,aliases[i].id))continue;
        a=label;b=aliases[i].old;
        while(*a && *b && toupper((unsigned char)*a)==toupper((unsigned char)*b)){++a;++b;}
        if(!*a && !*b)return aliases[i].name;
    }
    return NULL;
}
#endif
