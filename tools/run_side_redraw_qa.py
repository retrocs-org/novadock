#!/usr/bin/env python3
"""Compare all-side hover frames across deliberately slow desktop repaint."""
import argparse,json,shlex,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--resources',type=Path);p.add_argument('--mode',choices=('TOP','BACKDROP'),default='BACKDROP');p.add_argument('--alpha',type=int,default=100);p.add_argument('--binary',type=Path);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 import release
 env=release.credentials.build_environment(release.config(release.default_config()));plan=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True);cmd=shlex.split(next(x for x in plan.splitlines() if 'scripts/compile-amiga.py' in x));cmd=cmd[:cmd.index('--')+1]
 with (out/'compile.log').open('w') as log:
  subprocess.run(cmd+['-O','-Iinclude','-o',str(out/'SlowDesktop'),'tests/native_slow_desktop.c','-lamiga'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
  subprocess.run(cmd+['-O','-Iinclude','-o',str(out/'SideQA'),'tests/native_side_redraw.c','src/control_client.c','src/config.c','src/nd_options.c','src/prefs_layout.c','-lamiga'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 dock=out/'Dock';(dock/'Icons').mkdir(parents=True);(dock/'Plugins').mkdir();shutil.copy2(ROOT/'Plugins/Clock',dock/'Plugins/Clock');shutil.copy2(a.binary or ROOT/'NovaDock',dock/'NovaDock');shutil.copytree(ROOT/'Themes/Nord',dock/'Themes/Nord')
 for name in ('Folder.iff','Trash.iff','Settings.iff'):shutil.copy2(ROOT/'Icons'/name,dock/'Icons'/name)
 for source,name in [('Plugins/Clock.info','QAClock.info'),('NovaDock.info','QAShell.info'),('Plugins/MusicPlayer.info','QAMusic.info')]:shutil.copy2(ROOT/source,dock/'Icons'/name)
 (out/'ISOLATED-QA-ONLY').touch();(out/'test.json').write_text(json.dumps({'application':f'QATests:SideQA {a.mode} {a.alpha}','expectedReturnCode':0}))
 cmd=[sys.executable,'tools/run_native_fixture.py',str(out),'--timeout','300','--hidden']
 if a.resources:cmd+=['--resources',str(a.resources)]
 r=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True);(out/'run.log').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='');bases=[x for x in r.stdout.splitlines() if x.startswith('/private/tmp/nd-native-latest-')]
 if bases:
  base=Path(bases[0]);shutil.copy2(base/'AutoRun/launch.log',out/'native.log')
  for pic in (base/'QATests').glob('*.ppm'):shutil.copy2(pic,out/pic.name)
 return r.returncode
if __name__=='__main__':raise SystemExit(main())
