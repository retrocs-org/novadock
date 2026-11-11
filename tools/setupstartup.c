/* Optional setup: create a WBStartup PROJECT alias launching the real tool.
 * No binary duplicated in WBStartup; Workbench uses the project's DefaultTool.
 * Run after copying NovaDock to SYS:Utilities/NovaDock.
 * Existing entries keep their target, artwork and other tool types.
 */
#include <exec/types.h>
#include <workbench/workbench.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/dos.h>
#include "nd_version.h"
#include <stdlib.h>

/* Version string for the AmigaDOS Version command (kept global so the
 * linker never discards it). */
const char nd_version[]=ND_VER_STRING("SetupStartup");
struct Library *IconBase = NULL;
/* SetupStartup [PROGRAM]: PROGRAM is the installed NovaDock (default
 * SYS:Utilities/NovaDock); the installer passes the location the
 * user chose. */
int main(int argc, char **argv)
{
    struct DiskObject *disk;
    STRPTR previous, *previous_types, *types;
    int count=0, i, existing=0;
    BPTR f, dir;
    int result = 10, created_marker = 0;
    const char *alias = "SYS:WBStartup/NovaDock";
    const char *program = argc > 1 && argv[1][0] ? argv[1] :
                          "SYS:Utilities/NovaDock";
    IconBase = OpenLibrary("icon.library", 36);
    if (!IconBase) return 20;
    dir = CreateDir("SYS:WBStartup");
    if (dir) UnLock(dir);
    disk = GetDiskObject((STRPTR)alias);
    if (disk) {
        if (FindToolType(disk->do_ToolTypes,(STRPTR)"DONOTWAIT")) {
            FreeDiskObject(disk);result=0;goto done;
        }
        existing=1;
    }
    if (!existing) {
    /* Use a harmless marker file, which Workbench opens via DefaultTool. */
    f = Open((STRPTR)alias, MODE_NEWFILE);
    if (!f) goto done;
    Close(f);
    created_marker = 1;
    disk = GetDefDiskObject(WBPROJECT);
    if (!disk) goto done;
    }
    previous = disk->do_DefaultTool;
    previous_types = disk->do_ToolTypes;
    if (previous_types) while (previous_types[count]) ++count;
    types=(STRPTR *)malloc((count+2)*sizeof(STRPTR));
    if (!types) {FreeDiskObject(disk);goto done;}
    for (i=0;i<count;++i) types[i]=previous_types[i];
    types[count]=(STRPTR)"DONOTWAIT";
    types[count+1]=NULL;
    disk->do_ToolTypes=types;
    if (!existing) disk->do_DefaultTool = (STRPTR)program;
    disk->do_StackSize = 16384;
    if (PutDiskObject((STRPTR)alias, disk)) {
        Printf("WBStartup project shortcut installed.\n");
        result = 0;
    }
    disk->do_DefaultTool = previous;
    disk->do_ToolTypes=previous_types;
    free(types);
    FreeDiskObject(disk);
done:
    if (result != 0 && created_marker) DeleteFile((STRPTR)alias);
    CloseLibrary(IconBase);
    return result;
}
