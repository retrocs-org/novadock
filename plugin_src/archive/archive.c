/* NovaDock Archive Extractor: resident status tile plus short-lived helper. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <exec/memory.h>
#include <workbench/workbench.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/wb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_archive.h"

#define EXTRACT_CONSOLE \
    "CON:10/12/620/176/NovaDock Archive Extractor/AUTO/CLOSE"
#define ND_MAX_STATUS_FILES 8
#define ND_ARCHIVE_IDLE_TICKS 250U
#define ND_ARCHIVE_HEARTBEAT_TICKS 500U

const char nd_version[]=ND_VER_STRING("ArchiveExtractor");
/* Honoured by the vbcc and libnix startup code: the helper runs extractor
 * shells and stdio in a process that may otherwise get the 4 KB default. */
long __stack=16384;

struct Library *WorkbenchBase=NULL;
static char status_file[48];

static const char *find_tool(int kind,char *path,unsigned int size)
{
    static const char *zip[]={"C:UnZip","C:unzip",NULL};
    static const char *lha[]={"C:LhA","C:lha",NULL};
    static const char *rar[]={"C:UnRAR","C:unrar","C:RAR","C:rar",NULL};
    static const char *arj[]={"C:UnARJ","C:unarj","C:ARJ","C:arj",NULL};
    static const char *lzx[]={"C:LZX","C:lzx","C:UnLZX","C:unlzx",NULL};
    static const char *tar[]={"C:tar","C:Tar",NULL};
    static const char *gz[]={"C:gzip","C:GZip",NULL};
    const char **list;
    BPTR lock;
    int i;
    if (kind==ND_ARCHIVE_ZIP) list=zip;
    else if (kind==ND_ARCHIVE_LHA) list=lha;
    else if (kind==ND_ARCHIVE_RAR) list=rar;
    else if (kind==ND_ARCHIVE_ARJ) list=arj;
    else if (kind==ND_ARCHIVE_LZX) list=lzx;
    else if (kind==ND_ARCHIVE_TAR || kind==ND_ARCHIVE_TGZ) list=tar;
    else if (kind==ND_ARCHIVE_GZ) list=gz;
    else return NULL;
    for (i=0;list[i];++i) {
        lock=Lock((STRPTR)list[i],ACCESS_READ);
        if (lock) {
            UnLock(lock);
            if (strlen(list[i])>=size) return NULL;
            strcpy(path,list[i]);
            return path;
        }
    }
    return NULL;
}
static int available_tools(void)
{
    char tool[32];
    int available=0,kind;
    for (kind=ND_ARCHIVE_ZIP;kind<=ND_ARCHIVE_LAST;++kind)
        if (find_tool(kind,tool,sizeof(tool))) ++available;
    return available;
}
static void write_status(const char *text)
{
    FILE *file;
    if (!status_file[0]) return;
    file=fopen(status_file,"w");
    if (!file) return;
    fputs(text,file);fputc('\n',file);fclose(file);
}
static int read_status_file(const char *name,char *text,unsigned int size)
{
    FILE *file;
    char *end;
    if (!text || size<2) return 0;
    file=fopen(name,"r");
    if (!file) return 0;
    if (!fgets(text,(int)size,file)) {fclose(file);return 0;}
    fclose(file);
    end=strchr(text,'\n');if (end) *end='\0';
    return *text!='\0';
}
/* Summarise every helper's status file.  Running extractions are counted;
 * finished ones report their result once and are then removed. */
