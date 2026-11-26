#!/usr/bin/env python3
"""Verify Tiny metric artwork in a separate offline FS-UAE guest."""
import argparse,json,shlex,shutil,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--no-build',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--profile',choices=('a4000-rtg','a500-ecs'),default='a4000-rtg');p.add_argument('--visible',action='store_true');p.add_argument('--resources',type=Path);a=p.parse_args()
 out=a.output.resolve() if a.output else Path(tempfile.mkdtemp(prefix='nd-tiny-native-',dir='/private/tmp'));out.mkdir(parents=True,exist_ok=True)
 if (out/'test.json').exists():p.error('Use a new output directory')
 import release
 env=release.credentials.build_environment(release.config(release.default_config()))
 if not a.no_build:
  with (out/'build.log').open('w') as log:subprocess.run(['sh','scripts/build-amiga.sh'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 plan=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True);cmd=shlex.split(next(x for x in plan.splitlines() if 'scripts/compile-amiga.py' in x));cmd=cmd[:cmd.index('--')+1]+['-O','-Iinclude','-o',str(out/'TinyQA'),'tests/native_tiny_metrics.c','src/control_client.c','src/config.c','src/nd_options.c','src/prefs_layout.c','-lamiga']
 with (out/'compile.log').open('w') as log:subprocess.run(cmd,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 dock=out/'Dock';(dock/'Plugins').mkdir(parents=True)
 for n in ('NovaDock','NovaDockPrefs','Plugins/Activity','Plugins/Memory','Plugins/Clock','Plugins/Network','Plugins/Network.ndplugin'):shutil.copy2(ROOT/n,dock/n)
 shutil.copytree(ROOT/'Themes/Nord',dock/'Themes/Nord');shutil.copytree(ROOT/'Icons',dock/'Icons')
 (out/'ISOLATED-QA-ONLY').touch();(out/'test.json').write_text(json.dumps({'application':'QATests:TinyQA','expectedReturnCode':0}))
 cmd=[sys.executable,str(ROOT/'tools/run_native_fixture.py'),str(out),'--profile',a.profile,'--timeout','180']
 if not a.visible:cmd+=['--hidden']
 if a.resources:cmd+=['--resources',str(a.resources.resolve())]
 result=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True);(out/'run.log').write_text(result.stdout+result.stderr);print(result.stdout+result.stderr,end='')
 bases=[x for x in result.stdout.splitlines() if x.startswith('/private/tmp/nd-native-latest-')]
 if bases:
  base=Path(bases[0]);shutil.copy2(base/'system-source.json',out/'system-source.json');
  if (base/'AutoRun/launch.log').exists():shutil.copy2(base/'AutoRun/launch.log',out/'native.log')
  for n in ('tiny-icons.ppm','tiny-hover.ppm','tiny-appearance.ppm','preview-normal-artwork.ppm','preview-tiny-artwork.ppm','preview-normal-solid.ppm','preview-tiny-solid.ppm'):
   if (base/'QATests'/n).exists():shutil.copy2(base/'QATests'/n,out/n)
 return result.returncode
if __name__=='__main__':raise SystemExit(main())
