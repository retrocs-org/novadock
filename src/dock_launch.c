/* NovaDock: Starting programs and companions; quiet requesters. */
#include "dock_internal.h"

/* Every asynchronous launch runs in a fresh shell that first sets an
 * explicit stack (and optional working directory): a shell created by a
 * Workbench-started NovaDock would otherwise run commands with 4 KB. */
int launch_command_at(const char *cmd,const char *directory,ULONG stack)
{
    BPTR input, output, lock;
    APTR requesters;
    char prepared[ND_COMMAND_LEN*4+64];
    char quoted[ND_COMMAND_LEN*2+4];
    LONG rc;
    if (!cmd || !*cmd) return 0;
    if (stack<ND_MIN_STACK) stack=ND_DEFAULT_STACK;
    if (stack>ND_MAX_STACK) stack=ND_MAX_STACK;
    sprintf(prepared,"Stack %lu\n",(unsigned long)stack);
    if (directory && *directory) {
        if (!nd_quote_path(quoted,sizeof(quoted),directory)) return 0;
        /* A working directory on an absent volume must fail quietly, not
         * stop the dock behind a "Please insert volume" requester. */
        requesters=quiet_requesters();
        lock=Lock((STRPTR)directory,ACCESS_READ);
        restore_requesters(requesters);
        if (!lock) return 0;
        UnLock(lock);
        strcat(prepared,"CD ");strcat(prepared,quoted);strcat(prepared,"\n");
    }
    if (strlen(prepared)+strlen(cmd)+1>sizeof(prepared)) return 0;
    strcat(prepared,cmd);
    input = Open("NIL:", MODE_OLDFILE);
    output = Open("NIL:", MODE_NEWFILE);
    if (!input || !output) {
        if (input) Close(input);
        if (output) Close(output);
        return 0;
    }
    rc = SystemTags((STRPTR)prepared,
            SYS_Input,input,SYS_Output,output,
            SYS_Asynch,TRUE,NP_StackSize,stack,TAG_DONE);
    /* On successful asynchronous SystemTags, the spawned shell owns handles. */
    if (rc == -1) { Close(input); Close(output); return 0; }
    return 1;
}
int launch_command(const char *cmd)
{
    return launch_command_at(cmd,NULL,ND_DEFAULT_STACK);
}
/* Stack recorded in a program's tool icon, as Workbench would use it. */
/* Like launch_command, but errors appear in a window that opens only when
 * the command prints something and then waits to be closed. */
int launch_command_console(const char *cmd)
{
    BPTR input,output;
    char prepared[ND_COMMAND_LEN*5+32];
    LONG rc;
    if (!cmd || !*cmd || strlen(cmd)+32>sizeof(prepared)) return 0;
    sprintf(prepared,"Stack %lu\n%s",(unsigned long)ND_DEFAULT_STACK,cmd);
    input=Open("NIL:",MODE_OLDFILE);
    output=Open("CON:20/40/560/120/NovaDock Copy/AUTO/CLOSE/WAIT",MODE_NEWFILE);
    if (!input || !output) {
        if (input) Close(input);
        if (output) Close(output);
        return 0;
    }
    rc=SystemTags((STRPTR)prepared,SYS_Input,input,SYS_Output,output,
                  SYS_Asynch,TRUE,NP_StackSize,ND_DEFAULT_STACK,TAG_DONE);
    if (rc==-1) {Close(input);Close(output);return 0;}
    return 1;
}
ULONG icon_stack(const char *path)
{
    struct DiskObject *icon;
    ULONG stack=ND_DEFAULT_STACK;
    if (!IconBase || !path || !*path) return stack;
    icon=GetDiskObject((STRPTR)path);
    if (icon) {
        if (icon->do_Type==WBTOOL && icon->do_StackSize>(LONG)stack)
            stack=(ULONG)icon->do_StackSize;
        FreeDiskObject(icon);
    }
    return stack;
}
/* The dock's own drawer comes first so a development copy uses its own
 * plugins and themes even when an installed copy exists.  The
 * install locations are only probes: with requesters quietened, a missing
 * Bin: assign fails silently instead of asking for a volume. */
