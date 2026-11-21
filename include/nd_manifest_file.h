#ifndef ND_MANIFEST_FILE_H
#define ND_MANIFEST_FILE_H
#include <stdio.h>
#include <string.h>
#include "nd_manifest.h"
/* Installed-file parsing shared by Preferences and release health checks. */
static int nd_manifest_read_text(const char *path,struct NDManifest *m)
{
    static char line[512];
    FILE *file=fopen(path,"r");
    int ok=1,ch;
    if (!file) {strcpy(m->error,"manifest cannot be read");return 0;}
    while (fgets(line,sizeof(line),file)) {
        if (!strchr(line,'\n') && !feof(file)) {
            while ((ch=fgetc(file))!='\n' && ch!=EOF) {}
            strcpy(m->error,"manifest line too long");ok=0;break;
        }
        if (nd_manifest_line(m,line)==0) {ok=0;break;}
    }
    if (ferror(file)) {strcpy(m->error,"manifest read failed");ok=0;}
    fclose(file);
    return ok;
}

#endif
