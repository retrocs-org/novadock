#!/usr/bin/env python3
"""Validate external artwork banks, state selection, SDK fallback and HASS pixels."""
from pathlib import Path
import importlib.util,os,struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from pack_plugin_art import build_bank
from generate_plugin_status_art import SOURCES,source_frames
import generate_hass_device_icons as hass
from generate_collection_icons import PALETTE
bank=(ROOT/'Plugins/HASS.assets/Artwork.ndart').read_bytes()
assert bank==build_bank([{'name':k,'state':s,'pixels':hass.artwork(k,s)} for k in hass.KINDS for s in range(3)]+[{'name':'SadHASS','state':0,'pixels':hass.bridge_art(True)},{'name':'default','state':0,'pixels':hass.artwork('HASS',2)}],PALETTE)
# Independently decode the runtime bank, including the final frame and alpha.
count=struct.unpack_from('>H',bank,6)[0];seen=set()
for i in range(count):
    name,state,reserved,n,offset=struct.unpack_from('>24sBBHI',bank,56+i*32);name=name.rstrip(b'\0').decode();pixels=[[0]*32 for _ in range(32)]
    assert not reserved and (name,state) not in seen;seen.add((name,state))
    for j in range(n):
        x,y,w,h,color=bank[offset+j*5:offset+j*5+5]
        for yy in range(y,y+h):
            for xx in range(x,x+w):pixels[yy][xx]=color
    expected=hass.bridge_art(True) if name=='SadHASS' else hass.artwork('HASS',2) if name=='default' else hass.artwork(name,state)
    assert pixels==expected,(name,state)
for kind in hass.KINDS:
    assert hass.artwork(kind,0)!=hass.artwork(kind,1) or kind in ('Door','FrontDoor','DoorSensor','Garage','WindowSensor','Thermometer','Moisture','Leak','FireAlarm','Sensor','Scene','HASS')
    assert not (ROOT/'Icons'/f'HASS{kind}On.iff').exists()
# Each status bank is owned by its plugin and retains its original indexed pixels.
status_paths=[]
for program,folder,name in SOURCES:
    path=ROOT/'Plugins'/(program+'.assets')/'Artwork.ndart';status_paths.append(path)
    frames,palette=source_frames(ROOT/'plugin_src'/folder/'artwork'/(name+'.json'))
    data=path.read_bytes();assert data==build_bank(frames,palette)
    assert struct.unpack_from('>H',data,6)[0]==1
    pixels=[[0]*32 for _ in range(32)]
    label,state,reserved,n,offset=struct.unpack_from('>24sBBHI',data,56)
    assert label.rstrip(b'\0').decode()==name and not state and not reserved
    for j in range(n):
        x,y,w,h,color=data[offset+j*5:offset+j*5+5]
        for yy in range(y,y+h):
            for xx in range(x,x+w):pixels[yy][xx]=color
    assert pixels==frames[0]['pixels']
    assert not (ROOT/'Icons'/(name+'.iff')).exists()
    assert name not in (ROOT/'src/dock_draw.c').read_text()
