/* NovaDock Trash: a dock tile for the Workbench Trashcan.
 *
 *   Trash ID              resident tile: item count and a badge
 *   Trash ID PUT path     move one object (and its icon) into the Trashcan
 *                         drawer of its own volume, creating it if needed
 *   Trash ID EMPTY        ask, then delete everything in the known Trashcans
 *
 * Moving is a Rename() within the object's own volume, never a copy, so it
 * is instant and cannot leave half-moved data.  The resident recounts only
 * after a helper changed something (a marker in T:), so it never spins a
 * floppy motor or raises an insert-volume requester on its own.
 * AmigaOS 3.0 or later, any 68000; C89 for vbcc. */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <stdio.h>
#include <string.h>
#include "nd_version.h"
#include "nd_client.h"

const char nd_version[]=ND_VER_STRING("Trash");
struct IntuitionBase *IntuitionBase=NULL;
struct Library *IconBase=NULL;
struct Library *WorkbenchBase=NULL;

#define TRASH_PATH 300
#define TRASH_MAX_DIRS 16
#define TRASH_LIST "T:NovaDock-trash.list"      /* Trashcans used */
#define TRASH_CHANGED "T:NovaDock-trash.changed" /* recount request */
#define TRASH_POLL_TICKS 50U
#define TRASH_HEARTBEAT_TICKS 500U

/* ---- quiet DOS helpers ------------------------------------------------ */

static APTR quiet(void)
{
    struct Process *self=(struct Process *)FindTask(NULL);
    APTR old=self->pr_WindowPtr;
    self->pr_WindowPtr=(APTR)-1L;   /* missing volumes fail silently */
    return old;
}

static void unquiet(APTR old)
{
    ((struct Process *)FindTask(NULL))->pr_WindowPtr=old;
}

/* Full path of an existing object, without requesters; 0 if missing. */
static int resolve(const char *path,char *full,int size)
{
    APTR old=quiet();
    BPTR lock=Lock((STRPTR)path,ACCESS_READ);
    int ok=0;
    if (lock) {
        ok=NameFromLock(lock,full,size)!=0;
        UnLock(lock);
    }
    unquiet(old);
    return ok;
}

static int exists(const char *path)
{
    char full[TRASH_PATH];
    return resolve(path,full,sizeof(full));
}

static void touch(const char *path)
{
    BPTR f=Open((STRPTR)path,MODE_NEWFILE);
    if (f) Close(f);
}

/* ---- Workbench notification (workbench.library V44, OS 3.5+) ---------- */

static void tell_workbench(const char *path,LONG action)
{
    char parent[TRASH_PATH];
    STRPTR part;
    BPTR lock;
    if (strlen(path)>=sizeof(parent)) return;
    strcpy(parent,path);
    part=PathPart((STRPTR)parent);
    *part='\0';
    WorkbenchBase=OpenLibrary((STRPTR)"workbench.library",44);
    if (!WorkbenchBase) return;
    lock=Lock((STRPTR)parent,ACCESS_READ);
    if (lock) {
        UpdateWorkbench(FilePart((STRPTR)path),lock,action);
        UnLock(lock);
    }
    CloseLibrary(WorkbenchBase);
    WorkbenchBase=NULL;
}

/* ---- the Trashcans this plugin knows about ---------------------------- */

/* SYS:Trashcan plus every Trashcan a PUT used (since the last reboot). */
static int known_trashcans(char dirs[][TRASH_PATH])
{
    static const char *const fixed="SYS:Trashcan";
    char line[TRASH_PATH],full[TRASH_PATH];
    BPTR f;
    int count=0,i,len;
    const char *candidate;
    int from_list=0;
    f=Open((STRPTR)TRASH_LIST,MODE_OLDFILE);
    for (;;) {
        if (!from_list) {candidate=fixed;from_list=1;}
        else {
            if (!f || !FGets(f,(STRPTR)line,sizeof(line))) break;
            len=(int)strlen(line);
            while (len>0 && (line[len-1]=='\n' || line[len-1]=='\r')) line[--len]='\0';
            if (!len) continue;
            candidate=line;
        }
        if (count>=TRASH_MAX_DIRS || !resolve(candidate,full,sizeof(full))) continue;
        for (i=0;i<count;++i) if (!strcmp(dirs[i],full)) break;
        if (i==count) strcpy(dirs[count++],full);
    }
    if (f) Close(f);
    return count;
}

