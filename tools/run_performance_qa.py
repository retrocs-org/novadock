#!/usr/bin/env python3
"""Run paired native 0.5+ runtime measurements in a fresh Workbench guest."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import performance_check
CONFIG='''SIDE=BOTTOM
POSITION=CENTER
FONT=WORKBENCH
LABELS=ALWAYS
ITEM_SIZE=NORMAL
THEME=Nord
SHAPE=ROUNDED
TILE_WIDTH=86
TILE_HEIGHT=76
GAP=5
ROWS=2
ROW_COUNTS=AUTO
LEFT_CLICK=LAUNCH
TOOLTIPS=YES
ALWAYS_ON_TOP=NO
RUNNING_APPS=YES
REMOVABLE_MEDIA=NO
AUTO_HIDE=NO
REFRESH_MODE=LOW_MEMORY
DRAWER=System|SYS:|SYS:Disk|ON|AUTO||||NO|YES
LAUNCH=Shell|NewShell|SYS:System/Shell|ON|AUTO
PLUGIN=MusicPlayer|Plugins/MusicPlayer|MUSIC|METER|ON
PLUGIN=CPU|Plugins/Activity|CPU|METER|ON
PLUGIN=RAM|Plugins/Memory|RAM|METER|ON
PLUGIN=Clock|Plugins/Clock|CLOCK|METER|ON
PLUGIN=COLOUR|Plugins/Colour|COLOUR|METER|ON
PLUGIN=QR|Plugins/QR|QR|METER|ON
PLUGIN=MOON|Plugins/Moon|MOON|METER|ON
PLUGIN=BENCH|Plugins/Bench|BENCH|METER|ON
OPTION.CLOCK.SECONDS=NO
OPTION.CLOCK.FORMAT=DIGITAL
'''
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for key in ['baseline','candidate','profile','output']:p.add_argument('--'+key,type=Path,required=True)
 p.add_argument('--timeout-seconds',type=int,default=300,help='Wall-clock allowance per guest; use 1200 for cycle-exact 68000 profiles.');p.add_argument('--compiler',default=os.environ.get('VC','vc'));p.add_argument('--app',type=Path,default=Path('/Applications/Amiga Workbench.app/Contents/MacOS/AmigaWorkbench'));a=p.parse_args()
 if a.timeout_seconds<1:p.error('Timeout must be positive.')
 if a.output.exists():p.error('Use a new output directory; existing QA evidence is never replaced.')
 for folder in [a.baseline,a.candidate]:
  for name in ['NovaDock','NovaDockPrefs','Plugins/MusicPlayer','Plugins/Activity','Plugins/Memory','Plugins/Clock','Plugins/Bench','Plugins/QR','Plugins/Moon','Plugins/Colour']:
   if not (folder/name).is_file():p.error('Compare staged 0.5+ runtime drawers; missing '+str(folder/name))
 if not a.app.is_file() or not a.profile.is_file():p.error('Workbench app and profile must exist.')
 output=a.output.resolve();fixture=output/'inputs';fixture.mkdir(parents=True)
 hashes={}
 for side,folder in [('Before',a.baseline),('After',a.candidate)]:
  destination=fixture/side;destination.mkdir()
  for name in ['NovaDock','NovaDockPrefs','Plugins','Icons','Themes','Music','Catalogs']:
   source=folder/name
   if source.is_dir():shutil.copytree(source,destination/name,ignore=shutil.ignore_patterns('*.o','*.c','*.asm'))
   elif source.is_file():shutil.copy2(source,destination/name)
  (destination/'novadock.cfg').write_text(CONFIG)
  hashes[side]={str(file.relative_to(destination)):hashlib.sha256(file.read_bytes()).hexdigest() for file in sorted(destination.rglob('*')) if file.is_file()}
 (fixture/'ISOLATED-QA-ONLY').write_text('Disposable performance fixture; no user settings or HASS controls.\n')
 (output/'inputs.json').write_text(json.dumps(dict(profile=json.loads(a.profile.read_text()),hashes=hashes),indent=2)+'\n')
 command=[sys.executable,str(ROOT/'scripts/compile-amiga.py'),'--compiler',a.compiler,'--','-O','-Iinclude','-o',str(fixture/'PerfCheck'),'tests/native_05_performance.c','src/control_client.c','src/interaction.c','src/config.c','src/nd_options.c','-lamiga']
 with (output/'compile.log').open('w') as log:subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
 texts=[]
 for side in ['Before','After']:
  test=fixture/(side.lower()+'.json');test.write_text(json.dumps(dict(name='NovaDock performance '+side,application='QATests:PerfCheck',arguments=[side],timeoutSeconds=a.timeout_seconds,expectedReturnCode=0)))
  native=output/('native-'+side.lower())
  subprocess.run([str(a.app),'--config',str(a.profile.resolve()),'--mount','QATests:'+str(fixture),'--test',str(test),'--output',str(native)],check=True)
  logs=list((native/'profile').glob('Installations/*/CPUs/*/AutoRun/launch.log'))
  if len(logs)!=1:raise RuntimeError('Expected one native performance log')
  texts.append(logs[0].read_text())
 combined='\n'.join(texts);(output/'performance.log').write_text(combined)
 results=performance_check.compare(combined);(output/'performance.json').write_text(json.dumps(results,indent=2)+'\n')
 for r in results:print('%s %s: %d -> %d'%('PASS' if r['passed'] else 'REGRESSION',r['metric'],r['before'],r['after']))
 return 0 if all(r['passed'] for r in results) else 1
if __name__=='__main__':raise SystemExit(main())