program=r'''
#include <assert.h>
#include <string.h>
#include "nd_artwork.h"
#include "nd_hass_art.h"
typedef unsigned short UWORD;
struct NDClient {UWORD host_artwork,host_artwork_meta,art_sending,art_state,art_mode,quiet_valid;unsigned long art_key;};
#define ND_NO_METER 65535U
static char shown[32];
static unsigned int nd_client_update_quiet(struct NDClient *c,const char *caption,const char *value,unsigned int meter,unsigned int elapsed,unsigned int heartbeat){(void)c;assert(!strcmp(value,"off")&&meter==65535U&&elapsed==50&&heartbeat==500);strcpy(shown,caption);return 123;}
'''
s=(ROOT/'plugin_src/common/nd_client.c').read_text();program+=s[s.index('unsigned int nd_client_update_art_quiet('):]
program+=r'''
int main(int argc,char **argv){struct NDArtwork a;struct NDClient c;unsigned char *d;unsigned int n,state;unsigned long i;char path[520],caption[32],name[24];const unsigned char *off,*on;
 assert(argc==5);memset(&a,0,sizeof(a));assert(nd_artwork_load(&a,argv[1]));
 off=nd_artwork_frame(&a,"Bulb",0,&n);assert(off&&n);on=nd_artwork_frame(&a,"Bulb",1,&n);assert(on&&on!=off);
 assert(!nd_artwork_frame(&a,"missing",0,&n)&&n==0);
 assert(nd_artwork_path("SYS:Resources/NovaDock","Plugins/HASS",path,sizeof(path))&&!strcmp(path,"SYS:Resources/NovaDock/Plugins/HASS.assets/Artwork.ndart"));
 assert(nd_artwork_path("ignored","Work:Plugins/Example",path,sizeof(path))&&!strcmp(path,"Work:Plugins/Example.assets/Artwork.ndart"));
 assert(!nd_artwork_path("base","program",path,5));
 assert(nd_artwork_caption(caption,"ChristmasLights",2));assert(nd_artwork_parse(caption,name,&state)&&state==2&&!strcmp(name,"ChristmasLights"));
 assert(!nd_artwork_parse("ART:bad/path:1",name,&state));assert(!nd_artwork_parse("ART:lamp:256",name,&state));assert(!nd_artwork_parse("ART:lamp:",name,&state));
 memset(&c,0,sizeof(c));c.host_artwork=0;assert(nd_client_update_art_quiet(&c,"LAMP","off","Bulb",0,50,500)==123&&!strcmp(shown,"LAMP"));
 c.host_artwork=1;nd_client_update_art_quiet(&c,"LAMP","off","Bulb",0,50,500);assert(!strcmp(shown,"ART:Bulb:0"));
 nd_client_update_art_quiet(&c,"LAMP","off","bad/name",0,50,500);assert(!strcmp(shown,"LAMP"));
 c.host_artwork_meta=1;c.quiet_valid=1;nd_client_update_art_quiet(&c,"LAMP","off","Bulb",0,50,500);assert(!strcmp(shown,"LAMP")&&c.art_key==nd_artwork_key("Bulb")&&c.art_state==0&&!c.art_sending&&!c.quiet_valid);
 c.quiet_valid=1;nd_client_update_art_quiet(&c,"LAMP","off","Bulb",1,50,500);assert(c.art_state==1&&!c.quiet_valid);
 assert(nd_hass_visual_state("light.desk","off ")==0&&nd_hass_visual_state("light.desk","on ")==1);
 assert(nd_hass_visual_state("light.desk","Waiting for AmiHASS")==2);
 assert(nd_hass_visual_state("cover.garage","closed")==0&&nd_hass_visual_state("lock.front","unlocked")==1);
 assert(!strcmp(nd_hass_auto_icon("light.strip","",""),"LightStrip"));
 assert(!strcmp(nd_hass_auto_icon("light.tree","","Christmas tree"),"ChristmasLights"));
 assert(!strcmp(nd_hass_auto_icon("binary_sensor.door","door",""),"DoorSensor"));
 assert(!strcmp(nd_hass_auto_icon("sensor.cpu","temperature",""),"Thermometer"));
 d=(unsigned char *)malloc(a.size);assert(d);memcpy(d,a.data,a.size);
 for(i=0;i<56;++i){unsigned char old=d[i];d[i]^=255;if(i<8)assert(!nd_artwork_validate(d,a.size));else assert(nd_artwork_validate(d,a.size));d[i]=old;}
 assert(!nd_artwork_validate(d,55));d[56+25]=1;assert(!nd_artwork_validate(d,a.size));d[56+25]=0;
 d[56+28]=255;assert(!nd_artwork_validate(d,a.size));memcpy(d,a.data,a.size);
 i=nd_artwork_u32(d+56+28);d[i]=32;assert(!nd_artwork_validate(d,a.size));memcpy(d,a.data,a.size);d[i+2]=0;assert(!nd_artwork_validate(d,a.size));
 free(d);nd_artwork_close(&a);
 {const char *names[]={"SadWHDLoad","SadMusic","SadSensor"};for(i=0;i<3;++i){assert(nd_artwork_load(&a,argv[i+2]));assert(nd_artwork_frame(&a,names[i],0,&n)&&n);assert(!nd_artwork_frame(&a,names[i],1,&n));nd_artwork_close(&a);}}
 assert(!a.data&&!a.size);assert(!nd_artwork_load(&a,"missing-artwork-bank"));return 0;}
'''
with tempfile.TemporaryDirectory() as t:
    p=Path(t);(p/'check.c').write_text(program)
    cmd=[os.environ.get('CC','cc'),'-std=c99','-Wno-deprecated-declarations','-I'+str(ROOT/'include'),str(p/'check.c'),'-o',str(p/'check')]
    if os.environ.get('SANITIZE')=='1':cmd+=['-fsanitize=address,undefined']
    subprocess.run(cmd,check=True);subprocess.run([str(p/'check'),str(ROOT/'Plugins/HASS.assets/Artwork.ndart'),*[str(path) for path in status_paths]],cwd=p,check=True)
subprocess.run([sys.executable,str(ROOT/'tools/generate_hass_device_icons.py'),'--check'],check=True)
subprocess.run([sys.executable,str(ROOT/'tools/generate_plugin_status_art.py'),'--check'],check=True)
print('Plugin artwork banks: bounds, corruption rejection, state pixels, SDK fallback and external HASS assets passed.')
