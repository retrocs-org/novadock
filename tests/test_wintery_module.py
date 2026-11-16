#!/usr/bin/env python3
"""Check the bundled module's ProTracker layout, timing and source credit."""
from pathlib import Path
import hashlib,struct
root=Path(__file__).resolve().parents[1];data=(root/'Music/WinteryLoop.mod').read_bytes()
assert data[:12]==b'Wintery Loop' and data[1080:1084]==b'M.K.' and data[950]==6
lengths=[struct.unpack_from('>H',data,20+i*30+22)[0]*2 for i in range(31)]
assert all(2<=n<=131070 for n in lengths[:6]) and not any(lengths[6:])
assert len(data)==1084+6*1024+sum(lengths)
for i in range(6):
 p=data[1084+i*1024:1084+(i+1)*1024]
 assert p[:8]==bytes([1,0,(i+1)*16,0,0,0,15,130])
 assert data[952+i]==i
assert data[1084+5*1024+63*16+4:1084+5*1024+63*16+8]==bytes([0,0,11,0])
assert abs(sum(lengths)/(3546895/256)-44.307692)<0.001
credit=(root/'Music/WinteryLoop.LICENSE').read_text();assert 'Emma_MA' in credit and 'CC0' in credit and 'https://opengameart.org/content/wintery-loop' in credit
print('Wintery loop: valid six-pattern sample-based MOD, 130 BPM looping and source/CC0 credits passed.')
