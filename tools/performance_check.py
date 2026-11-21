#!/usr/bin/env python3
"""Compare paired native PERF rows; absolute and relative tolerances cover timer noise."""
import argparse,json
from pathlib import Path
# Allow a small fixed cost on top of the relative budget. Units are 50 Hz ticks.
BUDGETS={'startup_ticks':(1.35,50),'prefs_general_ticks':(1.35,25),'prefs_plugins_cold_ticks':(1.6,100),'prefs_plugins_warm_ticks':(1.1,25),'prefs_qr_ticks':(1.35,25),'idle_wakes':(1.0,15),'idle_paints':(1.0,2),'used_bytes':(1.0,524288)}
def compare(text):
 rows={}
 for line in text.splitlines():
  if not line.startswith('PERF,'):continue
  fields=line.strip().split(',')
  if len(fields)!=4 or fields[1] not in ('Before','After') or fields[2] not in BUDGETS:raise ValueError('Invalid performance row')
  key=tuple(fields[1:3])
  if key in rows:raise ValueError('Duplicate performance row')
  value=int(fields[3])
  if value<0:raise ValueError('Negative performance result')
  rows[key]=value
 result=[]
 for metric,(ratio,slack) in BUDGETS.items():
  before=rows['Before',metric];after=rows['After',metric];limit=before*ratio+slack
  result.append(dict(metric=metric,before=before,after=after,limit=limit,passed=after<=limit))
 return result
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path);p.add_argument('--output',type=Path);a=p.parse_args()
 try:results=compare(a.log.read_text())
 except (ValueError,KeyError) as e:p.error('Incomplete or invalid native performance report: '+str(e))
 if a.output:a.output.write_text(json.dumps(results,indent=2)+'\n')
 for r in results:print(('%s %s: %d -> %d (budget %.0f)')%('PASS' if r['passed'] else 'REGRESSION',r['metric'],r['before'],r['after'],r['limit']))
 raise SystemExit(0 if all(r['passed'] for r in results) else 1)
