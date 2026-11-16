#!/usr/bin/env python3
"""Exercise the real background-menu dispatcher without an Amiga desktop."""
from pathlib import Path
import os,re,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
menu=(root/'src/dock_menu.c').read_text()
start=menu.index('static int dock_popup_action(struct NDDock *dock, int action)\n{')
end=menu.index('\nvoid popup_events(',start)
body=menu[start:end]
labels=re.search(r'static const char \*dock_actions\[\] = \{(.*?)\};',menu,re.S).group(1)
labels=re.findall(r'"([^"]+)"',labels)
assert labels[-2:]==['Restart NovaDock','Quit NovaDock']
assert 'if (dock->popup_item == -2) return '+str(len(labels))+';' in menu
harness=r'''
#include <assert.h>
#include <stddef.h>
struct NDDock {int running,restart_requested,edit_mode;void *screen;};
static void set_edit_mode(struct NDDock *d,int edit){d->edit_mode=edit;}
static int launch_companion(struct NDDock *d,const char *name,const char *args){(void)d;(void)name;(void)args;return 1;}
static void DisplayBeep(void *screen){(void)screen;}
static void open_about(struct NDDock *d){(void)d;}
'''+body+r'''
int main(void){struct NDDock dock={1,0,0,NULL};
assert(dock_popup_action(&dock,RESTART)==1 && !dock.running && dock.restart_requested);
dock.running=1;
assert(dock_popup_action(&dock,QUIT)==1 && !dock.running && !dock.restart_requested);
dock.running=1;
assert(!dock_popup_action(&dock,3) && dock.running && !dock.restart_requested);
assert(!dock_popup_action(&dock,-1) && dock.running && !dock.restart_requested);
return 0;}
'''
harness=harness.replace('RESTART',str(labels.index('Restart NovaDock'))).replace('QUIT',str(labels.index('Quit NovaDock')))
with tempfile.TemporaryDirectory() as tmp:
    source=Path(tmp)/'menu.c';source.write_text(harness);exe=Path(tmp)/'menu'
    subprocess.run([os.environ.get('HOST_CC','cc'),'-std=c89','-Wall','-Wextra','-pedantic',str(source),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
main=(root/'src/novadock.c').read_text();cleanup=main.split('\ncleanup:',1)[1]
launch=cleanup.index('nd_control_launch_dock()')
for operation in ('close_settings_writer(dock)','stop_rexx(dock)','stop_hotkeys(dock)','remove_port(dock)','release_icons(dock)','free_colors(dock)'):
    assert cleanup.index(operation)<launch,operation+' must finish before relaunch'
assert main.index('finish_settings_saves(dock)')<main.index('dock->restart_requested=0;')<main.index('\ncleanup:')
print('Dock menu: restart before quit, distinct actions and shutdown-before-relaunch ordering passed.')
