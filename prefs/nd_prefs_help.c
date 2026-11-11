/* Help for NovaDock Preferences: NovaDock.guide in MultiView. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "nd_prefs_help.h"

/* DOS work only: each Preferences page names its own IntuitionBase, so
 * the beep on failure lives in the nd_prefs_help() macro. */
static int found(const char *path,char *out,int size)
{
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    int ok=0;
    if (lock) {
        ok=NameFromLock(lock,out,size)!=0;
        UnLock(lock);
    }
    return ok;
}

int nd_prefs_help_launch(void)
{
    /* Beside Preferences first (development drawers), then the places
     * NovaDock is installed. */
    static const char *guides[]={"PROGDIR:NovaDock.guide",
        "HELP:NovaDock.guide","HELP:NovaDock/NovaDock.guide","NovaDockHelp:NovaDock.guide","HELP:english/NovaDock.guide",
        "NovaDock:NovaDock.guide","SYS:Utilities/NovaDock/NovaDock.guide",
        "Bin:NovaDock/NovaDock.guide",NULL};
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR requesters=self->pr_WindowPtr;
    char guide[256],viewer[256],command[560];
    BPTR input,output;
    LONG rc;
    int i,ok=0;
    guide[0]='\0';
    self->pr_WindowPtr=(APTR)-1L;   /* missing volumes fail silently */
    for (i=0;guides[i] && !guide[0];++i) {
        if (i==0 && !GetProgramDir()) continue;
        if (!found(guides[i],guide,sizeof(guide))) guide[0]='\0';
    }
    ok=guide[0] && found("SYS:Utilities/MultiView",viewer,sizeof(viewer));
    self->pr_WindowPtr=requesters;
    if (!ok || strlen(viewer)+strlen(guide)+6>=sizeof(command)) return 0;
    strcpy(command,"\"");strcat(command,viewer);strcat(command,"\" \"");
    strcat(command,guide);strcat(command,"\"");
    input=Open("NIL:",MODE_OLDFILE);
    output=Open("NIL:",MODE_NEWFILE);
    if (!input || !output) {
        if (input) Close(input);
        if (output) Close(output);
        return 0;
    }
    rc=SystemTags((STRPTR)command,SYS_Input,input,SYS_Output,output,
                  SYS_Asynch,TRUE,NP_StackSize,16384,TAG_DONE);
    if (rc==-1) {Close(input);Close(output);return 0;}
    return 1;
}