static int collect_status(char *value,unsigned int size,int *running)
{
    struct AnchorPath *anchor;
    char names[ND_MAX_STATUS_FILES][48],text[32],result[32];
    int count=0,active=0,i,have_result=0;
    anchor=(struct AnchorPath *)AllocVec(sizeof(struct AnchorPath)+64,
                                         MEMF_PUBLIC|MEMF_CLEAR);
    *running=0;
    if (!anchor) return 0;
    anchor->ap_Strlen=0;
    if (MatchFirst((STRPTR)ND_ARCHIVE_STATUS_PATTERN,anchor)==0) {
        do {
            if (count<ND_MAX_STATUS_FILES &&
                strlen(anchor->ap_Info.fib_FileName)+3<sizeof(names[0])) {
                strcpy(names[count],"T:");
                strcat(names[count],anchor->ap_Info.fib_FileName);
                ++count;
            }
        } while (MatchNext(anchor)==0);
    }
    MatchEnd(anchor);
    FreeVec(anchor);
    for (i=0;i<count;++i) {
        if (!read_status_file(names[i],text,sizeof(text))) continue;
        if (!strcmp(text,"EXTRACTING")) ++active;
        else {
            if (!have_result) {strcpy(result,text);have_result=1;}
            remove(names[i]);
        }
    }
    *running=active;
    if (active>1) {sprintf(value,"EXTRACTING %d",active);return 2;}
    if (active==1) {strcpy(value,"EXTRACTING");return 2;}
    if (have_result && strlen(result)<size) {strcpy(value,result);return 1;}
    return 0;
}
static void wait_and_close_console(BPTR console)
{
    char response[8];
    if (!console) return;
    FPuts(console,"\nPress RETURN or click the close gadget to exit.\n");
    Flush(console);
    /* CON: input is line buffered. Closing the window also releases this
     * read, so both the keyboard and the close gadget dismiss the result. */
    (void)FGets(console,response,sizeof(response));
    Close(console);
}
static int ask_yes(BPTR console,const char *question)
{
    char response[8];
    FPuts(console,(STRPTR)question);Flush(console);
    if (!FGets(console,response,sizeof(response))) return 0;
    return response[0]=='y' || response[0]=='Y';
}
static int refresh_workbench_parent(const char *destination)
{
    BPTR object,parent;
    STRPTR name;
    int refreshed=0;
    if (!destination || !*destination) return 0;
    object=Lock((STRPTR)destination,ACCESS_READ);
    if (!object) return 0;
    parent=ParentDir(object);
    UnLock(object);
    if (!parent) return 0;
    name=FilePart((STRPTR)destination);
    /* UpdateWorkbench() is public from workbench.library V44 (OS 3.5). */
    WorkbenchBase=OpenLibrary("workbench.library",44);
    if (WorkbenchBase) {
        if (name && *name) {
            UpdateWorkbench(name,parent,UPDATEWB_ObjectAdded);
            refreshed=1;
        }
        CloseLibrary(WorkbenchBase);
        WorkbenchBase=NULL;
    }
    UnLock(parent);
    return refreshed;
}
/* SelectError() exists only in dos.library V47 (OS 3.2); calling it on
 * 3.0-3.9 jumps through a missing vector.  Older systems get the same
 * effect by swapping the process's error stream directly. */
