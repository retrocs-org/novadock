#!/usr/bin/env python3
"""Exercise production directory-watch lifecycle with a fake filesystem."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
class TrashNotifications(unittest.TestCase):
    def test_watch_lifecycle_and_coalesced_external_changes(self):
        source=(ROOT/'plugin_src/trash/trash.c').read_text()
        start=source.index('#define TRASH_WATCH_MAX')
        end=source.index('/* ---- PUT ',start)
        harness=r"""
#include <assert.h>
#include <string.h>
#define TRASH_MAX_DIRS 16
#define TRASH_PATH 300
#define TRASH_CHANGED "T:NovaDock-trash.changed"
#define NRF_SEND_MESSAGE 1
#define NRF_WAIT_REPLY 8
typedef char *STRPTR;
struct Message {int unused;};
struct MsgPort {int pending;};
struct NotifyRequest {
 STRPTR nr_Name;unsigned long nr_Flags,nr_UserData;
 union {struct {struct MsgPort *nr_Port;} nr_Msg;} nr_stuff;
 unsigned long reserved[4];
};
struct NotifyMessage {struct Message msg;struct NotifyRequest *nm_NReq;};
static int starts,ends,replies,deleted,fail,known=1;
static struct NotifyRequest *live[32];
static int lookups,marker,deletes;
static int exists(const char *p) {(void)p;++lookups;return marker;}
static int DeleteFile(STRPTR p) {(void)p;++deletes;marker=0;return 1;}
static struct NotifyRequest notified;
static struct NotifyMessage message;
static int StartNotify(struct NotifyRequest *r)
{int i;assert(r->nr_Flags==(NRF_SEND_MESSAGE|NRF_WAIT_REPLY));
 for(i=0;i<4;++i)assert(!r->reserved[i]);
 ++starts;if(fail)return 0;live[starts-1]=r;return 1;}
static void EndNotify(struct NotifyRequest *r) {(void)r;++ends;}
static struct Message *GetMsg(struct MsgPort *p)
{if(!p->pending)return 0;--p->pending;message.nm_NReq=&notified;return &message.msg;}
static void ReplyMsg(struct Message *m) {assert(m==&message.msg);++replies;}
static void DeleteMsgPort(struct MsgPort *p) {assert(!p->pending);++deleted;}
static int known_trashcans(char dirs[][TRASH_PATH])
{strcpy(dirs[0],"System:Trashcan");if(known>1)strcpy(dirs[1],"Data:Trashcan");return known;}
"""+source[start:end]+r"""
int main(void)
{
 static struct TrashWatch w;struct MsgPort port={0};char longpath[300];int i,before;
 w.port=&port;notified.nr_UserData=1;
 watch_trashcans(&w);assert(starts==2&&w.count==2);
 assert(!strcmp(live[0]->nr_Name,"SYS:Trashcan"));
 assert(!strcmp(live[1]->nr_Name,"System:Trashcan"));
 watch_trashcans(&w);assert(starts==2); /* unchanged watches are not rearmed */
 known=2;watch_trashcans(&w);assert(starts==3&&w.count==3);
 assert(!strcmp(live[0]->nr_Name,"SYS:Trashcan")); /* stable names */
 assert(!trash_changed(&w));assert(!replies);
 port.pending=3;assert(trash_changed(&w));assert(replies==3);
 assert(!trash_changed(&w)); /* a burst causes only one recount */
 notified.nr_UserData=2;port.pending=1;assert(trash_changed(&w)==2);
 notified.nr_UserData=1; /* marker-only events are distinct from directory changes */
 fail=1;watch_path(&w,"Other:Trashcan");assert(w.count==3&&ends==1);
 fail=0;watch_path(&w,"Other:Trashcan");assert(w.count==4);
 memset(longpath,'x',sizeof(longpath));longpath[299]=0;
 i=starts;watch_path(&w,longpath);assert(starts==i);
 w.marker_watched=1;
 for(i=0;i<60;++i)assert(!trash_refresh_pending(&w));
 assert(!lookups); /* zero idle filesystem calls over one minute */
 notified.nr_UserData=1;port.pending=2;
 assert(trash_refresh_pending(&w));assert(!lookups);
 notified.nr_UserData=2;port.pending=1;
 assert(!trash_refresh_pending(&w));assert(lookups==1&&!deletes);
 marker=1;port.pending=1;assert(trash_refresh_pending(&w));assert(deletes==1);
 assert(!trash_refresh_pending(&w)); /* no repeat recount */
 w.marker_watched=0;marker=1;assert(trash_refresh_pending(&w));assert(deletes==2);
 notified.nr_UserData=1;before=replies;

 port.pending=2;stop_trash_watch(&w);
 assert(ends==5&&replies==before+2&&deleted==1&&!w.port&&!w.count);
 watch_trashcans(&w);assert(!trash_changed(&w)); /* no port: safe manual fallback */
 stop_trash_watch(&w);assert(deleted==1);
 return 0;
}
"""
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp);(path/'watch.c').write_text(harness)
            subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra',
                            str(path/'watch.c'),'-o',str(path/'watch')],check=True)
            subprocess.run([str(path/'watch')],check=True)
if __name__=='__main__':unittest.main()
