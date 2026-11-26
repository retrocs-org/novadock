/* Permanent titled notes, theme-matched picker and native multiline editor. */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_plugin_versions.h"
#include "nd_client.h"
#include "nd_option_runtime.h"
#include "nd_note_editor.h"
#include "nd_markdown.h"
#include "../clipboard/nd_clipboard_popup.h"
#if defined(ND_PLUGIN_RESOURCES)
#include "nd_plugin_resources.h"
#endif
const char nd_version[]=ND_PLUGIN_VER_STRING(NOTES,"Notes");
long __stack=32768;
struct IntuitionBase *IntuitionBase=NULL;struct GfxBase *GfxBase=NULL;
struct Library *GadToolsBase=NULL,*KeymapBase=NULL,*DiskfontBase=NULL;
struct NoteEntry {unsigned int id;char title[80],preview[64],filename[40];};
static struct NoteEntry entries[ND_NOTE_MAX];static struct NDNote note;
static char root[128],path[220];static unsigned int count;
static int drawer_exists(const char *name)
{
    BPTR lock;struct FileInfoBlock *fib;int ok=0;
    struct Process *self=(struct Process *)FindTask(NULL);APTR old=self->pr_WindowPtr;
    self->pr_WindowPtr=(APTR)-1;lock=Lock((STRPTR)name,ACCESS_READ);self->pr_WindowPtr=old;
    if(lock){fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);
        if(fib){ok=Examine(lock,fib) && fib->fib_DirEntryType>0;FreeDosObject(DOS_FIB,fib);}UnLock(lock);}
    return ok;
}
static int ensure_root(void)
{
    BPTR lock;
    if(drawer_exists(root))return 1;
    if(!strcmp(root,"ENVARC:NovaDock/Notes") && !drawer_exists("ENVARC:NovaDock")){
        lock=CreateDir("ENVARC:NovaDock");if(!lock)return 0;UnLock(lock);
    }
    lock=CreateDir(root);if(!lock)return 0;UnLock(lock);return 1;
}
static void note_path(unsigned int id,char *out)
{sprintf(out,"%s%snote-%04u.md",root,root[strlen(root)-1]==':' ? "":"/",id);}
static int entry_compare(const void *left,const void *right)
{const struct NoteEntry *a=left,*b=right;return strcmp(a->title,b->title);}
static int scan_notes(void)
{
    BPTR lock;struct FileInfoBlock *fib;struct NDNote *temp;struct NDMarkdown *parsed;
    unsigned int id;char candidate[220],expected[40];int ok=1;
    count=0;if(!ensure_root())return 0;lock=Lock(root,ACCESS_READ);if(!lock)return 0;
    fib=(struct FileInfoBlock *)AllocDosObject(DOS_FIB,NULL);temp=(struct NDNote *)AllocVec(sizeof(*temp),MEMF_PUBLIC);parsed=AllocVec(sizeof(*parsed),MEMF_PUBLIC);
    if(!fib || !temp || !parsed || !Examine(lock,fib)){ok=0;goto done;}
    while(ExNext(lock,fib)){
        if(fib->fib_DirEntryType>=0 || sscanf(fib->fib_FileName,"note-%u",&id)!=1 || !id || id>9999)continue;
        sprintf(expected,"note-%04u.md",id);
        if(strcmp(expected,fib->fib_FileName)){sprintf(expected,"note-%04u.ndnote",id);if(strcmp(expected,fib->fib_FileName))continue;}
        strcpy(candidate,root);if(!AddPart(candidate,fib->fib_FileName,sizeof(candidate))){ok=0;continue;}if(nd_note_load(candidate,temp)!=1){ok=0;continue;}
        if(count>=ND_NOTE_MAX){ok=0;continue;}
        entries[count].id=id;strcpy(entries[count].filename,fib->fib_FileName);strcpy(entries[count].title,temp->title[0] ? temp->title:"Untitled note");
        nd_md_parse(temp->text,parsed,0,0);
        strncpy(entries[count].preview,parsed->text,sizeof(entries[count].preview)-1);entries[count].preview[sizeof(entries[count].preview)-1]=0;++count;
    }
    if(IoErr()!=ERROR_NO_MORE_ENTRIES)ok=0;
    qsort(entries,count,sizeof(entries[0]),entry_compare);
done:if(fib)FreeDosObject(DOS_FIB,fib);if(temp)FreeVec(temp);if(parsed)FreeVec(parsed);UnLock(lock);return ok;
}
static int save_note(void)
{
    char stage[260];BPTR lock;unsigned int i;int rc;
    if(!ensure_root())return 0;
    if(!note.title[0]){
        struct NDMarkdown *plain=AllocVec(sizeof(*plain),MEMF_PUBLIC);
        if(plain){nd_md_parse(note.text,plain,0,0);
            for(i=0;plain->text[i] && plain->text[i]!='\n' && i<sizeof(note.title)-1;++i)note.title[i]=plain->text[i];
            note.title[i]=0;FreeVec(plain);}
        if(!note.title[0])strcpy(note.title,"Untitled note");
    }
    for(i=0;i<20;++i){sprintf(stage,"%s%s.save-%08lx-%u",root,root[strlen(root)-1]==':' ? "":"/",(unsigned long)FindTask(NULL),i);lock=CreateDir(stage);if(lock)break;}
    if(!lock)return 0;UnLock(lock);rc=nd_note_save(path,stage,&note);(void)DeleteFile(stage);return rc;
}
static int finish_editor(struct NDNoteEditor **editor)
{
    int rc;if(!*editor)return 1;
    if(!nd_note_editor_prepare_save(*editor))return 0;
    if((note.original[0] || note.text[0] || note.title[0]) && nd_note_dirty(&note)){
        rc=save_note();if(rc!=1){nd_note_editor_notice(*editor,rc==-2 ? "Note changed elsewhere; resolve it before switching":"Save failed; your current note remains open");return 0;}
    }
    nd_note_editor_close(*editor);*editor=NULL;return 1;
}
static int new_note(void)
{
    unsigned int id;BPTR lock;
    if(count>=ND_NOTE_MAX || !ensure_root())return 0;
    for(id=1;id<=9999;++id){char legacy[220];
        note_path(id,path);lock=Lock(path,ACCESS_READ);if(lock){UnLock(lock);continue;}
        if(IoErr()!=ERROR_OBJECT_NOT_FOUND)continue;
        sprintf(legacy,"%s%snote-%04u.ndnote",root,root[strlen(root)-1]==':' ? "":"/",id);
        lock=Lock(legacy,ACCESS_READ);if(lock){UnLock(lock);continue;}
        if(IoErr()==ERROR_OBJECT_NOT_FOUND){nd_note_init(&note);return 1;}
    }
    return 0;
}
static int delete_note(void)
{
    char deleted[220],destination[280];BPTR lock;unsigned int i;int rc;
    if(!note.original[0])return 1;
    strcpy(deleted,root);if(!AddPart(deleted,"Deleted",sizeof(deleted)))return 0;
    if(!drawer_exists(deleted)){lock=CreateDir(deleted);if(!lock)return 0;UnLock(lock);}
    for(i=0;i<20;++i){
        sprintf(destination,"%s/%s-%08lx-%u",deleted,FilePart(path),(unsigned long)FindTask(NULL),i);
        lock=Lock(destination,ACCESS_READ);if(lock){UnLock(lock);continue;}
        rc=nd_note_archive(path,destination,note.original);return rc;
    }
    return 0;
}
static int show_picker(struct NDClipQuick *picker)
{
    struct NDClipHistory h;char labels[ND_NOTE_MAX][160];unsigned int i;
    memset(&h,0,sizeof(h));h.count=count;
    for(i=0;i<count;++i){sprintf(labels[i],"%.79s\n%.63s",entries[i].title,entries[i].preview);h.text[i]=labels[i];}
    return nd_clip_quick_open(picker,&h);
}
static unsigned int note_loop_elapsed(const struct DateStamp *before,const struct DateStamp *after)
{
    LONG days=after->ds_Days-before->ds_Days,ticks;
    if(days < -1 || days>1)return 500U;
    ticks=days*4320000L+(after->ds_Minute-before->ds_Minute)*3000L+after->ds_Tick-before->ds_Tick;
    if(ticks<0 || ticks>500L)return 500U;return (unsigned int)ticks;
}
int main(int argc,char **argv)
{
    struct DateStamp before,now;
    struct NDClient client;struct NDClipQuick *picker=NULL;struct NDNoteEditor *editor=NULL;
    char configured[128],value[32],badge[8],notice[96],recovery[260];
    unsigned int event,elapsed=0;unsigned long extra;
    int reread=1,rescan=1,action,selected,rc,result=20,scan_ok=1,prepared;
    if(argc<2)return 20;
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",39);
    GadToolsBase=OpenLibrary("gadtools.library",39);KeymapBase=OpenLibrary("keymap.library",36);DiskfontBase=OpenLibrary("diskfont.library",0);
    if(!IntuitionBase || !GfxBase || !GadToolsBase || !KeymapBase || !nd_client_init(&client,argv[1]))goto cleanup;
    picker=nd_clip_quick_create();if(!picker){nd_client_close(&client);goto cleanup;}
    nd_clip_quick_labels(picker,"NOTES","No saved notes - right-click Notes to add one","NovaDock Notes","Click to open - Esc to close","Note");
    root[0]=0;strcpy(value,"0 notes saved");
    DateStamp(&before);
    for(;;){
        DateStamp(&now);elapsed=note_loop_elapsed(&before,&now);before=now;
        if(reread){
            nd_plugin_option_string(argv[1],"DRAWER","ENVARC:NovaDock/Notes",configured,sizeof(configured));
            if(!configured[0] || !strchr(configured,':'))strcpy(configured,"ENVARC:NovaDock/Notes");
            while(strlen(configured)>1 && configured[strlen(configured)-1]=='/')configured[strlen(configured)-1]=0;
            if(!editor){strcpy(root,configured);rescan=1;nd_clip_quick_close(picker);}
            reread=0;
        }
        if(rescan){scan_ok=scan_notes();rescan=0;sprintf(value,scan_ok ? "%u note%s saved":"%u notes; read error",count,count==1 ? "":"s");}
        event=nd_client_update_art_quiet(&client,"NOTES",value,"notes",0,elapsed,500U);
        sprintf(badge,"%u",count);if(!(event&ND_EVENT_QUIT))event|=nd_client_set_badge(&client,count ? badge:"");
        extra=nd_client_events2(&client);
        if(event&ND_EVENT_QUIT){
            prepared=nd_note_editor_prepare_save(editor);
            if(editor && note.text[0] && nd_note_dirty(&note) && (!prepared || save_note()!=1)){
                /* A conflict never overwrites the other editor's work. */
                sprintf(recovery,"%s%srecovered-%08lx.ndnote",root,root[strlen(root)-1]==':' ? "":"/",(unsigned long)FindTask(NULL));
                strcpy(path,recovery);note.original[0]=0;if(!prepared)note.format[0]=0;if(save_note()!=1)result=20;else result=5;
            }else result=0;
            break;
        }
        if(event&ND_EVENT_SETTINGS)reread=1;
        if(extra&ND_EVENT2_NOTE_NEW){
            nd_clip_quick_close(picker);
            if(finish_editor(&editor)){
                scan_ok=scan_notes();if(new_note())editor=nd_note_editor_open(&note);
                if(!editor)strcpy(value,count>=ND_NOTE_MAX ? "32-note limit":"Cannot create note");
                rescan=1;
            }
        }else if(event&ND_EVENT_CLICK){
            scan_ok=scan_notes();
            if(!count && scan_ok && !editor){if(new_note())editor=nd_note_editor_open(&note);}
            else if(!show_picker(picker))strcpy(value,"Picker unavailable");
        }
        if(nd_clip_quick_events(picker)){
            char chosen[220];selected=nd_clip_quick_selected(picker);
            if(selected>=0 && (unsigned int)selected<count){
                strcpy(chosen,root);AddPart(chosen,entries[selected].filename,sizeof(chosen));
                nd_clip_quick_close(picker);
                if(editor && !strcmp(chosen,path))nd_note_editor_raise(editor);
                else if(finish_editor(&editor)){
                    strcpy(path,chosen);rc=nd_note_load(path,&note);
                    if(rc==1)editor=nd_note_editor_open(&note);else strcpy(value,"Note read failed");rescan=1;
                }
            }
        }
        action=nd_note_editor_events(editor);
        if(action==2 && !note.original[0] && !note.text[0] && !note.title[0]){
            nd_note_editor_close(editor);editor=NULL;reread=1;action=0;
        }
        if(action==1 || action==2){
            rc=save_note();
            if(rc==1){scan_ok=scan_notes();sprintf(value,"%u note%s saved",count,count==1 ? "":"s");
                sprintf(notice,"Saved - %u note%s stored permanently",count,count==1 ? "":"s");nd_note_editor_notice(editor,notice);
                if(action==2){nd_note_editor_close(editor);editor=NULL;reread=1;}}
            else nd_note_editor_notice(editor,rc==-2 ? "Note changed elsewhere; your edits remain open":"Save failed; check the notes drawer. Your edits remain open.");
        }else if(action==4){
            if(!nd_note_editor_prepare_save(editor))rc=0;
            else if(note.original[0] && nd_note_dirty(&note) && save_note()!=1)rc=0;
            else rc=delete_note();
            if(rc==1){nd_note_editor_close(editor);editor=NULL;reread=1;rescan=1;}
            else nd_note_editor_notice(editor,rc==-2 ? "Note changed elsewhere; it was not deleted":"Could not move note to Deleted; note preserved");
        }else if(action==3){nd_note_editor_close(editor);editor=NULL;reread=1;}
        nd_client_wait_signals(&client,500U,nd_note_editor_signals(editor)|nd_clip_quick_signals(picker));
    }
    nd_client_close(&client);nd_note_editor_close(editor);nd_clip_quick_free(picker);
cleanup:
    if(DiskfontBase)CloseLibrary(DiskfontBase);if(KeymapBase)CloseLibrary(KeymapBase);if(GadToolsBase)CloseLibrary(GadToolsBase);
    if(GfxBase)CloseLibrary((struct Library *)GfxBase);if(IntuitionBase)CloseLibrary((struct Library *)IntuitionBase);return result;
}