void get_program_dir(struct NDDock *dock)
{
    static const char *candidates[]={
        "NovaDock:",              /* installed elsewhere (Install-NovaDock) */
        "SYS:NovaDock",
        "SYS:Resources/NovaDock",
        "SYS:Utilities/NovaDock",
        "Bin:NovaDock",
        NULL
    };
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR requesters=self->pr_WindowPtr;
    BPTR lock,plugins;
    char probe[360];
    int i;
    dock->directory[0] = '\0';
    self->pr_WindowPtr=(APTR)-1L;
    lock=GetProgramDir();
    if (lock && !NameFromLock(lock,dock->directory,sizeof(dock->directory)))
        dock->directory[0]='\0';
    if (dock->directory[0]) {
        strcpy(probe,dock->directory);
        if (!AddPart(probe,"Plugins",sizeof(probe))) dock->directory[0]=0;
        else {
            plugins=Lock(probe,ACCESS_READ);
            if (plugins) UnLock(plugins); else dock->directory[0]=0;
        }
    }
    for (i=0;!dock->directory[0] && candidates[i];++i) {
        strcpy(probe,candidates[i]);
        if (!AddPart(probe,"Plugins",sizeof(probe))) continue;
        plugins=Lock(probe,ACCESS_READ);
        if (!plugins) continue;
        UnLock(plugins);
        lock=Lock((STRPTR)candidates[i],ACCESS_READ);
        if (lock) {
            if (!NameFromLock(lock,dock->directory,sizeof(dock->directory)))
                dock->directory[0]='\0';
            UnLock(lock);
        }
    }
    self->pr_WindowPtr=requesters;
}
/* Preferences tools live beside the dock during development and in
 * SYS:Prefs after installation. */
int launch_companion(struct NDDock *dock, const char *name,
                            const char *args)
{
    char installed[96], local[360], command[800];
    const char *path = NULL;
    BPTR lock;
    APTR requesters;
    size_t len;
    if (!dock || !name || !*name || strchr(name, '/') || strchr(name, ':') ||
        strlen(name) + 15 > sizeof(installed))
        return 0;
    requesters = quiet_requesters();
    /* Installed Preferences uses its recorded drawer (SYS:Prefs by default).
     * Keep the package-local
     * executable as a fallback for machines without an installation. */
    sprintf(installed, "NovaDockPrefs:%s", name);
    lock = Lock(installed, ACCESS_READ);
    if (lock) { UnLock(lock); path = installed; }
    if (!path) {
        sprintf(installed, "SYS:Prefs/%s", name);
        lock = Lock(installed, ACCESS_READ);
        if (lock) { UnLock(lock); path = installed; }
    }
    len = strlen(dock->directory);
    if (!path && len && len + strlen(name) + 2 < sizeof(local)) {
        strcpy(local, dock->directory);
        if (local[len-1] != ':' && local[len-1] != '/') strcat(local, "/");
        strcat(local, name);
        lock = Lock(local, ACCESS_READ);
        if (lock) {
            UnLock(lock);
            path = local;
        }
    }
    if (!path) { restore_requesters(requesters); return 0; }
    restore_requesters(requesters);
    if (!nd_quote_path(command, sizeof(command), path)) return 0;
    if (args && *args) {
        if (strlen(command) + strlen(args) + 2 >= sizeof(command)) return 0;
        strcat(command, " ");
        strcat(command, args);
    }
    return launch_command(command);
}
int has_iff_extension(const char *path)
{
    size_t n = strlen(path);
    if (n < 4) return 0;
    path += n - 4;
    return path[0] == '.' && (path[1] == 'i' || path[1] == 'I') &&
           (path[2] == 'f' || path[2] == 'F') &&
           (path[3] == 'f' || path[3] == 'F');
}
int has_filename_extension(const char *path)
{
    const char *name,*dot;
    if (!path || !*path) return 0;
    name=strrchr(path,'/');
    if (!name) name=strrchr(path,':');
    name=name ? name+1 : path;
    dot=strrchr(name,'.');
    return dot && dot!=name && dot[1];
}
int has_info_extension(const char *path)
{
    size_t n = strlen(path);
    if (n < 5) return 0;
    path += n - 5;
    return path[0] == '.' && (path[1] == 'i' || path[1] == 'I') &&
           (path[2] == 'n' || path[2] == 'N') &&
           (path[3] == 'f' || path[3] == 'F') &&
           (path[4] == 'o' || path[4] == 'O');
}
/* Supports classic icon.library DiskObjects, compact IFF/ILBM brushes and,
 * when datatypes.library is present, any installed picture datatype.  Icon
 * paths are absolute (assign:) or relative to the installed dock. */
/* Absent volumes must never raise "Please insert volume" requesters from
 * background work (icon loading, meters, media scans).  pr_WindowPtr=-1
 * makes DOS fail such requests silently for this process. */
APTR quiet_requesters(void)
{
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR old=self->pr_WindowPtr;
    self->pr_WindowPtr=(APTR)-1L;
    return old;
}
void restore_requesters(APTR old)
{
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=old;
}
/* HOOK=EVENT|command lines: each matching command runs asynchronously with
 * the event name and a quoted detail (volume or plugin ID) appended, so one
 * script can serve several events.  Hooks never block the dock. */
void fire_hook(struct NDDock *dock,const char *event,const char *detail)
{
    char command[ND_HOOK_COMMAND_LEN+ND_HOOK_EVENT_LEN+ND_COMMAND_LEN+8];
    int i;
    if (!dock || !event) return;
    for (i=0;i<dock->conf.hook_count && i<ND_MAX_HOOKS;++i)
        if (!strcmp(dock->conf.hooks[i].event,event) &&
            nd_hook_command(command,sizeof(command),dock->conf.hooks[i].command,
                            event,detail))
            (void)launch_command(command);
}
