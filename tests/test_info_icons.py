#!/usr/bin/env python3
"""Validate the shipped classic Workbench DiskObjects."""
from pathlib import Path
import struct

root = Path(__file__).resolve().parents[1]
payloads = []
for name, width, height, length in (
        ('NovaDock.info', 54, 22, 450),
        ('NovaDockPrefs.info', 54, 22, 450),
        ('Plugins/MusicPlayer.info', 48, 38, 554),
        ('Plugins/RunningApps.info', 48, 38, 554),
        ('Plugins/AmIRC.info', 48, 32, 482)):
    data = (root / name).read_bytes()
    assert len(data) == length, (name, len(data))
    assert struct.unpack_from('>HH', data, 0) == (0xE310, 1)
    assert struct.unpack_from('>hhhh', data, 8) == (0, 0, width, height)
    assert data[48] == 3  # WBTOOL
    assert struct.unpack_from('>I', data, 74)[0] >= 16384
    assert struct.unpack_from('>hhhhh', data, 78) == (0, 0, width, height, 2)
    assert b'FORM' not in data  # no newer colour-icon extension required
    payloads.append(data)
assert len(set(payloads)) == 5
# Release drawer icon: DiskObject, OldDrawerData, one image, then the OS 2.x
# drawer extension, and nothing else.
import subprocess, sys, tempfile
with tempfile.TemporaryDirectory() as tmp:
    path = Path(tmp) / 'NovaDock.info'
    subprocess.run([sys.executable, str(root / 'tools/generate_info_icons.py'),
                    '--drawer-icon', str(path)], check=True, capture_output=True)
    data = path.read_bytes()
assert struct.unpack_from('>HH', data, 0) == (0xE310, 1)
assert data[48] == 2                                    # WBDRAWER
assert struct.unpack_from('>I', data, 66)[0] != 0       # has DrawerData
assert struct.unpack_from('>II', data, 50) == (0, 0)    # no tool, no tool types
left, top, width, height = struct.unpack_from('>hhhh', data, 78)
assert width > 0 and height > 0 and struct.unpack_from('>H', data, 78 + 46)[0] == 1
def image_bytes(data):
    offset=78+(56 if struct.unpack_from('>I',data,66)[0] else 0)
    start=offset
    for pointer in (22,26):
        if struct.unpack_from('>I',data,pointer)[0]:
            w,h,depth=struct.unpack_from('>hhh',data,offset+4)
            assert (w,h,depth)==(54,22,2)
            offset+=20+((w+15)//16)*2*h*depth
    return data[start:offset],offset

images,end=image_bytes(data)
assert len(data)==end+6
assert struct.unpack_from('>I',data,44)[0] & 0xff == 1
assert struct.unpack_from('>IH',data,end)==(1,1) # DDFLAGS_SHOWICONS, DDVM_BYICON
# The generic installed settings-drawer asset still inherits its parent.
assert struct.unpack_from('>IH',(root/'Icons/SoftwarePackage.info').read_bytes(),-6)==(0,0)
assert struct.unpack_from('>II',data,58)==(0x80000000,0x80000000)
assert images==image_bytes((root/'Icons/SoftwarePackage.info').read_bytes())[0]
assert struct.unpack_from('>I',data,26)[0] # supplied selected image retained
installer=(root/'Install-NovaDock.info').read_bytes()
assert installer[48]==4 and struct.unpack_from('>I',installer,66)[0]==0
assert image_bytes(installer)[0]==image_bytes((root/'Icons/Install.info').read_bytes())[0]
assert b'StartInstaller\0' in installer and b'APPNAME=NovaDock\0' in installer
assert b'SYS:System/Installer\0' not in installer
# The manual opens with MultiView.
guide = (root / 'NovaDock.guide.info').read_bytes()
assert struct.unpack_from('>HH', guide, 0) == (0xE310, 1) and guide[48] == 4
assert b'SYS:Utilities/MultiView\0' in guide
print('NovaDock, Preferences, MusicPlayer, Running Apps, AmIRC, manual and release drawer icons passed.')
