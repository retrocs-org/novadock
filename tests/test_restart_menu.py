#!/usr/bin/env python3
"""Exercise the real background-menu dispatcher without an Amiga desktop."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
menu=(root/'src/dock_menu.c').read_text()
start=menu.index('/* The background (dock) menu, top to bottom.')
rows_code=menu[start:menu.index('static int popup_count_for(',start)]
start=menu.index('static int dock_popup_action(struct NDDock *dock, int action)\n{')
end=menu.index('\nvoid popup_events(',start)
body=menu[start:end]
labels=re.search(r'static const char \*dock_actions\[\] = \{(.*?)\};',menu,re.S).group(1)
labels=re.findall(r'"([^"]+)"',labels)
assert labels[-2:]==['Restart NovaDock','Quit NovaDock']
assert 'if (dock->popup_item == -2) {int rows[ND_DOCK_MENU_ROWS];return dock_menu_rows(dock,rows);}' in menu
harness=r"""
#include <assert.h>
#include <stddef.h>
#include <string.h>
#define ND_MENU_PROFILES 4
#define ND_PROFILE_LEN 25
struct NDDock {int running,restart_requested,edit_mode;void *screen;int undo;
 struct {char profile[ND_PROFILE_LEN];} conf;char menu_profiles[ND_MENU_PROFILES][ND_PROFILE_LEN];int menu_profile_count;};
static int undos,saved,used;static char last[ND_PROFILE_LEN];
static int dock_has_undo(const struct NDDock *d){return d->edit_mode && d->undo;}
static int undo_last_edit(struct NDDock *d){++undos;d->undo=0;return 1;}
static void set_edit_mode(struct NDDock *d,int edit){d->edit_mode=edit;}
static int launch_companion(struct NDDock *d,const char *name,const char *args){(void)d;(void)name;(void)args;return 1;}
static void DisplayBeep(void *screen){(void)screen;}
static int artwork;
static void reload_artwork(struct NDDock *d){(void)d;++artwork;}
static void open_about(struct NDDock *d){(void)d;}
static int save_profile(struct NDDock *d,const char *n){(void)d;++saved;strcpy(last,n);return 1;}
static int use_profile(struct NDDock *d,const char *n){(void)d;++used;strcpy(last,n);return 1;}
"""+rows_code+body+r"""
static int row_for(struct NDDock *d,int code){int r[ND_DOCK_MENU_ROWS],i,n=dock_menu_rows(d,r);for(i=0;i<n;++i)if(r[i]==code)return i;return -1;}
int main(void){struct NDDock dock;int rows[ND_DOCK_MENU_ROWS];
memset(&dock,0,sizeof(dock));dock.running=1;
assert(dock_menu_rows(&dock,rows)==7);
assert(!dock_popup_action(&dock,row_for(&dock,DM_ARTWORK)) && artwork==1 && dock.running);
assert(dock_popup_action(&dock,row_for(&dock,DM_RESTART))==1 && !dock.running && dock.restart_requested);
dock.running=1;assert(dock_popup_action(&dock,row_for(&dock,DM_QUIT))==1 && !dock.running && !dock.restart_requested);
dock.running=1;assert(!dock_popup_action(&dock,-1) && !dock_popup_action(&dock,7) && dock.running);
dock.edit_mode=1;dock.undo=1;assert(!dock_popup_action(&dock,row_for(&dock,DM_UNDO)) && undos==1 && dock.running);
dock.undo=1;assert(dock_popup_action(&dock,row_for(&dock,DM_RESTART))==1 && !dock.running && dock.restart_requested && undos==1);
dock.running=1;dock.restart_requested=0;dock.undo=0;dock.edit_mode=0;
strcpy(dock.menu_profiles[0],"Games");strcpy(dock.menu_profiles[1],"Work");dock.menu_profile_count=2;strcpy(dock.conf.profile,"Home");
assert(dock_menu_rows(&dock,rows)==10);
assert(!dock_popup_action(&dock,row_for(&dock,DM_PROFILE_USE+1)) && used==1 && !strcmp(last,"Work"));
assert(!dock_popup_action(&dock,row_for(&dock,DM_PROFILE_UPDATE)) && saved==1 && !strcmp(last,"Home"));
assert(dock_popup_action(&dock,row_for(&dock,DM_QUIT))==1 && !dock.running);
dock.running=1;dock.menu_profile_count=9;assert(dock_menu_rows(&dock,rows)==ND_DOCK_MENU_ROWS-1);
dock.edit_mode=1;dock.undo=1;assert(dock_menu_rows(&dock,rows)==ND_DOCK_MENU_ROWS);
return 0;}
"""
with tempfile.TemporaryDirectory() as tmp:
    source=Path(tmp)/'menu.c';source.write_text(harness);exe=Path(tmp)/'menu'
    subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c89','-Wall','-Wextra','-pedantic',str(source),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
main=(root/'src/novadock.c').read_text();cleanup=main.split('\ncleanup:',1)[1]
launch=cleanup.index('nd_control_launch_dock()')
for operation in ('close_settings_writer(dock)','stop_rexx(dock)','stop_hotkeys(dock)','remove_port(dock)','release_icons(dock)','free_colors(dock)'):
    assert cleanup.index(operation)<launch,operation+' must finish before relaunch'
assert main.index('finish_settings_saves(dock)')<main.index('dock->restart_requested=0;')<main.index('\ncleanup:')
print('Dock menu: undo row, profile rows, restart before quit, distinct actions and shutdown-before-relaunch ordering passed.')
