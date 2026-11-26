#!/usr/bin/env python3
"""Validate every packaged state at Tiny and Normal dock icon sizes."""
import argparse,re,struct
from pathlib import Path

def check_bank(path):
 path=Path(path);data=path.read_bytes()
 def require(ok,message):
  if not ok:raise ValueError(str(path)+': '+message)
 require(len(data)>=56 and data[:5]==b'NDAR\0' and data[5] in (1,2),'invalid artwork header')
 header=72 if data[5]==2 else 56;n=struct.unpack_from('>H',data,6)[0]
 require(0<n<=256 and header+n*32<=len(data)<=262144,'invalid frame table')
 if header==72:require(data[56]==0 and all(c<=5 for c in data[56:72]),'invalid palette roles')
 frames=[];seen=set();envelopes={}
 for i in range(n):
  name,state,flags,count,offset=struct.unpack_from('>24sBBHI',data,header+i*32)
  require(b'\0' in name,'unterminated frame name');name=name.split(b'\0',1)[0]
  require(re.fullmatch(rb'[A-Za-z0-9_-]{1,23}',name) is not None,'invalid frame name')
  require((name,state) not in seen,'duplicate frame state');seen.add((name,state))
  require(flags==0 if header==56 else flags&~15==0 and (not flags&8 or flags&1),'invalid presentation flags')
  require(0<count<=1024 and header+n*32<=offset<=len(data) and offset+count*5<=len(data),'invalid frame runs')
  runs=[tuple(data[j:j+5]) for j in range(offset,offset+count*5,5)]
  require(all(w and h and x+w<=32 and y+h<=32 and 0<c<16 for x,y,w,h,c in runs),'invalid frame rectangle')
  l=min(r[0] for r in runs);t=min(r[1] for r in runs);r=max(r[0]+r[2] for r in runs);b=max(r[1]+r[3] for r in runs)
  old=envelopes.get(name,(32,32,0,0));envelopes[name]=(min(l,old[0]),min(t,old[1]),max(r,old[2]),max(b,old[3]))
  frames.append((name,state,flags,runs))
 cases=0
 for name,state,flags,runs in frames:
  for label,tw,th,tiny in [('Normal',86,76,False),('Tiny',43,38,True),('minimum Tiny',34,30,True),('compact Normal',55,46,False)]:
   reserve=(15 if tiny else 26 if th<64 else 50) if flags&8 else (15 if tiny or th<64 else 35) if flags&1 else 4 if tiny else 24
   size=min(tw-(12 if header==56 or flags&2 else 4 if tiny else 16),th-reserve,24 if flags&8 else 38 if flags&1 else 48)
   require(size>0,'no icon area for '+label)
   l,t,r,b=(0,0,32,32) if flags&1 else envelopes[name];w=r-l;h=b-t;span=max(w,h);visible=0
   for x,y,rw,rh,c in runs:
    xx=2*(x-l)+span-w;yy=2*(y-t)+span-h
    x1=xx*size//(2*span);y1=yy*size//(2*span);x2=(xx+2*rw)*size//(2*span);y2=(yy+2*rh)*size//(2*span)
    require(0<=x1<=x2<=size and 0<=y1<=y2<=size,'scaled art exceeds '+label+' bounds')
    visible+=(x2-x1)*(y2-y1)
   require(visible>0,name.decode()+' state '+str(state)+' disappears in '+label)
   cases+=1
 return n,cases

def check_project(root):
 root=Path(root);banks=set();problems=[]
 for manifest in root.rglob('*.ndplugin'):
  fields=dict(line.split('=',1) for line in manifest.read_text().splitlines() if '=' in line and not line.lstrip().startswith(';'))
  if 'artwork' in fields.get('NOVADOCK_FEATURES','').split(','):
   bank=manifest.with_suffix('.assets')/'Artwork.ndart'
   project_bank=root/(fields.get('NOVADOCK_PROGRAM',manifest.stem)+'.assets')/'Artwork.ndart'
   if not bank.is_file() and project_bank.is_file():bank=project_bank
   if not bank.is_file():problems.append(str(manifest)+': artwork requires '+str(bank))
   else:banks.add(bank)
 banks.update(root.rglob('*.assets/Artwork.ndart'));frames=cases=0
 for bank in sorted(banks):
  try:n,c=check_bank(bank);frames+=n;cases+=c
  except (ValueError,OSError) as error:problems.append(str(error))
 return problems,len(banks),frames,cases

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('project',type=Path,nargs='?',default=Path('.'));a=p.parse_args()
 problems,banks,frames,cases=check_project(a.project)
 for message in problems:print('FAIL',message)
 if problems:return 1
 print(f'PASS artwork size policy: {banks} banks, {frames} states, {cases} Tiny/Normal size checks.')
 print('Visually verify every state, including sad/error/offline, in both sizes and magnified before release.')
 return 0
if __name__=='__main__':raise SystemExit(main())
