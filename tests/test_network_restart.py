#!/usr/bin/env python3
"""Exercise native provider command selection with cold and registered interfaces."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]

class Reconnect(unittest.TestCase):
    def test_reconnect_and_cold_start(self):
        source=(ROOT/'plugin_src/network/network.c').read_text()
        start=source.index('static int roadshow_profile(')
        function=source[start:source.index('static int a2065_present(',start)]
        harness=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "nd_netcontrol.h"
static int resident,registered,profile=1,dhcp=1,queries,count,fail_at;
static char calls[4][256];
static int stack_resident(void){return resident;}
static int exists(const char *path)
{
 assert(!strcmp(path,"DEVS:NetInterfaces/A2065") || !strcmp(path,"SYS:Storage/NetInterfaces/A2065"));
 return profile;
}
static int command(const char *text)
{
 if(!strcmp(text,"C:ShowNetStatus INTERFACE=\"A2065\" QUIET")){
  ++queries;return registered ? 0:5;
 }
 assert(count<4);strcpy(calls[count++],text);return count==fail_at ? 5:0;
}
#define rd_interface_uses_dhcp(path) (dhcp)
'''+function+r'''
int main(void)
{
 struct NCConfig cfg;char out[256];nc_defaults(&cfg);strcpy(cfg.interface_name,"A2065");
 assert(roadshow_online(&cfg)==0 && count==1 && !queries);
 assert(!strcmp(calls[0],"C:AddNetInterface DEVS:NetInterfaces/A2065 QUIET TIMEOUT=60"));
 count=0;resident=1;registered=0;
 assert(roadshow_online(&cfg)==0 && count==1 && queries==1);
 assert(!strcmp(calls[0],"C:AddNetInterface DEVS:NetInterfaces/A2065 QUIET TIMEOUT=60"));
 count=0;registered=1;
 assert(roadshow_online(&cfg)==0 && count==2);
 assert(!strcmp(calls[0],"C:ConfigureNetInterface A2065 ONLINE QUIET"));
 assert(!strcmp(calls[1],"C:ConfigureNetInterface A2065 CONFIGURE=DHCP TIMEOUT=60 QUIET"));
 count=0;fail_at=1;assert(roadshow_online(&cfg)==5 && count==1);
 count=0;fail_at=2;assert(roadshow_online(&cfg)==5 && count==2);
 count=0;fail_at=0;dhcp=0;
 assert(roadshow_online(&cfg)==0 && count==1);
 assert(!strcmp(calls[0],"C:ConfigureNetInterface A2065 ONLINE QUIET"));
 count=0;assert(roadshow_offline(&cfg)==0 && count==1);
 assert(!strcmp(calls[0],"C:ConfigureNetInterface A2065 OFFLINE QUIET"));
 assert(!roadshow_command(&cfg,NC_ONLINE,out,8));
 count=0;profile=0;assert(roadshow_online(&cfg)==20 && !count);
 assert(nc_control_request(1,NC_OFFLINE,0,1,NC_ONLINE)==NC_REQUEST_RESTART);
 assert(nc_control_request(1,NC_ONLINE,0,0,NC_ONLINE)==NC_REQUEST_BUSY);
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as folder:
            temp=Path(folder);(temp/'test.c').write_text(harness)
            subprocess.run([os.environ.get('CC','cc'),'-std=c89','-Wall','-Wextra','-pedantic',
                '-I'+str(ROOT/'include'),'-I'+str(ROOT/'plugin_src/network'),str(temp/'test.c'),
                str(ROOT/'plugin_src/network/nd_netcontrol.c'),str(ROOT/'plugin_src/network/nd_network.c'),
                str(ROOT/'plugin_src/network/nd_roadshow.c'),'-o',str(temp/'test')],check=True)
            subprocess.run([str(temp/'test')],check=True)

if __name__=='__main__':unittest.main()
