#!/usr/bin/env python3
"""Check deterministic artwork and independently decode generated ILBM planes."""
import importlib.util
import struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('collection_art',root/'tools/generate_collection_icons.py')
art=importlib.util.module_from_spec(spec);spec.loader.exec_module(art)
seen=set()
for name in art.NAMES:
    pixels=art.artwork(name);data=(root/'Icons'/f'{name}.iff').read_bytes()
    assert data==art.ilbm(pixels),name+' artwork needs regenerating'
    assert (root/'Icons'/f'{name}.png').read_bytes()==art.png(pixels)
    assert data[:4]==b'FORM' and struct.unpack_from('>I',data,4)[0]==len(data)-8 and data[8:12]==b'ILBM'
    chunks={};at=12
    while at<len(data):
        kind=data[at:at+4];n=struct.unpack_from('>I',data,at+4)[0];chunks[kind]=data[at+8:at+8+n];at+=8+n+(n&1)
    assert struct.unpack_from('>HH',chunks[b'BMHD'])==(32,32)
    assert chunks[b'BMHD'][8:11]==bytes((4,2,0))
    body=chunks[b'BODY'];assert len(body)==512
    decoded=[]
    for y in range(32):
        row=[]
        for x in range(32):
            row.append(sum(((body[y*16+plane*4+x//8]>>(7-x%8))&1)<<plane for plane in range(4)))
        decoded.append(row)
    assert decoded==pixels
    fingerprint=bytes(v for row in decoded for v in row)
    assert fingerprint not in seen,name+' duplicates another icon'
    seen.add(fingerprint)
    assert sum(v!=0 for v in fingerprint)>100
    assert all(0<=v<16 for v in fingerprint)
assert len(seen)==70
print('Seventy distinct shared collection icons: exact regeneration, transparent ILBM planes and PNG assets passed.')
