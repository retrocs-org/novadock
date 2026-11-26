#!/usr/bin/env python3
"""Exercise the real plugin menu labels and dispatch with host-side OS stubs."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def function(source, signature):
    start = source.index(signature + '\n{')
    end = source.index('\n}', start) + 2
    return source[start:end]


class PluginMenuTests(unittest.TestCase):
    def test_inactive_dock_right_button_filter(self):
        source = (ROOT/'src/dock_input.c').read_text()
        start = source.index('    memset(&right_button,0,sizeof(right_button));')
        end = source.index('    filter=CreateCxObj', start)
        setup = source[start:end]
        harness = r"""
#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) 0
#define dock_view_y(d) 0

#include <string.h>
#define IX_VERSION 2
#define IECLASS_RAWMOUSE 2
#define IECODE_RBUTTON 0x69
#define IX_NORMALQUALS 0x7fff
struct IX {unsigned int ix_Version,ix_Class,ix_Code,ix_CodeMask,ix_Qualifier,ix_QualMask;};
/* MatchIX mask semantics verified against commodities.library on Workbench. */
static int matches(struct IX *ix,unsigned int cls,unsigned int code,unsigned int qualifiers)
{
    return cls==ix->ix_Class && !((code^ix->ix_Code)&ix->ix_CodeMask) &&
        !((qualifiers^ix->ix_Qualifier)&ix->ix_QualMask);
}
int main(void)
{
    struct IX right_button;
    unsigned int qualifiers,code;
""" + setup + r"""
    for(qualifiers=0;qualifiers<=0xffff;++qualifiers)
        assert(matches(&right_button,2,0x69,qualifiers));
    for(code=0;code<=0xff;++code)
        assert(matches(&right_button,2,code,0x2000)==(code==0x69));
    assert(!matches(&right_button,4,0x69,0x2000));
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            (path/'filter.c').write_text(harness)
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c89', '-Wall', '-Wextra',
                            str(path/'filter.c'), '-o', str(path/'filter')], check=True)
            subprocess.run([str(path/'filter')], check=True)

    def test_all_plugins_share_settings_and_remove_dispatch(self):
        source = (ROOT / 'src/dock_menu.c').read_text()
        harness = r"""
#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) 0
#define dock_view_y(d) 0
#include <stdio.h>
#include <string.h>
#include "nd_config.h"
#define ITEM_TEXT(item,which) ""
#define ND_EVENT_NETWORK_ONLINE 1
#define ND_EVENT_NETWORK_OFFLINE 2
#define ND_EVENT_BITSKIM_RECEIVING 4
#define ND_EVENT_BITSKIM_CANCEL 8
#define ND_EVENT_MANAGE_HISTORY 16
#define ND_EVENT2_NOTE_NEW 2UL
#define ND_EVENT2_SENSOR_PREVIOUS 2048UL
#define ND_EVENT2_SENSOR_NEXT 4096UL
#define ND_EVENT2_SENSOR_DETAILS 8192UL
#define ND_MENU_PROFILES 4
struct NDDock {struct NDConfig conf; unsigned long plugin_event2[48]; int popup_item,edit_mode,popup_network_online,popup_warp_present,popup_prism_present; void *screen; int undo_kind; void *undo_config; char menu_profiles[ND_MENU_PROFILES][ND_PROFILE_LEN]; int menu_profile_count;};
#define ND_UNDO_NONE 0
#define ND_UNDO_MOVE 1
#define ND_UNDO_REMOVE 2
static int launches,removed,beeps,allow_launch=1;
static const char *nd_option_schema_id(const struct NDConfig *c,const char *id){return strstr(c->items[0].command,"Plugins/Sensors") ? "SENSORS":id;}
static const char *music="";
static char arguments[80];
const char *nd_option_get(const struct NDConfig *c,const char *id,const char *key)
{(void)c;(void)id;(void)key;return music;}
static int launch_companion(struct NDDock *d,const char *name,const char *args)
{(void)d;assert(!strcmp(name,"NovaDockPrefs"));++launches;strcpy(arguments,args);return allow_launch;}
static void DisplayBeep(void *screen) {(void)screen;++beeps;}
static int remove_dragged_item(struct NDDock *d,int index) {(void)d;assert(index==0);++removed;return 1;}
static void open_trash(struct NDDock *d) {(void)d;}
static int launch_plugin_helper(struct NDDock *d,int i,const char *a,const char *b)
{(void)d;(void)i;(void)b;strcpy(arguments,a);return 1;}
static void show_feedback(struct NDDock *d,const char *s,int error) {(void)d;(void)s;(void)error;}
static void launch_item(struct NDDock *d,int i) {(void)d;(void)i;}
static void request_dopus_action(struct NDDock *d,int i) {(void)d;(void)i;}
static int launch_default_music_application(struct NDDock *d) {(void)d;return 1;}
"""
        harness += '\n' + function((ROOT/'src/config.c').read_text(), 'int nd_hass_plugin_path(const char *command)')
        for signature in [
            'static int plugin_can_launch(const struct NDDock *dock,\n                             const struct NDItem *item)',
            'int is_trash_tile(const struct NDDock *dock,int index)',
            'static int plugin_settings_action(const struct NDDock *dock,const struct NDItem *item)',
            'static int dock_has_undo(const struct NDDock *dock)',
        ]:
            harness += '\n' + function(source, signature)
        start = source.index('/* The background (dock) menu, top to bottom.')
        harness += '\n' + source[start:source.index('static int popup_count_for(', start)]
        harness += '\n' + function(source, 'static int popup_count_for(const struct NDDock *dock)')
        harness += '\nconst char *nd_trf(const char *english_format) {return english_format;}\n'
        start = source.index('static const char *launcher_actions[]')
        end = source.index('static const char *popup_label(', start)
        harness += '\n' + source[start:end]
        harness += '\n' + function(source, 'static int plugin_popup_action(struct NDDock *dock, int index, int action)')
        harness += r"""
int main(void)
{
    static struct NDDock dock;
    const char *ids[]={"TRASH","NET","ROADSHOW","BITSKIM","CLIPBOARD","DOPUS4","MUSIC","CPU","RAM","CLOCK","EYES","ARCHIVE","RUNNINGAPPS","TIMER","NOTES","THIRD_PARTY"};
    int i,count,before;
    char expected[80];
    dock.conf.count=1;dock.conf.items[0].kind=ND_ITEM_PLUGIN;
    dock.screen=&dock;
    for(i=0;i<(int)(sizeof(ids)/sizeof(ids[0]));++i) {
        strcpy(dock.conf.items[0].id,ids[i]);
        count=popup_count_for(&dock);
        assert(count>=2);
        assert(!strcmp(popup_label_en(&dock,count-2),"Edit settings..."));
        assert(!strcmp(popup_label_en(&dock,count-1),"Remove"));
        before=launches;
        assert(!plugin_popup_action(&dock,0,count-2));
        assert(launches==before+1);
        sprintf(expected,"PAGE=PLUGIN PLUGIN=%s FOCUSED=1",ids[i]);
        assert(!strcmp(arguments,expected));
        allow_launch=0;before=beeps;
        plugin_popup_action(&dock,0,count-2);
        assert(beeps==before+1);allow_launch=1;
        before=removed;
        assert(plugin_popup_action(&dock,0,count-1)==1);
        assert(removed==before+1);
    }
    strcpy(dock.conf.items[0].command,"Plugins/Sensors");strcpy(dock.conf.items[0].id,"SENSORS_2");
    assert(popup_count_for(&dock)==5 && !strcmp(popup_label_en(&dock,0),"Sensor readings..."));
    plugin_popup_action(&dock,0,1);assert(dock.plugin_event2[0]&ND_EVENT2_SENSOR_NEXT);
    plugin_popup_action(&dock,0,2);assert(dock.plugin_event2[0]&ND_EVENT2_SENSOR_PREVIOUS);
    plugin_popup_action(&dock,0,0);assert(dock.plugin_event2[0]&ND_EVENT2_SENSOR_DETAILS);
    strcpy(dock.conf.items[0].command,"Plugins/HASS");strcpy(dock.conf.items[0].id,"HASS_2");
    assert(popup_count_for(&dock)==2);assert(!strcmp(popup_label_en(&dock,0),"Edit settings..."));
    assert(!strcmp(popup_label_en(&dock,1),"Remove"));
    plugin_popup_action(&dock,0,0);assert(!strcmp(arguments,"PAGE=PLUGIN PLUGIN=HASS_2 FOCUSED=1"));
    before=removed;assert(plugin_popup_action(&dock,0,1)==1);assert(removed==before+1);
    dock.conf.items[0].command[0]=0;
    dock.popup_warp_present=1;
    strcpy(dock.conf.items[0].id,"CPU");assert(popup_count_for(&dock)==3);
    assert(!strcmp(popup_label_en(&dock,0),"Set Turbo Level..."));
    plugin_popup_action(&dock,0,0);assert(!strcmp(arguments,"WARP_TURBO"));
    plugin_popup_action(&dock,0,1);assert(!strcmp(arguments,"PAGE=PLUGIN PLUGIN=CPU FOCUSED=1"));
    strcpy(dock.conf.items[0].id,"NET");assert(popup_count_for(&dock)==5);
    assert(!strcmp(popup_label_en(&dock,2),"Configure Wi-Fi Network..."));
    plugin_popup_action(&dock,0,2);assert(!strcmp(arguments,"WARP_WIFI"));
    plugin_popup_action(&dock,0,3);assert(!strcmp(arguments,"PAGE=PLUGIN PLUGIN=NET FOCUSED=1"));
    dock.popup_prism_present=1;assert(popup_count_for(&dock)==6);
    assert(!strcmp(popup_label_en(&dock,3),"Configure Prism Wi-Fi..."));
    plugin_popup_action(&dock,0,3);assert(!strcmp(arguments,"PRISM_WIFI"));
    plugin_popup_action(&dock,0,4);assert(!strcmp(arguments,"PAGE=PLUGIN PLUGIN=NET FOCUSED=1"));
    dock.popup_warp_present=0;assert(popup_count_for(&dock)==5);
    plugin_popup_action(&dock,0,2);assert(!strcmp(arguments,"PRISM_WIFI"));
    dock.popup_prism_present=0;
    strcpy(dock.conf.items[0].id,"NOTES");
    assert(popup_count_for(&dock)==4);
    assert(!strcmp(popup_label_en(&dock,1),"Add new note..."));
    plugin_popup_action(&dock,0,0);assert(dock.conf.items[0].clicks==1);
    plugin_popup_action(&dock,0,1);assert(dock.plugin_event2[0]&ND_EVENT2_NOTE_NEW);
    strcpy(dock.conf.items[0].id,"TRASH");
    assert(popup_count_for(&dock)==5);
    assert(!strcmp(popup_label_en(&dock,2),"Refresh"));
    plugin_popup_action(&dock,0,2);assert(!strcmp(arguments,"REFRESH"));
    strcpy(dock.conf.items[0].id,"MUSIC");music="SYS:Tools/Player";
    count=popup_count_for(&dock);assert(count==3);
    assert(!strcmp(popup_label_en(&dock,0),"Launch"));
    plugin_popup_action(&dock,0,1);
    assert(!strcmp(arguments,"PAGE=PLUGIN PLUGIN=MUSIC FOCUSED=1"));
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            (path/'menu.c').write_text(harness)
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c89', '-Wall', '-Wextra',
                            '-I'+str(ROOT/'include'), str(path/'menu.c'), '-o', str(path/'menu')], check=True)
            subprocess.run([str(path/'menu')], check=True)

    def test_edit_drops_add_instead_of_running_tile_actions(self):
        source = (ROOT/'src/dock_events.c').read_text()
        harness = r"""
