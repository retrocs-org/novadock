#!/usr/bin/env python3
"""Tilt icon set: exact regeneration, valid ILBM, shared palette, outline and table."""
import importlib.util
import struct
import sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
from artifact_compare import same_artifact
spec=importlib.util.spec_from_file_location('tilt_art',root/'tools/generate_tilt_icons.py')
art=importlib.util.module_from_spec(spec);spec.loader.exec_module(art)
table=(root/'include/nd_collection_icons.h').read_text()
assert len(art.PALETTE)==16 and len(set(art.PALETTE))==16
seen=set()
for name in art.NAMES:
    pixels=art.artwork(name);data=(root/'Icons'/f'{name}.iff').read_bytes()
    assert data==art.ilbm(pixels),name+' artwork needs regenerating'
    assert same_artifact((root/'Icons'/f'{name}.png').read_bytes(),art.png(pixels)),name
    assert '{"%s","Icons/%s.iff"}'%(name,name) in table,name+' missing from nd_collection_icons.h'
    chunks={};at=12
    while at<len(data):
        kind=data[at:at+4];n=struct.unpack_from('>I',data,at+4)[0];chunks[kind]=data[at+8:at+8+n];at+=8+n+(n&1)
    assert struct.unpack_from('>HH',chunks[b'BMHD'])==(32,32) and chunks[b'BMHD'][8:11]==bytes((4,2,0))
    assert chunks[b'CMAP']==bytes(v for rgb in art.PALETTE for v in rgb)
    body=chunks[b'BODY'];assert len(body)==512
    decoded=[[sum(((body[y*16+p*4+x//8]>>(7-x%8))&1)<<p for p in range(4)) for x in range(32)] for y in range(32)]
    assert decoded==pixels,name
    # Fits with its outline: nothing on the outer edge but the outline itself.
    for i in range(32):
        assert all(v in (0,1) for v in (pixels[0][i],pixels[31][i],pixels[i][0],pixels[i][31])),name
    # Every coloured pixel next to transparency is outline (the bold edge).
    for y in range(32):
        for x in range(32):
            if pixels[y][x] not in (0,1):
                for dx,dy in ((1,0),(-1,0),(0,1),(0,-1)):
                    assert pixels[y+dy][x+dx]!=0,(name,x,y)
    fingerprint=bytes(v for row in pixels for v in row)
    assert fingerprint not in seen,name+' duplicates another icon'
    seen.add(fingerprint)
    assert sum(v!=0 for v in fingerprint)>250,name
assert len(seen)==40
names=[l.split('"')[1] for l in table.splitlines() if l.startswith(' {"')]
assert len(names)==len(set(names)), 'duplicate icon names in the table'
assert '#define ND_COLLECTION_ICON_COUNT %d'%len(names) in table
print('Forty Tilt icons: exact regeneration, shared palette, outlined edges, distinct artwork and gallery entries passed.')
