#!/usr/bin/env python3
"""Convert the CC0 Wintery loop WAV into a one-channel sample-based MOD.

Usage: python3 tools/convert_wintery.py /path/to/wintery-loop.wav
The original recording stays outside the source tree. No network is used.
"""
from pathlib import Path
import argparse,audioop,hashlib,math,struct,wave
SOURCE_SHA256='2abe80e3b68c4bd27ce33998ce1daf208564579bf4a1f91435d64d5b4403a670'
CLOCK=3546895;PERIOD=256;RATE=round(CLOCK/PERIOD);BPM=130

def convert(source):
    if hashlib.sha256(source.read_bytes()).hexdigest()!=SOURCE_SHA256:
        raise ValueError('The source is not the reviewed CC0 Wintery loop WAV.')
    with wave.open(str(source),'rb') as w:
        if (w.getnchannels(),w.getsampwidth())!=(2,2):raise ValueError('Expected stereo 16-bit PCM.')
        mono=audioop.tomono(w.readframes(w.getnframes()),2,0.5,0.5)
        pcm,_=audioop.ratecv(mono,2,1,w.getframerate(),RATE,None,1,1)
    pcm=audioop.lin2lin(pcm,2,1)
    # Six patterns at the source's 130 BPM; pad only the sub-millisecond tail.
    length=round(CLOCK/PERIOD*64*6*15/BPM)//2*2
    pcm=pcm[:length].ljust(length,b'\0')
    edges=[round(CLOCK/PERIOD*64*i*15/BPM)//2*2 for i in range(7)]
    samples=[pcm[edges[i]:edges[i+1]] for i in range(6)]
    header=bytearray(b'Wintery Loop'.ljust(20,b'\0'))
    for i in range(31):
        data=samples[i] if i<6 else b''
        header+=(('Emma_MA CC0 %02d'%(i+1)).encode().ljust(22,b'\0') if data else bytes(22))
        header+=struct.pack('>HBBHH',len(data)//2,0,64 if data else 0,0,1)
    header+=bytes([6,0])+bytes(range(6))+bytes(122)+b'M.K.'
    patterns=bytearray(6*1024)
    for i in range(6):
        patterns[i*1024:i*1024+4]=bytes([PERIOD>>8,PERIOD&255,(i+1)<<4,0])
        patterns[i*1024+4:i*1024+8]=bytes([0,0,15,BPM])
        if i==5:patterns[i*1024+63*16+4:i*1024+63*16+8]=bytes([0,0,11,0])
    return bytes(header+patterns)+b''.join(samples)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('--output',type=Path,default=Path(__file__).resolve().parents[1]/'Music/WinteryLoop.mod');a=p.parse_args()
    data=convert(a.source);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
    print('Created',a.output.name,len(data),'bytes; 8-bit mono, native period',PERIOD,'sample-based MOD.')
if __name__=='__main__':main()