#include <assert.h>
#define dock_left(d) ((d)->window->LeftEdge)
#define dock_top(d) ((d)->window->TopEdge)
#define dock_width(d) ((d)->window->Width)
#define dock_height(d) ((d)->window->Height)
#define dock_view_x(d) 0
#define dock_view_y(d) 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nd_config.h"
#include "nd_interaction.h"
#define AMTYPE_APPMENUITEM 2
#define MEMF_PUBLIC 0
#define MEMF_CLEAR 0
#define MEMF_REVERSE 0
#define nd_tr(text) (text)
#define nd_trf(text) (text)
struct Message {int dummy;};
struct WBArg {int number;};
struct AppMessage {int am_Type,am_UserData,am_MouseX,am_MouseY,am_NumArgs;struct WBArg *am_ArgList;};
struct NDDock {struct NDConfig conf;int edit_mode,action_count,action_items[1],overflow,view_start,visible_count,drop_preview;void *drop_port,*screen,*write_owner;};
static struct AppMessage message;
static int pending,replies,actions,added,position[4],hover=0,last_archive_count;
static void draw_all(struct NDDock *d) {(void)d;}
static void *GetMsg(void *p) {(void)p;if(!pending)return NULL;pending=0;return &message;}
static void ReplyMsg(struct Message *p) {(void)p;++replies;}
static void *AllocVec(size_t n,int flags) {(void)flags;return calloc(1,n);}
static void FreeVec(void *p) {free(p);}
static void DisplayBeep(void *s) {(void)s;}
static void show_feedback(struct NDDock *d,const char *s,int error) {(void)d;(void)s;(void)error;}
static int hovered_tile(struct NDDock *d,int x,int y) {(void)d;(void)x;(void)y;return hover;}
static int hovered_scroll(struct NDDock *d,int x,int y) {(void)d;(void)x;(void)y;return 0;}
static int launch_action(struct NDDock *d,int *i) {(void)d;(void)i;++actions;return 1;}
static int is_trash_tile(struct NDDock *d,int i) {return i>=0 && !strcmp(d->conf.items[i].id,"TRASH");}
static int build_wb_path(struct WBArg *arg,char *p,size_t n,int *kind)
{(void)n;sprintf(p,"SYS:Tools/App%d",arg->number);*kind=ND_ITEM_LAUNCH;return 1;}
static int launch_plugin_helper(struct NDDock *d,int i,const char *a,const char *b)
{(void)d;(void)i;(void)a;(void)b;++actions;return 1;}
static int drop_extracts_archive(struct NDDock *d,int i) {return i>=0 && !strcmp(d->conf.items[i].id,"ARCHIVE");}
static int launch_archive_drop(struct NDDock *d,int i,const struct AppMessage *p)
{(void)d;(void)i;last_archive_count=p->am_NumArgs;++actions;return 1;}
static int drop_opens_launcher(struct NDDock *d,int i,int x,int y)
{(void)x;(void)y;return i>=0 && d->conf.items[i].kind==ND_ITEM_LAUNCH;}
static int open_dropped_objects(struct NDDock *d,int i,const struct AppMessage *m,int *failed)
{(void)d;(void)i;*failed=0;actions+=m->am_NumArgs;return m->am_NumArgs;}
static int drop_copies_into_drawer(struct NDDock *d,int i,int x,int y)
{(void)x;(void)y;return i>=0 && d->conf.items[i].kind==ND_ITEM_DRAWER;}
static int launch_command_console(const char *p) {(void)p;++actions;return 1;}
static int drop_after_tile(struct NDDock *d,int i,int x,int y)
{(void)d;(void)i;(void)y;return nd_drop_after_item(x,100);}
static int nd_media_persistent_count(const struct NDConfig *c) {return c->count;}
static int load_edit_configuration(struct NDDock *d,struct NDConfig *c) {*c=d->conf;return 0;}
void nd_label_from_path(const char *path,char *name,unsigned int n)
{(void)path;(void)n;strcpy(name,"App");}
static void dropped_icon_path(const char *p,int kind,char *icon,size_t n)
{(void)p;(void)kind;(void)n;icon[0]=0;}
int nd_insert_launcher_visible(struct NDConfig *c,int pos,int kind,const char *name,const char *target,const char *icon)
{(void)kind;(void)name;(void)target;(void)icon;assert(added<4);position[added++]=pos;++c->count;return 1;}
static int save_and_reload(struct NDDock *d,struct NDConfig **c)
{assert(replies==1);d->conf=**c;free(*c);*c=NULL;return 1;}
"""
        config_source = (ROOT/'src/config.c').read_text()
        start=config_source.index('int nd_shelf_plugin_path(')
        harness += '\n'+config_source[start:config_source.index('int nd_monitor_plugin_path(',start)]
        harness += '\n'+function(source, 'void dropped_icons(struct NDDock *dock)')
        harness += r"""
