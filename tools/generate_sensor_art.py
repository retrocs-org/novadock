#!/usr/bin/env python3
"""Portable indexed sensor artwork, using the existing plugin bank palette."""
KINDS=('Sensor','SensorCPU','SensorRAM','SensorFPGA','SensorBattery','SensorFan','SensorPower','SensorTemp')
def sensor_frames():
 frames=[]
 for kind in KINDS:
  p=[[0]*32 for _ in range(32)]
  def box(c,x,y,w,h):
   for yy in range(y,y+h):
    for xx in range(x,x+w):p[yy][xx]=c
  if kind in ('SensorCPU','SensorFPGA','Sensor'):
   for i in range(7,27,5):
    box(2,i,2,2,4);box(2,i,26,2,4);box(2,2,i,4,2);box(2,26,i,4,2)
   box(1,5,5,22,22);box(2,6,6,20,20);box(3,7,7,18,2);box(1,9,11,14,12)
   if kind=='SensorFPGA':
    for y in (12,16,20):
     for x in (10,14,18):box(4,x,y,2,2)
   else:box(4,11,13,10,3);box(5,11,18,7,2)
  elif kind=='SensorRAM':
   box(1,2,6,28,20);box(5,3,7,26,17);box(3,4,8,24,2)
   for x in (5,13,21):box(1,x,12,6,9)
   for x in range(4,29,4):box(6,x,25,2,4)
  elif kind=='SensorBattery':
   box(1,11,2,10,3);box(1,6,5,20,25);box(2,7,6,18,23);box(3,8,7,16,2);box(1,9,11,14,15);box(5,10,19,12,6);box(4,14,13,4,4)
  elif kind=='SensorPower':
   box(1,3,3,26,26);box(2,4,4,24,24);box(3,5,5,22,2)
   for x,y,w,h in ((16,8,6,3),(13,11,7,3),(10,14,8,4),(15,18,6,3),(13,21,5,3)):box(6,x,y,w,h)
  elif kind=='SensorFan':
   box(1,3,3,26,26);box(2,4,4,24,24);box(1,6,6,20,20)
   box(4,11,7,5,8);box(4,18,11,7,5);box(4,16,18,5,7);box(4,7,16,8,5);box(3,13,13,6,6)
  else:
   box(1,12,2,8,20);box(2,13,3,6,18);box(3,14,4,2,15);box(1,9,20,14,10);box(7,11,21,10,7);box(7,15,10,2,14)
   for y in (6,11,16):box(2,23,y,5,2)
  frames.append(dict(name=kind,state=0,flags=9,pixels=p))
 return frames