static void remember_trashcan(const char *dir)
{
    char dirs[TRASH_MAX_DIRS][TRASH_PATH];
    int count=known_trashcans(dirs),i;
    BPTR f;
    for (i=0;i<count;++i) if (!strcmp(dirs[i],dir)) return;
    f=Open((STRPTR)TRASH_LIST,MODE_READWRITE);
    if (!f) return;
    Seek(f,0,OFFSET_END);
    FPuts(f,(STRPTR)dir);
    FPutC(f,'\n');
    Close(f);
}

/* Objects in the Trashcans; icons (.info) are not counted separately. */
static long count_items(void)
{
    char dirs[TRASH_MAX_DIRS][TRASH_PATH];
    struct FileInfoBlock *fib;
    APTR old;
    BPTR lock;
    long total=0;
    int count=known_trashcans(dirs),i,len;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
    if (!fib) return 0;
    old=quiet();
    for (i=0;i<count;++i) {
        lock=Lock((STRPTR)dirs[i],ACCESS_READ);
        if (!lock) continue;
        if (Examine(lock,fib) && fib->fib_DirEntryType>0)
            while (ExNext(lock,fib)) {
                len=(int)strlen(fib->fib_FileName);
                if (len>5 && !strcmp(fib->fib_FileName+len-5,".info")) continue;
                ++total;
            }
        UnLock(lock);
    }
    unquiet(old);
    FreeDosObject(DOS_FIB,fib);
    return total;
}

/* ---- PUT ---------------------------------------------------------------- */

/* TEXT is passed as an argument, never as the format: file names may
 * contain '%'.  Returns the chosen gadget (1 = first, 0 = last). */
static LONG ask_user(const char *text,const char *gadgets)
{
    struct EasyStruct es;
    APTR args[1];
    if (!IntuitionBase) return 0;
    es.es_StructSize=sizeof(es);
    es.es_Flags=0;
    es.es_Title=(UBYTE *)"NovaDock Trash";
    es.es_TextFormat=(UBYTE *)"%s";
    es.es_GadgetFormat=(UBYTE *)gadgets;
    args[0]=(APTR)text;
    return EasyRequestArgs(NULL,&es,NULL,args);
}

static void tell_user(const char *text,const char *gadgets)
{
    (void)ask_user(text,gadgets);
}

/* The volume's Trashcan drawer, created with a trashcan icon if missing. */
static int ensure_trashcan(const char *trash)
{
    struct DiskObject *icon;
    char info[TRASH_PATH+8];
    BPTR lock;
    if (exists(trash)) return 1;
    lock=CreateDir((STRPTR)trash);
    if (!lock) return 0;
    UnLock(lock);
    sprintf(info,"%s.info",trash);
    if (!exists(info) && (IconBase=OpenLibrary((STRPTR)"icon.library",37))!=NULL) {
        icon=GetDefDiskObject(WBGARBAGE);
        if (icon) {
            (void)PutDiskObject((STRPTR)trash,icon);
            FreeDiskObject(icon);
        }
        CloseLibrary(IconBase);
        IconBase=NULL;
    }
    tell_workbench(trash,UPDATEWB_ObjectAdded);
    return 1;
}

