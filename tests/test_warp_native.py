#!/usr/bin/env python3
"""Run the actual native-operation dispatcher against explicit fake hardware."""
from pathlib import Path
import os, subprocess, tempfile, unittest
ROOT=Path(__file__).resolve().parents[1]
def function(text,name):
    start=text.index('static void '+name+'(')
    return text[start:text.index('\n}',start)+2]
class WarpNativeTests(unittest.TestCase):
    def test_hardware_gates_write_order_and_secret_cleanup(self):
        source=(ROOT/'plugin_src/warp/nd_warp_ui.c').read_text()
        defines=source[source.index('#define WARP_OK'):source.index('struct WarpMessage')]
        harness=r'''
#include <assert.h>
#include <string.h>
#include "nd_warp_control.h"
#include "nd_prism.h"
struct Library {unsigned int lib_Version,lib_Revision;};
struct WarpMessage {int operation,level,result,open_network;char ssid[33],password[64];unsigned long khz,unit;struct NDPrismScan scan;};
static struct Library lib={30,74};
static int present=1,diag_ok=1,ssid_ok=1,pass_ok=1,freq_ok=1,diag_calls,ssid_calls,pass_calls,freq_calls,closes;
static const char *model="Warp1260";
static unsigned char *diag_buffer,*packet_buffer;static char *pass_buffer;
static int nd_warp_control_present(void){return present;}
static struct Library *OpenLibrary(const char *name,int v){assert(!strcmp(name,"cswarp.library") && v==30);return &lib;}
static int nd_cswarp_diag(struct Library *b,unsigned char *data)
{assert(b==&lib);++diag_calls;diag_buffer=data;memset(data,0,428);strcpy((char *)data,model);strcpy((char *)data+84,"configured-wifi");strcpy((char *)data+212,"board-stored-private-password");data[382]=0x42;data[383]=0x96;return diag_ok;}
static int nd_cswarp_ssid(struct Library *b,char *ssid)
{assert(b==&lib && !strcmp(ssid,"test-wifi"));assert(!pass_calls);++ssid_calls;return ssid_ok;}
static int nd_cswarp_password(struct Library *b,char *password)
{assert(b==&lib && !strcmp(password,"test-secret") && ssid_calls==1);++pass_calls;pass_buffer=password;return pass_ok;}
static int nd_cswarp_frequency(struct Library *b,unsigned char *packet)
{assert(b==&lib && packet[0]==0x42 && packet[1]==0xb4 && packet[9]==1);++freq_calls;packet_buffer=packet;return freq_ok;}
static void CloseLibrary(struct Library *b)
{unsigned int i;assert(b==&lib);++closes;if(diag_buffer)for(i=0;i<428;++i)assert(!diag_buffer[i]);if(packet_buffer)for(i=0;i<10;++i)assert(!packet_buffer[i]);if(pass_buffer)for(i=0;i<64;++i)assert(!pass_buffer[i]);}
int nd_prism_scan(unsigned long unit,struct NDPrismScan *s){assert(unit==0);s->count=0;return ND_PRISM_OK;}
int nd_prism_save(const char *ssid,const char *pass,int open){assert(!strcmp(ssid,"test-wifi") && !strcmp(pass,"test-secret") && !open);return ND_PRISM_OK;}
'''+defines+function(source,'board_operation')+r'''
static void reset(struct WarpMessage *m,int operation)
{memset(m,0,sizeof(*m));m->operation=operation;m->level=4;strcpy(m->ssid,"test-wifi");strcpy(m->password,"test-secret");diag_calls=ssid_calls=pass_calls=freq_calls=closes=0;diag_buffer=packet_buffer=NULL;pass_buffer=NULL;}
static void cleared(struct WarpMessage *m){unsigned int i;for(i=0;i<64;++i)assert(!m->password[i]);}
int main(void)
{
 struct WarpMessage m;
 reset(&m,ND_WARP_QUERY);board_operation(&m);assert(m.result==WARP_OK && m.khz==75000 && !strcmp(m.ssid,"configured-wifi"));assert(!ssid_calls && !pass_calls && !freq_calls);cleared(&m);
 reset(&m,ND_WARP_WIFI);present=0;board_operation(&m);assert(m.result==WARP_NO_BOARD && !diag_calls && !closes);cleared(&m);present=1;
 reset(&m,ND_WARP_WIFI);lib.lib_Revision=73;board_operation(&m);assert(m.result==WARP_LIBRARY && !diag_calls && !ssid_calls);lib.lib_Revision=74;cleared(&m);
 reset(&m,ND_WARP_WIFI);model="Warp1240";board_operation(&m);assert(m.result==WARP_MODEL && !ssid_calls && !pass_calls);model="Warp1260";cleared(&m);
 reset(&m,ND_WARP_TURBO);diag_ok=0;board_operation(&m);assert(m.result==WARP_DIAG_FAILED && !freq_calls);diag_ok=1;
 reset(&m,ND_WARP_WIFI);ssid_ok=0;board_operation(&m);assert(m.result==WARP_SSID_FAILED && ssid_calls==1 && !pass_calls);ssid_ok=1;cleared(&m);
 reset(&m,ND_WARP_WIFI);pass_ok=0;board_operation(&m);assert(m.result==WARP_PASS_FAILED && pass_calls==1);pass_ok=1;cleared(&m);
 reset(&m,ND_WARP_WIFI);board_operation(&m);assert(m.result==WARP_OK && pass_calls==1 && ssid_calls==1);cleared(&m);
 reset(&m,ND_WARP_WIFI);m.password[0]=0;board_operation(&m);assert(m.result==WARP_INVALID && !ssid_calls && !pass_calls);
 reset(&m,ND_WARP_TURBO);m.level=8;board_operation(&m);assert(m.result==WARP_INVALID && !freq_calls);
 reset(&m,ND_WARP_TURBO);board_operation(&m);assert(m.result==WARP_OK && freq_calls==1 && !m.khz);cleared(&m);
 reset(&m,ND_PRISM_SCAN);board_operation(&m);assert(m.result==ND_PRISM_OK && !diag_calls);cleared(&m);
 reset(&m,ND_PRISM_SAVE);board_operation(&m);assert(m.result==ND_PRISM_OK && !diag_calls);cleared(&m);
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            p=Path(d);(p/'native.c').write_text(harness)
            subprocess.run([os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-fsanitize=address,undefined',
                '-I'+str(ROOT/'plugin_src/warp'),'-I'+str(ROOT/'plugin_src/prism'),str(p/'native.c'),str(ROOT/'plugin_src/warp/nd_warp_control.c'),'-o',str(p/'native')],check=True)
            subprocess.run([str(p/'native')],check=True)
if __name__=='__main__':unittest.main()
