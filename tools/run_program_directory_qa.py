#!/usr/bin/env python3
"""Verify actual plugin startup in portable and split Install layouts in FS-UAE."""
import argparse,json,shlex,shutil,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--output',type=Path);p.add_argument('--no-build',action='store_true')
 p.add_argument('--resources',type=Path);p.add_argument('--visible',action='store_true')
 a=p.parse_args()
 out=a.output.resolve() if a.output else Path(tempfile.mkdtemp(prefix='nd-directory-qa-',dir='/private/tmp'))
 out.mkdir(parents=True,exist_ok=True)
 if (out/'test.json').exists():p.error('Use a new output directory')
 import release
 env=release.credentials.build_environment(release.config(release.default_config()))
 if not a.no_build:
  with (out/'build.log').open('w') as log:subprocess.run(['sh','scripts/build-amiga.sh'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 plan=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True)
 cmd=shlex.split(next(l for l in plan.splitlines() if 'scripts/compile-amiga.py' in l))
 cmd=cmd[:cmd.index('--')+1]+['-O','-Iinclude','-o',str(out/'LookupQA'),'tests/native_program_directory.c','src/control_client.c','src/config.c','src/nd_options.c','-lamiga']
 with (out/'compile.log').open('w') as log:subprocess.run(cmd,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 dock=out/'Dock';(dock/'Plugins').mkdir(parents=True)
 for name in ('NovaDock','NovaDock.info','Plugins/Activity','Plugins/Memory','Plugins/Clock','Plugins/ArchiveExtractor','Plugins/Pet'):
  shutil.copy2(ROOT/name,dock/name)
  if (ROOT/(name+'.assets')).is_dir():shutil.copytree(ROOT/(name+'.assets'),dock/(name+'.assets'))
 for name in ('Icons','Themes/Nord'):shutil.copytree(ROOT/name,dock/name)
 for name in ('Bin','Metadata'):
  base=out/name;(base/'Plugins').mkdir(parents=True)
  for program in ('NovaDock','NovaDock.info'):shutil.copy2(dock/program,base/program)
 (out/'Metadata/Plugins/Stale.ndplugin').write_text('NAME=Stale metadata\n')
 (out/'Metadata/Plugins/Stale.info').write_bytes(b'not a program')
 (out/'Metadata/Plugins/Stale.assets').mkdir()
 (out/'ISOLATED-QA-ONLY').touch()
 (out/'test.json').write_text(json.dumps({'application':'QATests:LookupQA','expectedReturnCode':0}))
 cmd=[sys.executable,str(ROOT/'tools/run_native_fixture.py'),str(out),'--timeout','180']
 if not a.visible:cmd+=['--hidden']
 if a.resources:cmd+=['--resources',str(a.resources.resolve())]
 result=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True)
 (out/'run.log').write_text(result.stdout+result.stderr);print(result.stdout+result.stderr,end='')
 bases=[x for x in result.stdout.splitlines() if x.startswith('/private/tmp/nd-native-latest-')]
 if bases:
  base=Path(bases[0]);shutil.copy2(base/'system-source.json',out/'system-source.json')
  if (base/'AutoRun/launch.log').exists():shutil.copy2(base/'AutoRun/launch.log',out/'native.log')
 return result.returncode
if __name__=='__main__':raise SystemExit(main())