static BPTR select_error_stream(BPTR stream)
{
    struct Process *self;
    BPTR old;
    if (((struct Library *)DOSBase)->lib_Version>=47)
        return (BPTR)SelectError(stream);
    self=(struct Process *)FindTask(NULL);
    old=self->pr_CES;
    self->pr_CES=stream;
    return old;
}
static LONG run_in_console(const char *command,BPTR console)
{
    BPTR old_error;
    LONG result;
    /* AmigaDOS requires distinct input/output handles. Passing this CON: as
     * input and NULL as output makes SystemTags open '*' on the same console,
     * keeping both extractor progress and any interactive input visible. */
    old_error=select_error_stream(console);
    result=SystemTags((STRPTR)command,
                      SYS_Input,console,SYS_Output,0,
                      SYS_UserShell,TRUE,
                      NP_StackSize,16384,TAG_DONE);
    (void)select_error_stream(old_error);
    return result;
}
static int extract_archive(const char *archive)
{
    char destination[260],tool[32],command[900];
    int kind;
    BPTR lock,console,created;
    LONG result;
    nd_archive_status_name((unsigned long)FindTask(NULL),status_file,
                           sizeof(status_file));
    kind=nd_archive_kind(archive);
    if (!nd_archive_destination(archive,destination,sizeof(destination))) {
        write_status("UNSUPPORTED");return 20;
    }
    console=Open(EXTRACT_CONSOLE,MODE_OLDFILE);
    if (!console) {write_status("WINDOW FAILED");return 20;}
    FPrintf(console,"NovaDock Archive Extractor\n\nArchive: %s\nDestination: %s\n",
            archive,destination);
    if (!find_tool(kind,tool,sizeof(tool))) {
        write_status("TOOL MISSING");
        FPrintf(console,"\nERROR: No compatible extractor was found in C:.\n"
                "Install %s to unpack this archive.\n",
                (LONG)nd_archive_tool_names(kind));
        wait_and_close_console(console);return 20;
    }
    FPrintf(console,"Extractor: %s\n\n",tool);
    lock=Lock((STRPTR)destination,ACCESS_READ);
    if (lock) {
        UnLock(lock);write_status("FOLDER EXISTS");
        FPuts(console,"ERROR: The destination drawer already exists.\n");
        wait_and_close_console(console);return 20;
    }
    created=CreateDir((STRPTR)destination);
    if (!created) {
        write_status("CREATE FAILED");
        FPuts(console,"ERROR: The destination drawer could not be created.\n");
        wait_and_close_console(console);return 20;
    }
    UnLock(created);
    if (!nd_archive_command(kind,tool,archive,destination,command,sizeof(command))) {
        (void)DeleteFile((STRPTR)destination);
        write_status("INVALID PATH");
        FPuts(console,"ERROR: The extraction command could not be built.\n");
        wait_and_close_console(console);return 20;
    }
    write_status("EXTRACTING");
    FPuts(console,"Starting extraction...\n\n");Flush(console);
    result=run_in_console(command,console);
    if (result!=0 && !DeleteFile((STRPTR)destination)) {
        /* DeleteFile only succeeds on an empty drawer.  Offer to remove a
         * partial extraction so a corrected retry starts clean. */
        FPrintf(console,"\nExtraction failed (return code %ld).\n",result);
        if (ask_yes(console,"Remove the partially extracted drawer? (y/N) ")) {
            sprintf(command,"C:Delete \"%s\" ALL QUIET FORCE",destination);
            if (run_in_console(command,console)==0)
                FPuts(console,"Partial drawer removed.\n");
            else FPuts(console,"The drawer could not be removed completely.\n");
        } else FPuts(console,"The partial drawer was kept for inspection.\n");
        write_status("FAILED");
        wait_and_close_console(console);
        return 20;
    }
    write_status(result==0 ? "DONE" : "FAILED");
    if (result==0) {
        if (refresh_workbench_parent(destination))
            FPuts(console,"\nWorkbench directory refreshed.\n");
        FPuts(console,"\nExtraction finished successfully.\n");
    } else
        FPrintf(console,"\nExtraction failed (return code %ld).\n",result);
    wait_and_close_console(console);
    return result==0 ? 0 : 20;
}
int main(int argc,char **argv)
{
    struct NDClient client;
    unsigned int event;
    char value[32];
    int available=0,hold=0;
    unsigned int poll_ticks=50,elapsed_ticks=0,delay_ticks;
    char option[32];
    if (argc>=4 && !strcmp(argv[2],"EXTRACT")) return extract_archive(argv[3]);
    if (argc<2 || !nd_client_init(&client,argv[1])) return 20;
    available=available_tools();
    nd_plugin_option_string("ARCHIVE","POLL_TICKS","50",option,sizeof(option));
    poll_ticks=(unsigned int)atoi(option);
    if (poll_ticks<5 || poll_ticks>250) poll_ticks=50;
    strcpy(value,available ? "DROP HERE" : "NO TOOLS");
    for (;;) {
        char status[32],badge[4];
        int running;
        int state=collect_status(status,sizeof(status),&running);
        if (state) {
            strcpy(value,status);
            if (state==1) hold=8;
        } else if (hold>0) --hold;
        else strcpy(value,available ? "DROP HERE" : "NO TOOLS");
        event=nd_client_update_quiet(&client,"EXTRACT",value,ND_NO_METER,
                                     elapsed_ticks,ND_ARCHIVE_HEARTBEAT_TICKS);
        /* Badge: how many extractions are running (none: no badge). */
        if (running<=0) badge[0]='\0';
        else sprintf(badge,"%d",running>99 ? 99 : running);
        if (!(event&ND_EVENT_QUIT)) event|=nd_client_set_badge(&client,badge);
        if (event&ND_EVENT_QUIT) break;
        if (event&ND_EVENT_SETTINGS) {
            available=available_tools();
            nd_plugin_option_string("ARCHIVE","POLL_TICKS","50",option,
                                    sizeof(option));
            poll_ticks=(unsigned int)atoi(option);
            if (poll_ticks<5 || poll_ticks>250) poll_ticks=50;
        }
        /* MatchFirst() over T: is useful during an extraction but expensive
         * as a permanent idle poll.  The dock wakes this resident immediately
         * after a drop; otherwise an idle check every five seconds is enough. */
        delay_ticks=(state || hold>0) ? poll_ticks : ND_ARCHIVE_IDLE_TICKS;
        delay_ticks=nd_client_delay_ticks(&client,delay_ticks);
        elapsed_ticks=nd_client_wait(&client,delay_ticks);
    }
    nd_client_close(&client);
    return 0;
}
