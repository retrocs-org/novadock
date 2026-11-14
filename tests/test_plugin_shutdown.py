#!/usr/bin/env python3
"""Shutdown wakes quiet plugins and answers QUIT before removing the host."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class Shutdown(unittest.TestCase):
 def test_bounded_quit_drain(self):
  source=(ROOT/'src/dock_plugins.c').read_text();a=source.index('static void finish_plugin_clients(');code=source[a:source.index('\nvoid remove_port',a)]
  h=r"""#include <assert.h>
#include <string.h>
#define ND_ID_LEN 20
#define ND_MAX_ITEMS 48
#define ND_ITEM_PLUGIN 3
#define ND_WAKE_PORT_PREFIX "NovaDock.Wake."
#define ND_WAKE_SIGNAL (1UL<<15)
struct Item {int kind;char id[20];};struct Task {int number;};
struct MsgPort {struct Task *mp_SigTask;};struct NDDock {void *ipc;int is_public;struct {int count;struct Item items[48];}conf;};
static struct Task task;static struct MsgPort port={&task};
static int wakes,signals,forbidden,drains,delays;
static void wake_plugin(struct NDDock *d,int index){assert(d&&index<2);++wakes;}
static void Forbid(void){assert(!forbidden);forbidden=1;}
static void Permit(void){assert(forbidden);forbidden=0;}
static struct MsgPort *FindPort(const char *s){assert(forbidden);assert(!strcmp(s,"NovaDock.Wake.NOTES")||!strcmp(s,"NovaDock.Wake.TIMER"));return &port;}
static void Signal(struct Task *t,unsigned long bits){assert(forbidden&&t==&task&&bits==ND_WAKE_SIGNAL);++signals;}
static void process_ipc(struct NDDock *d,int quitting){assert(d->is_public&&quitting&&!forbidden);++drains;}
static void Delay(int ticks){assert(ticks==1&&drains>delays);++delays;}
"""+code+r"""
int main(void){struct NDDock d;memset(&d,0,sizeof(d));finish_plugin_clients(&d);assert(!wakes&&!drains);
 d.ipc=&port;finish_plugin_clients(&d);assert(!drains);d.is_public=1;d.conf.count=3;
 d.conf.items[0].kind=d.conf.items[1].kind=ND_ITEM_PLUGIN;strcpy(d.conf.items[0].id,"NOTES");strcpy(d.conf.items[1].id,"TIMER");
 finish_plugin_clients(&d);assert(wakes==2&&signals==2&&drains==51&&delays==50&&!forbidden);return 0;}
"""
  with tempfile.TemporaryDirectory() as t:
   p=Path(t);(p/'check.c').write_text(h)
   subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',str(p/'check.c'),'-o',str(p/'check')],check=True)
   subprocess.run([str(p/'check')],check=True)
if __name__=='__main__':unittest.main()