static int trash_put(const char *path)
{
    char full[TRASH_PATH],trash[TRASH_PATH],target[TRASH_PATH+8];
    char source_info[TRASH_PATH+8],target_info[TRASH_PATH+16],text[TRASH_PATH+64];
    char *colon;
    int n;
    if (!resolve(path,full,sizeof(full))) return 10;
    colon=strchr(full,':');
    /* A whole volume is not trash, and the Trashcan does not trash itself. */
    if (!colon || !colon[1]) return 10;
    if (!strncmp(colon+1,"Trashcan",8) && (!colon[9] || colon[9]=='/')) {
        tell_user("That is already in the trash.","OK");
        return 5;
    }
    n=(int)(colon-full)+1;
    memcpy(trash,full,n);
    strcpy(trash+n,"Trashcan");
    if (!ensure_trashcan(trash)) {
        sprintf(text,"Could not create a Trashcan on %.*s",n,full);
        tell_user(text,"OK");
        return 10;
    }
    sprintf(target,"%s/%s",trash,FilePart((STRPTR)full));
    for (n=1;exists(target) && n<100;++n)
        sprintf(target,"%s/%s.%d",trash,FilePart((STRPTR)full),n);
    if (exists(target) || !Rename((STRPTR)full,(STRPTR)target)) {
        sprintf(text,"Could not move %.200s to the trash.",FilePart((STRPTR)full));
        tell_user(text,"OK");
        return 10;
    }
    /* Its icon goes with it, under the same new name. */
    sprintf(source_info,"%s.info",full);
    sprintf(target_info,"%s.info",target);
    if (exists(source_info)) (void)Rename((STRPTR)source_info,(STRPTR)target_info);
    tell_workbench(full,UPDATEWB_ObjectRemoved);
    tell_workbench(target,UPDATEWB_ObjectAdded);
    remember_trashcan(trash);
    touch(TRASH_CHANGED);
    return 0;
}

/* ---- EMPTY ---------------------------------------------------------------- */

static int trash_empty(void)
{
    char dirs[TRASH_MAX_DIRS][TRASH_PATH],command[TRASH_PATH+48],text[160];
    BPTR nil_in,nil_out;
    long items=count_items();
    int count,i;
    if (!items) {
        tell_user("The trash is empty.","OK");
        return 0;
    }
    if (!IntuitionBase) return 10;
    sprintf(text,"Permanently delete %ld item%s in the trash?\n"
                 "This cannot be undone.",items,items==1 ? "" : "s");
    if (ask_user(text,"Delete|Cancel")!=1) return 0;
    count=known_trashcans(dirs);
    for (i=0;i<count;++i) {
        sprintf(command,"C:Delete \"%s/#?\" ALL QUIET FORCE",dirs[i]);
        nil_in=Open((STRPTR)"NIL:",MODE_OLDFILE);
        nil_out=Open((STRPTR)"NIL:",MODE_NEWFILE);
        if (nil_in && nil_out)
            (void)SystemTags((STRPTR)command,SYS_Input,nil_in,
                             SYS_Output,nil_out,TAG_DONE);
        if (nil_in) Close(nil_in);
        if (nil_out) Close(nil_out);
        tell_workbench(dirs[i],UPDATEWB_ObjectAdded);
    }
    touch(TRASH_CHANGED);
    return 0;
}

/* ---- resident tile -------------------------------------------------------- */

static void describe(long items,char *value,char *badge)
{
    if (items<=0) {strcpy(value,"EMPTY");badge[0]='\0';return;}
    sprintf(value,"%ld ITEM%s",items,items==1 ? "" : "S");
    sprintf(badge,"%ld",items>99 ? 99L : items);
}

int main(int argc,char **argv)
{
    struct NDClient client;
    unsigned int event,elapsed=0;
    char value[32],badge[ND_BADGE_LEN];
    long items;
    int rc;
    if (argc>=3 && (!strcmp(argv[2],"PUT") || !strcmp(argv[2],"EMPTY"))) {
        IntuitionBase=(struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library",37);
        if (!strcmp(argv[2],"PUT")) rc=argc>=4 ? trash_put(argv[3]) : 10;
        else rc=trash_empty();
        if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
        return rc;
    }
    if (argc<2 || !nd_client_init(&client,argv[1])) return 20;
    items=count_items();
    describe(items,value,badge);
    for (;;) {
        event=nd_client_update_quiet(&client,"TRASH",value,ND_NO_METER,
                                     elapsed,TRASH_HEARTBEAT_TICKS);
        if (!(event&ND_EVENT_QUIT)) event|=nd_client_set_badge(&client,badge);
        if (event&ND_EVENT_QUIT) break;
        /* A helper changed a Trashcan: recount once (T: is normally RAM). */
        if (exists(TRASH_CHANGED)) {
            DeleteFile((STRPTR)TRASH_CHANGED);
            items=count_items();
            describe(items,value,badge);
            elapsed=0;
            continue;
        }
        elapsed=nd_client_wait(&client,
                               nd_client_delay_ticks(&client,TRASH_POLL_TICKS));
    }
    nd_client_close(&client);
    return 0;
}