int main(void)
{
    static struct NDDock dock;
    struct WBArg args[3];
    const char *ids[]={"TRASH","BITSKIM","ARCHIVE","APP","DRAWER","SHELF_2"};
    int i,mode;
    args[0].number=0;args[1].number=1;
    message.am_ArgList=args;message.am_NumArgs=1;message.am_MouseX=75;
    dock.drop_port=&dock;
    for(mode=0;mode<=1;++mode) for(i=0;i<6;++i) {
        dock.edit_mode=mode;dock.conf.count=1;
        strcpy(dock.conf.items[0].id,ids[i]);
        strcpy(dock.conf.items[0].command,i==5 ? "Plugins/Shelf":"Work:Apps");
        dock.conf.items[0].kind=i==3 ? ND_ITEM_LAUNCH : i==4 ? ND_ITEM_DRAWER : ND_ITEM_PLUGIN;
        added=actions=replies=0;pending=1;
        dropped_icons(&dock);
        assert(replies==1);
        assert(added==mode);assert(actions==!mode);
        if(mode)assert(position[0]==1);
    }
    dock.edit_mode=0;dock.conf.count=1;pending=1;
    dock.conf.items[0].kind=ND_ITEM_PLUGIN;strcpy(dock.conf.items[0].id,"ARCHIVE");
    added=actions=replies=last_archive_count=0;message.am_NumArgs=3;
    dropped_icons(&dock);
    assert(replies==1 && actions==1 && !added && last_archive_count==3);
    dock.edit_mode=1;dock.conf.count=1;pending=1;
    added=actions=replies=0;message.am_NumArgs=2;message.am_MouseX=25;
    dropped_icons(&dock);
    assert(added==2 && position[0]==0 && position[1]==1 && actions==0 && replies==1);
    hover=-1;dock.conf.count=0;pending=1;added=actions=replies=0;
    dropped_icons(&dock);
    assert(added==2 && position[0]==0 && position[1]==1 && replies==1);
    dock.edit_mode=0;pending=1;added=actions=replies=0;
    dropped_icons(&dock);
    assert(added==0 && actions==0 && replies==1);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)
            (path/'drop.c').write_text(harness)
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c89', '-Wno-deprecated-declarations',
                            '-I'+str(ROOT/'include'), str(path/'drop.c'), str(ROOT/'src/interaction.c'),
                            '-o', str(path/'drop')], check=True)
            subprocess.run([str(path/'drop')], check=True)


if __name__ == '__main__':
    unittest.main()
