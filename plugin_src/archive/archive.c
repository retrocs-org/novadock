/* NovaDock Archiver: resident status tile plus short-lived helper. */
#include <exec/types.h>
#include <exec/execbase.h>
#include <ctype.h>
#include <dos/dos.h>
#include <dos/var.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <exec/memory.h>
#include <workbench/workbench.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/wb.h>
#include <proto/icon.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_archive.h"
#include "nd_archive_dialog.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif

#define EXTRACT_CONSOLE \
    "CON:10/12/620/176/NovaDock Archiver/AUTO/CLOSE"
#define ND_MAX_STATUS_FILES 8
#define ND_ARCHIVE_IDLE_TICKS 250U
#define ND_ARCHIVE_HEARTBEAT_TICKS 500U

const char nd_version[]=ND_PLUGIN_VER_STRING(ARCHIVE,"ArchiveExtractor");
/* Honoured by the vbcc and libnix startup code: the helper runs extractor
 * shells and stdio in a process that may otherwise get the 4 KB default. */
long __stack=16384;

struct Library *WorkbenchBase=NULL;
static char status_file[48];
static char job_name[ND_TEXT_LEN],status_subject[ND_TEXT_LEN];
struct Library *IconBase=NULL;

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
static unsigned long current_archive_module(void)
{
    struct Process *self=(struct Process *)FindTask(NULL);
    struct CommandLineInterface *cli;
    if(!self || self->pr_Task.tc_Node.ln_Type!=NT_PROCESS || !self->pr_CLI)return 0;
    cli=(struct CommandLineInterface *)BADDR(self->pr_CLI);
    return (unsigned long)cli->cli_Module;
}
/* Called only for a task found in Exec's live lists while scheduling is
 * disabled. Never dereference the process address from a status filename. */
