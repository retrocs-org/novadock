#!/usr/bin/env python3
"""Build plugin-owned status banks from their indexed artwork sources."""
import argparse,json
from pathlib import Path
from pack_plugin_art import build_bank
from artifact_compare import same_artifact
ROOT=Path(__file__).resolve().parents[1]
SOURCES=(('Games','games','SadWHDLoad'),('MusicLibrary','games','SadMusic'),('Sensors','sensors','SadSensor'))
def source_frames(path):
    doc=json.loads(path.read_text());frames=[]
    for frame in doc['frames']:
        frame=dict(frame);frame['pixels']=[[int(v,16) for v in row] if isinstance(row,str) else row for row in frame['pixels']];frames.append(frame)
    return frames,doc['palette']
def status_frames(program,folder,name):
    frames,palette=source_frames(ROOT/'plugin_src'/folder/'artwork'/(name+'.json'))
    if program=='Sensors':
        from generate_sensor_art import sensor_frames
        frames+=sensor_frames()
    return frames,palette
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');parser.add_argument('--clean-legacy',type=Path);args=parser.parse_args();removed=0
    for program,folder,name in SOURCES:
        source=ROOT/'plugin_src'/folder/'artwork';frames,palette=status_frames(program,folder,name)
        if args.clean_legacy:
            for ext in ('png','iff'):
                old=args.clean_legacy/(name+'.'+ext)
                if old.is_file() and same_artifact(old.read_bytes(),(source/(name+'.'+ext)).read_bytes()):old.unlink();removed+=1
            continue
        target=ROOT/'Plugins'/(program+'.assets')/'Artwork.ndart';data=build_bank(frames,palette)
        if args.check:
            if not target.is_file() or not same_artifact(target.read_bytes(),data):raise SystemExit(str(target)+' needs regeneration')
        else:target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    if args.clean_legacy:print(f'Removed {removed} unchanged legacy plugin status icon copies.')
    else:print('WHDLoad, MUSIC and SENSOR plugin-owned status banks are current.')
if __name__=='__main__':main()
