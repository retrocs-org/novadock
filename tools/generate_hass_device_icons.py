#!/usr/bin/env python3
"""Generate automatic HASS device/state art in the existing dock icon style."""
from pathlib import Path
import argparse
from generate_collection_icons import Canvas,artwork as collection_art,png,ilbm,PALETTE
from generate_hass_icons import artwork as bridge_art,rectangles
from pack_plugin_art import build_bank
ROOT=Path(__file__).resolve().parents[1]
KINDS=('Bulb','LightStrip','LightGroup','ChristmasLights','CeilingLight','FloorLamp','ShelfLamp','Switch','Fan','TV','AC','Heater','Door','FrontDoor','Garage','WindowSensor','DoorSensor','Thermometer','Moisture','Leak','FireAlarm','Sensor','Scene','HASS')
STATES=('Off','On','Unknown')
def artwork(kind,state):
    c=Canvas();b=c.box;l=c.line
    if kind=='HASS':c.p=bridge_art(False)
    elif kind=='Switch':
        c.frame(7,2,19,28,3);b(12,6,9,18,1);b(13,7 if state==1 else 15,7,8,5 if state==1 else 2)
        b(15,8 if state==1 else 16,3,2,3)
    else:c.p=collection_art(kind)
    if state!=1:
        # Unlit glass, screens and indicator lamps keep their solid silhouette.
        c.p=[[2 if v in (4,5,6,7) else v for v in row] for row in c.p]
        if kind=='Bulb':
            for x,y,w,h in ((0,0,32,4),(0,4,6,17),(26,4,6,17)):b(x,y,w,h,0)
            b(11,7,9,11,2);l(12,17,15,20,3);l(15,20,19,17,3)
        elif kind=='CeilingLight':b(0,23,32,7,0)
        elif kind=='ShelfLamp':b(16,14,13,8,0)
        elif kind=='AC':b(0,20,32,12,0)
        elif kind=='Heater':b(0,0,32,4,0)
        elif kind=='TV':b(7,9,18,11,1);b(25,24,2,1,2)
    if state==1 and kind in ('Door','FrontDoor','DoorSensor'):
        b(8,5,16,24,1);l(10,5,19,8,2);l(19,8,19,28,2);l(19,28,10,28,2);b(11,8,6,18,4);b(16,19,1,2,6)
    elif state==1 and kind=='Garage':
        b(7,14,19,14,1);b(7,14,19,3,3);b(9,26,15,2,2);l(7,17,7,27,4);l(25,17,25,27,4)
    elif state==1 and kind=='WindowSensor':
        b(5,6,16,20,1);l(6,6,16,9,3);l(16,9,16,25,3);l(16,25,6,26,3);b(8,10,6,12,4);b(26,10,3,3,7)
    elif state==1 and kind=='Fan':
        l(1,4,3,1,4);l(1,4,1,9,4);l(29,25,31,22,4);l(31,22,31,17,4)
    if state==2:
        # Unknown/pending must not look like an illuminated or active device.
        b(22,21,10,11,1);b(24,22,5,1,3);b(28,23,2,3,3);b(26,25,3,2,3);b(25,27,2,1,3);b(25,30,2,1,3)
    return c.p

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');parser.add_argument('--clean-legacy',type=Path);args=parser.parse_args()
    if args.clean_legacy:
        removed=0
        old=[('HASS'+kind+state,artwork(kind,i)) for kind in KINDS for i,state in enumerate(STATES)] + [('HASS',bridge_art(False)),('SadHASS',bridge_art(True))]
        for name,pixels in old:
            for extension,encode in [('png',png),('iff',ilbm)]:
                p=args.clean_legacy/(name+'.'+extension)
                if p.is_file() and p.read_bytes()==encode(pixels):p.unlink();removed+=1
        print(f'Removed {removed} unchanged legacy HASS icon copies.');return
    frames=[{'name':kind,'state':i,'pixels':artwork(kind,i)} for kind in KINDS for i in range(3)]
    frames += [{'name':'SadHASS','state':0,'pixels':bridge_art(True)},{'name':'default','state':0,'pixels':artwork('HASS',2)}]
    files={ROOT/'Plugins/HASS.assets/Artwork.ndart':build_bank(frames,PALETTE)}
    for kind in KINDS:
        for i,state in enumerate(STATES):
            pixels=artwork(kind,i)
            for extension,encode in [('png',png),('iff',ilbm)]:files[ROOT/'plugin_src/hass/artwork'/f'{kind}{state}.{extension}']=encode(pixels)
    for path,data in files.items():
        if args.check:assert path.read_bytes()==data,str(path)+' needs regeneration'
        else:path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    print(f'{len(KINDS)} automatic HASS device icons with off/on/unknown states are current.')
if __name__=='__main__':main()
