"""Independent, offline macOS Vision check of the production ILBM exporter."""
from pathlib import Path
import os,shutil,struct,subprocess,sys,zlib

def verify(path):
 if sys.platform!='darwin' or not shutil.which('swift'):
  print('Independent Vision QR decode unavailable on this host.');return
 path=Path(path);b=path.read_bytes();i=12;chunks={}
 while i<len(b):
  n=int.from_bytes(b[i+4:i+8],'big');chunks[b[i:i+4]]=b[i+8:i+8+n];i+=8+n+(n&1)
 w,h=struct.unpack('>HH',chunks[b'BMHD'][:4]);stride=((w+15)//16)*2;scale=4;rows=[]
 for y in range(h):
  row=b'\0'+b''.join(bytes([0 if chunks[b'BODY'][y*stride+x//8]&(128>>(x%8)) else 255])*scale for x in range(w))
  rows += [row]*scale
 def chunk(t,v):return struct.pack('>I',len(v))+t+v+struct.pack('>I',zlib.crc32(t+v))
 png=path.with_suffix('.png');png.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w*scale,h*scale,8,0,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(rows)))+chunk(b'IEND',b''))
 env=dict(os.environ,CLANG_MODULE_CACHE_PATH=str(path.parent/'swift-cache'))
 subprocess.run(['swift',str(Path(__file__).with_name('qr_decode.swift')),str(png)],env=env,check=True,timeout=60)