static int status_task_matches(struct Task *task,unsigned long owner,unsigned long module)
{
    struct Process *process;struct CommandLineInterface *cli;
    const unsigned char *command;char name[260],*base;unsigned int n,i;
    static const char expected[]="ArchiveExtractor";
    if((unsigned long)task!=owner || task->tc_Node.ln_Type!=NT_PROCESS)return 0;
    process=(struct Process *)task;if(!process->pr_CLI)return 0;
    cli=(struct CommandLineInterface *)BADDR(process->pr_CLI);
    if(module)return (unsigned long)cli->cli_Module==module;
    /* Compatibility with status files written before the module token was
     * added: reject a task address reused by another CLI command. */
    if(!cli->cli_CommandName)return 0;
    command=(const unsigned char *)BADDR(cli->cli_CommandName);n=command[0];
    memcpy(name,command+1,n);name[n]=0;base=FilePart(name);
    if(strlen(base)!=sizeof(expected)-1)return 0;
    for(i=0;expected[i];++i)
        if(tolower((unsigned char)base[i])!=tolower((unsigned char)expected[i]))return 0;
    return 1;
}
static int status_owner_alive(const char *path,unsigned long module)
{
    const char *name=path;char *end;unsigned long owner;struct Node *node;int alive;
    if(!strncmp(name,"T:",2))name+=2;
    if(strncmp(name,"NovaDockArchive-",16))return 0;
    owner=strtoul(name+16,&end,16);
    if(!owner || strcmp(end,".status"))return 0;
    Forbid();
    alive=status_task_matches(FindTask(NULL),owner,module);
    for(node=SysBase->TaskReady.lh_Head;!alive && node->ln_Succ;node=node->ln_Succ)
        alive=status_task_matches((struct Task *)node,owner,module);
    for(node=SysBase->TaskWait.lh_Head;!alive && node->ln_Succ;node=node->ln_Succ)
        alive=status_task_matches((struct Task *)node,owner,module);
    Permit();return alive;
}
static void write_status(const char *text)
{
    FILE *file;
    char temporary[56];
    struct MsgPort *port;
    if (!status_file[0]) return;
    sprintf(temporary,"%s.tmp",status_file);
    file=fopen(temporary,"w");if(!file)return;
    fprintf(file,"%s\t%s\t%lx\n",text,job_name,current_archive_module());
    if(fclose(file)!=0){remove(temporary);return;}
    /* Publish only a complete status line, then wake the resident. */
    remove(status_file);
    if(rename(temporary,status_file)!=0){remove(temporary);return;}
    Forbid();
    port=FindPort((STRPTR)ND_WAKE_PORT_PREFIX "ARCHIVE");
    if(port && port->mp_SigTask)Signal(port->mp_SigTask,ND_WAKE_SIGNAL);
    Permit();
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
    char names[ND_MAX_STATUS_FILES][48],text[96],result[32],subject[ND_TEXT_LEN],*detail,*token,*end;
    unsigned long module;
    int count=0,active=0,packing=0,unpacking=0,i,have_result=0;
    anchor=(struct AnchorPath *)AllocVec(sizeof(struct AnchorPath)+64,
                                         MEMF_PUBLIC|MEMF_CLEAR);
    *running=0;status_subject[0]=0;subject[0]=0;
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
        detail=strchr(text,'\t');if(detail)*detail++=0;
        module=0;token=detail ? strchr(detail,'\t'):NULL;
        if(token){
            *token++=0;module=strtoul(token,&end,16);
            if(end==token || *end){remove(names[i]);continue;}
        }
        if(strlen(text)>=sizeof(result))continue;
        if(!active && detail){strncpy(subject,detail,sizeof(subject)-1);subject[sizeof(subject)-1]=0;}
        if((!strcmp(text,"EXTRACTING") || !strcmp(text,"COMPRESSING") ||
            !strcmp(text,"OPTIONS")) && !status_owner_alive(names[i],module)){
            if(!have_result){strcpy(result,"INTERRUPTED");have_result=1;}
            remove(names[i]);continue;
        }
        if (!strcmp(text,"EXTRACTING")) {++active;++unpacking;}
        else if (!strcmp(text,"COMPRESSING")) {++active;++packing;}
        else if (!strcmp(text,"OPTIONS")) {++active;}
        else {
            if (!have_result) {strcpy(result,text);have_result=1;}
            remove(names[i]);
        }
    }
    *running=active;
    if(active>1)strcpy(status_subject,"Multiple items");
    else strcpy(status_subject,subject);
    if (active) {
        const char *state=packing && unpacking ? "WORKING" :
                          packing ? "COMPRESSING" : unpacking ? "EXTRACTING":"OPTIONS";
        if(active>1)sprintf(value,"%s %d",state,active);
        else strcpy(value,state);
        return 2;
    }
    if (have_result && strlen(result)<size) {strcpy(value,result);return 1;}
    return 0;
}
static char completed_destination[420];
static int open_completed_destination(void)
{
    int ok=0;
    if(!completed_destination[0])return 0;
    WorkbenchBase=OpenLibrary("workbench.library",44);
    if(WorkbenchBase){
        ok=OpenWorkbenchObjectA((STRPTR)completed_destination,NULL)!=0;
        CloseLibrary(WorkbenchBase);WorkbenchBase=NULL;
    }
    return ok;
}
static void completed_at(const char *path,int drawer,BPTR console)
{
    BPTR lock,parent;char value[8];LONG n;
    completed_destination[0]=0;
    lock=Lock((STRPTR)path,ACCESS_READ);if(!lock)return;
    parent=drawer ? lock:ParentDir(lock);
    if(parent){
        if(!NameFromLock(parent,completed_destination,sizeof(completed_destination)))completed_destination[0]=0;
        if(!drawer)UnLock(parent);
    }
    UnLock(lock);
    n=GetVar("NovaDock/ArchiveOpenDestination",value,sizeof(value),GVF_GLOBAL_ONLY);
    if(n==3 && !strncmp(value,"YES",3) && !open_completed_destination())
        FPuts(console,"The destination could not be opened. Workbench V44 or later is required.\n");
}
static void wait_and_close_console(BPTR console)
{
    struct NDArchiveCloseInput input;
    unsigned char bytes[128];
    char response[8];
    LONG n,i;int done=0;
    if(!console)return;
    FPuts(console,"\nClick inside this window, press RETURN, or use the close gadget to exit.\n");
    if(completed_destination[0])FPuts(console,"Press O to open the destination drawer.\n");
    Flush(console);
    if(SetMode(console,1)){
        memset(&input,0,sizeof(input));
        /* Raw events are requested only after the external command exits.
         * The console owns its window and input; no IDCMP port is borrowed. */
        FPuts(console,"\2332;11{");Flush(console);
        while(!done && (n=Read(console,bytes,sizeof(bytes)))>0)
            for(i=0;i<n;++i){
                if(!input.state && (bytes[i]=='o' || bytes[i]=='O') && completed_destination[0]){
                    if(!open_completed_destination())FPuts(console,"Cannot open the destination drawer.\n");
                }else if(nd_archive_close_input(&input,bytes[i])){done=1;break;}
            }
        FPuts(console,"\2332;11}");Flush(console);
        (void)SetMode(console,0);
    }else (void)FGets(console,response,sizeof(response));
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
/* Keep user-supplied icons; add a project/drawer icon only when absent. */
static int ensure_workbench_icon(const char *path,int drawer)
{
    struct DiskObject *icon;BPTR lock;char info[530];int ok=0;
    if(strlen(path)+6>=sizeof(info))return 0;
    strcpy(info,path);strcat(info,".info");
    lock=Lock(info,ACCESS_READ);if(lock){UnLock(lock);return 1;}
    IconBase=OpenLibrary("icon.library",36);if(!IconBase)return 0;
    icon=GetDefDiskObject(drawer ? WBDRAWER:WBPROJECT);
    if(icon){
        icon->do_CurrentX=NO_ICON_POSITION;icon->do_CurrentY=NO_ICON_POSITION;
        ok=PutDiskObject((STRPTR)path,icon)!=0;FreeDiskObject(icon);
    }
    CloseLibrary(IconBase);IconBase=NULL;return ok;
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
    BPTR old_error,old_dir=0,directory=0;
    LONG result;
    const char *line;
    char drawer[260];
    unsigned int length;
    /* SystemTags accepts one command. Set the helper's working directory
     * explicitly instead of feeding a CD plus a second shell command. */
    if(!strncmp(command,"CD \"",4)) {
        line=strchr(command,'\n');
        if(!line || line<=command+5 || line[-1]!='"')return 20;
        length=(unsigned int)(line-command)-5;
        if(length>=sizeof(drawer))return 20;
        memcpy(drawer,command+4,length);drawer[length]=0;
        directory=Lock(drawer,ACCESS_READ);if(!directory)return 20;
        old_dir=CurrentDir(directory);command=line+1;
    }
    old_error=select_error_stream(console);
    result=SystemTags((STRPTR)command,
                      SYS_Input,console,SYS_Output,0,
                      SYS_UserShell,TRUE,
                      NP_StackSize,16384,TAG_DONE);
    (void)select_error_stream(old_error);
    if(directory){CurrentDir(old_dir);UnLock(directory);}
    return result;
}
static void print_pair(BPTR console,const char *format,LONG first,LONG second)
{
    LONG args[2];args[0]=first;args[1]=second;
    VFPrintf(console,(STRPTR)format,args);
}
static int extract_archive(const char *archive,int detected_kind)
{
    char destination[260],tool[32],command[900];
    int kind,make=1,selected,icon=1,warning=0;
    BPTR lock,console,created;
    LONG result;
    nd_archive_status_name((unsigned long)FindTask(NULL),status_file,
                           sizeof(status_file));
    kind=detected_kind;
    write_status("OPTIONS");
    selected=nd_archive_extraction_request(archive,destination,sizeof(destination),&make,&icon);
    if(selected<=0){write_status(selected==0 ? "CANCELLED":"WINDOW FAILED");return selected==0 ? 0:20;}
    console=Open(EXTRACT_CONSOLE,MODE_OLDFILE);
    if (!console) {write_status("WINDOW FAILED");return 20;}
    print_pair(console,"NovaDock Archiver\n\nArchive: %s\nDestination: %s\n",
            (LONG)archive,(LONG)destination);
    if (!find_tool(kind,tool,sizeof(tool))) {
        write_status("TOOL MISSING");
        print_pair(console,"\nERROR: No compatible extractor was found in C:.\n"
                "Install %s to unpack this archive.\n",
                (LONG)nd_archive_tool_names(kind),0);
        wait_and_close_console(console);return 20;
    }
    print_pair(console,"Extractor: %s\n\n",(LONG)tool,0);
    if(make){
        lock=Lock((STRPTR)destination,ACCESS_READ);
        if(lock){UnLock(lock);write_status("FOLDER EXISTS");
            FPuts(console,"ERROR: The destination drawer already exists.\n");wait_and_close_console(console);return 20;}
        created=CreateDir((STRPTR)destination);
        if(!created){write_status("CREATE FAILED");FPuts(console,"ERROR: The destination drawer could not be created.\n");wait_and_close_console(console);return 20;}
        UnLock(created);
    }
    if (!nd_archive_command(kind,tool,archive,destination,command,sizeof(command))) {
        if(make)(void)DeleteFile((STRPTR)destination);
        write_status("INVALID PATH");
        FPuts(console,"ERROR: The extraction command could not be built.\n");
        wait_and_close_console(console);return 20;
    }
    write_status("EXTRACTING");
    FPuts(console,"Starting extraction...\n\n");Flush(console);
    result=run_in_console(command,console);
    /* Info-ZIP result 1 means processing completed with warnings. Keep the
     * output and make warnings visible; reserve failed-output cleanup for
     * actual errors. Other unpackers retain their own success convention. */
    warning=kind==ND_ARCHIVE_ZIP && result==1;
    if (result!=0 && !warning && make && !DeleteFile((STRPTR)destination)) {
        /* DeleteFile only succeeds on an empty drawer.  Offer to remove a
         * partial extraction so a corrected retry starts clean. */
        print_pair(console,"\nExtraction failed (return code %ld).\n",result,0);
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
    write_status(warning ? "WARNINGS":result==0 ? "DONE" : "FAILED");
    if (result==0 || warning) {
        if(make && icon && !ensure_workbench_icon(destination,1))
            FPuts(console,"WARNING: The drawer was extracted but its icon could not be created.\n");
        if (refresh_workbench_parent(destination))
            FPuts(console,"\nWorkbench directory refreshed.\n");
        completed_at(destination,1,console);
        FPuts(console,warning ? "\nExtraction finished with warnings. Review the unpacker messages; some entries may have been skipped.\n":
                      "\nExtraction finished successfully.\n");
    } else
        print_pair(console,"\nExtraction failed (return code %ld).\n",result,0);
    wait_and_close_console(console);
    return warning ? 5:result==0 ? 0 : 20;
}
static int find_compressor(int kind,char *tool,unsigned int size)
{
    const char *name=kind==ND_ARCHIVE_ZIP ? "C:Zip" :
                     kind==ND_ARCHIVE_LHA ? "C:LhA" : NULL;
    BPTR lock;
    if(!name || strlen(name)>=size)return 0;
    lock=Lock((STRPTR)name,ACCESS_READ);if(!lock)return 0;
    UnLock(lock);strcpy(tool,name);return 1;
}
/* A source drawer's icon lives beside the drawer, outside recursion. */
static int source_icon_exists(const char *source)
{
    char path[266];BPTR lock;struct FileInfoBlock *fib;int found=0;
    if(strlen(source)+6>sizeof(path))return 0;
    strcpy(path,source);strcat(path,".info");
    lock=Lock(path,ACCESS_READ);if(!lock)return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if(fib){found=Examine(lock,fib) && fib->fib_DirEntryType<0;FreeDosObject(DOS_FIB,fib);}
    UnLock(lock);return found;
}
static int compress_sources(struct NDArchiveSource *sources,unsigned int count,int require_name)
{
    char output[420],parent[420],stage_dir[480],stage[520],tool[32],command[1400];
    BPTR console,lock;LONG result;
    int mask=0,kind=ND_ARCHIVE_ZIP,selected,i,created=0,icon=1;
    unsigned int item;
    char *part;
    if(require_name)sprintf(job_name,"%u items",count);
    nd_archive_status_name((unsigned long)FindTask(NULL),status_file,sizeof(status_file));
    if(find_compressor(ND_ARCHIVE_ZIP,tool,sizeof(tool)))mask|=1;
    if(find_compressor(ND_ARCHIVE_LHA,tool,sizeof(tool)))mask|=2;
    if(!mask){
        write_status("TOOL MISSING");console=Open(EXTRACT_CONSOLE,MODE_OLDFILE);
        if(console){FPuts(console,"Install C:Zip or C:LhA to create archives.\n");wait_and_close_console(console);}
        return 20;
    }
    write_status("OPTIONS");
    selected=nd_archive_compression_request(sources,count,require_name,mask,&kind,output,sizeof(output),&icon);
    if(selected<=0){write_status(selected==0 ? "CANCELLED":"WINDOW FAILED");return selected==0 ? 0:20;}
    console=Open(EXTRACT_CONSOLE,MODE_OLDFILE);
    if(!console){write_status("WINDOW FAILED");return 20;}
    print_pair(console,"NovaDock Archiver\n\nArchive: %s\n",(LONG)output,0);
    for(item=0;item<count;++item)
        if(!nd_archive_output_allowed(sources[item].path,sources[item].drawer,output))break;
    if(item<count){
        write_status("INVALID DESTINATION");
        FPuts(console,"Choose a destination outside the source drawer, with a different filename.\n");
        wait_and_close_console(console);return 20;
    }
    if(!find_compressor(kind,tool,sizeof(tool))){write_status("TOOL MISSING");wait_and_close_console(console);return 20;}
    /* Reserve a private directory atomically. The compressor never writes
     * directly to an existing/user-selected archive. Rename publishes only
     * a successful result and refuses an archive created by another task. */
    strcpy(parent,output);part=PathPart(parent);*part='\0';
    for(i=0;i<20;++i){
        sprintf(stage_dir,"%s%sNovaDockArchive-%08lx-%d",parent,
                parent[strlen(parent)-1]==':' ? "":"/",(unsigned long)FindTask(NULL),i);
        lock=CreateDir(stage_dir);
        if(lock){UnLock(lock);created=1;break;}
    }
    if(!created){write_status("CREATE FAILED");FPuts(console,"Cannot create a temporary output drawer.\n");wait_and_close_console(console);return 20;}
    sprintf(stage,"%s/result.%s",stage_dir,kind==ND_ARCHIVE_ZIP ? "zip":"lha");
    result=0;
    for(item=0;item<count;++item){
        if(!nd_archive_compress_command(kind,tool,sources[item].path,stage,
                                        sources[item].include_info,command,sizeof(command))){
            result=20;break;
        }
        strncpy(job_name,FilePart(sources[item].path),sizeof(job_name)-1);job_name[sizeof(job_name)-1]=0;
        write_status("COMPRESSING");
        print_pair(console,"\nAdding: %s\n",(LONG)sources[item].path,0);
        if(sources[item].include_info)
            print_pair(console,"Including Workbench icon: %s.info\n",(LONG)sources[item].path,0);
        FPuts(console,"\n");Flush(console);
        result=run_in_console(command,console);
        if(result!=0)break;
    }
    if(result==0){
        lock=Lock(output,ACCESS_READ);
        if(lock){UnLock(lock);result=20;}
        else if(!Rename(stage,output))result=20;
        if(result!=0){
            write_status("SAVE FAILED");
            print_pair(console,"\nArchive could not be saved to the requested name.\nResult kept at: %s\n",(LONG)stage,0);
            wait_and_close_console(console);return 20;
        }
        DeleteFile(stage_dir);
        if(icon && !ensure_workbench_icon(output,0))
            FPuts(console,"WARNING: The archive was saved but its icon could not be created.\n");
        (void)refresh_workbench_parent(output);
        write_status("DONE");FPuts(console,"\nArchive created successfully.\n");
        completed_at(output,0,console);
    } else {
        /* Only our own new temporary file is eligible for cleanup. */
        (void)DeleteFile(stage);(void)DeleteFile(stage_dir);
        write_status("FAILED");print_pair(console,"\nCompression failed (return code %ld).\n",result,0);
    }
    wait_and_close_console(console);return result==0 ? 0:20;
}
static int compress_object(const char *source,int drawer)
{
    struct NDArchiveSource item;
    strcpy(item.path,source);item.drawer=drawer;item.include_info=source_icon_exists(source);
    return compress_sources(&item,1,0);
}
static int resolve_source(const char *path,struct NDArchiveSource *source)
{
    BPTR lock;struct FileInfoBlock *fib;int valid=0;
    if(!nd_archive_safe_path(path))return 0;
    lock=Lock((STRPTR)path,ACCESS_READ);if(!lock)return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if(fib){
        if(Examine(lock,fib) && NameFromLock(lock,source->path,sizeof(source->path))){
            source->drawer=fib->fib_DirEntryType>0;valid=1;
        }
        FreeDosObject(DOS_FIB,fib);
    }
    UnLock(lock);
    if(valid)source->include_info=source_icon_exists(source->path);
    return valid;
}
static int handle_list(const char *path)
{
    struct NDArchiveSource *sources;char line[262],parent[120];
    FILE *file;unsigned int count=0,planned,prefix;int valid=1,result=20;BPTR console;
    nd_archive_status_name((unsigned long)FindTask(NULL),status_file,sizeof(status_file));
    /* Only the reserved hand-off files are consumed and removed. */
    if(strncmp(path,"T:NovaDockArchiveDrop-",22) || strlen(path)>=sizeof(parent) ||
       !nd_archive_safe_path(path) || !strstr(path,"/items") ||
       strcmp(strstr(path,"/items"),"/items"))return 20;
    for(prefix=22;path[prefix] && path[prefix]!='/';++prefix)
        if(!((path[prefix]>='0' && path[prefix]<='9') ||
             (path[prefix]>='a' && path[prefix]<='f') || path[prefix]=='-'))return 20;
    file=fopen(path,"r");if(!file)return 20;
    sources=(struct NDArchiveSource *)AllocVec(ND_ARCHIVE_MAX_SOURCES*sizeof(*sources),MEMF_PUBLIC|MEMF_CLEAR);
    if(!sources){
        fclose(file);remove(path);strcpy(parent,path);*PathPart(parent)=0;
        if(parent[strlen(parent)-1]=='/')parent[strlen(parent)-1]=0;
        DeleteFile(parent);return 20;
    }
    while(fgets(line,sizeof(line),file)){
        if(!strchr(line,'\n') || count>=ND_ARCHIVE_MAX_SOURCES){valid=0;break;}
        line[strcspn(line,"\r\n")]=0;
        if(!resolve_source(line,&sources[count])){valid=0;break;}
        ++count;
    }
    if(ferror(file))valid=0;fclose(file);
    remove(path);strcpy(parent,path);*PathPart(parent)=0;
    if(parent[strlen(parent)-1]=='/')parent[strlen(parent)-1]=0;
    DeleteFile(parent);
    planned=valid ? nd_archive_plan_sources(sources,count):0;
    if(planned)result=compress_sources(sources,planned,1);
    else{
        write_status("INVALID SELECTION");console=Open(EXTRACT_CONSOLE,MODE_OLDFILE);
        if(console){FPuts(console,"Cannot combine this selection. Check that all items exist and have distinct names.\n");wait_and_close_console(console);}
    }
    FreeVec(sources);return result;
}
static int handle_drop(const char *path,int extract_only)
{
    struct FileInfoBlock *fib;
    BPTR lock;
    FILE *f;
    unsigned char header[512];
    char source[260];
    unsigned int length=0;
    int drawer=0,kind=ND_ARCHIVE_NONE,valid=0;
    nd_archive_status_name((unsigned long)FindTask(NULL),status_file,sizeof(status_file));
    if(!nd_archive_safe_path(path)){write_status("INVALID PATH");return 20;}
    lock=Lock((STRPTR)path,ACCESS_READ);
    if(!lock){write_status("NOT FOUND");return 20;}
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if(fib){
        if(Examine(lock,fib) && NameFromLock(lock,source,sizeof(source))){drawer=fib->fib_DirEntryType>0;valid=1;}
        FreeDosObject(DOS_FIB,fib);
    }
    UnLock(lock);
    if(!valid || !nd_archive_safe_path(source)){write_status("INVALID PATH");return 20;}
    strncpy(job_name,FilePart(source),sizeof(job_name)-1);job_name[sizeof(job_name)-1]=0;
    if(!drawer){
        f=fopen(source,"rb");
        if(f){length=(unsigned int)fread(header,1,sizeof(header),f);fclose(f);}
        kind=nd_archive_detect(source,header,length);
    }
    if(kind!=ND_ARCHIVE_NONE)return extract_archive(source,kind);
    if(extract_only){write_status("UNSUPPORTED");return 20;}
    return compress_object(source,drawer);
}
int main(int argc,char **argv)
{
    struct NDClient client;
    unsigned int event;
    char value[32],caption[ND_TEXT_LEN];
    int available=0,hold=0;
    unsigned int poll_ticks=50,elapsed_ticks=0,delay_ticks;
    char option[32];
    if(argc>=4 && !strcmp(argv[2],"HANDLELIST"))return handle_list(argv[3]);
    if (argc>=4 && (!strcmp(argv[2],"EXTRACT") || !strcmp(argv[2],"HANDLE")))
        return handle_drop(argv[3],!strcmp(argv[2],"EXTRACT"));
    if (argc<2 || !nd_client_init(&client,argv[1])) return 20;
    available=available_tools();
    nd_plugin_option_string("ARCHIVE","POLL_TICKS","50",option,sizeof(option));
    poll_ticks=(unsigned int)atoi(option);
    if (poll_ticks<5 || poll_ticks>250) poll_ticks=50;
    strcpy(value,available ? "DROP HERE" : "NO TOOLS");
    strcpy(caption,"ARCHIVE");
    for (;;) {
        char status[32],badge[4];
        int running;
        int state=collect_status(status,sizeof(status),&running);
        event=0;if(state==1)event|=nd_client_notify(&client,"Archive operation finished",status,(!strcmp(status,"FAILED") || !strcmp(status,"INTERRUPTED")) ? 2:0);
        if (state) {
            strcpy(value,status);
            strcpy(caption,status_subject[0] ? status_subject:"ARCHIVE");
            hold=state==1 ? 8:0;
        } else if (hold>0) --hold;
        else {strcpy(value,available ? "DROP HERE" : "NO TOOLS");strcpy(caption,"ARCHIVE");}
        event|=nd_client_update_art_quiet(&client,caption,value,"archive",!strncmp(value,"COMPRESSING",11) ? 1U:!strncmp(value,"EXTRACTING",10) ? 2U:!strncmp(value,"WORKING",7) ? 3U:0U,
                                     elapsed_ticks,ND_ARCHIVE_HEARTBEAT_TICKS);
        /* Badge: how many operations are running (none: no badge). */
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
