#!/usr/bin/env python3
"""Verify continuous grip movement, bounds and persistence in isolated FS-UAE."""
import argparse,json,shlex,shutil,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--no-build',action='store_true');p.add_argument('--resources',type=Path);p.add_argument('--profile',choices=('a4000-rtg','a500-ecs'),default='a4000-rtg');p.add_argument('--backdrop',action='store_true');p.add_argument('--visible',action='store_true');a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 import release,check_plugin_qa,run_plugin_qa
 env=release.credentials.build_environment(release.config(release.default_config()))
 if not a.no_build:
  with (out/'build.log').open('w') as log:subprocess.run(['sh','scripts/build-amiga.sh'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 plan=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True)
 cmd=shlex.split(next(l for l in plan.splitlines() if 'scripts/compile-amiga.py' in l));cmd=cmd[:cmd.index('--')+1]+['-O','-Iinclude','-o',str(out/'GripQA'),'tests/native_continuous_grip.c','src/control_client.c','src/config.c','src/nd_options.c','src/layout.c','-lamiga']
 with (out/'compile.log').open('w') as log:subprocess.run(cmd,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 run_plugin_qa.copy_runtime(ROOT,out/'Dock',check_plugin_qa.load())
 (out/'ISOLATED-QA-ONLY').touch();(out/'test.json').write_text(json.dumps({'application':'QATests:GripQA '+('BACKDROP' if a.backdrop else 'TOP')+' ARTWORK','expectedReturnCode':0}))
 cmd=[sys.executable,'tools/run_native_fixture.py',str(out),'--profile',a.profile,'--timeout','500']
 if not a.visible:cmd+=['--hidden']
 if a.resources:cmd+=['--resources',str(a.resources.resolve())]
 result=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True);(out/'run.log').write_text(result.stdout+result.stderr);print(result.stdout+result.stderr,end='')
 bases=[s for s in result.stdout.splitlines() if s.startswith('/private/tmp/nd-native-latest-')]
 if bases:
  base=Path(bases[0]);shutil.copy2(base/'system-source.json',out/'system-source.json')
  if (base/'AutoRun/launch.log').exists():shutil.copy2(base/'AutoRun/launch.log',out/'native.log')
  for name in ('grip-failure.ppm','failed.cfg'):
   if (base/'QATests'/name).exists():shutil.copy2(base/'QATests'/name,out/name)
 return result.returncode
if __name__=='__main__':raise SystemExit(main())
