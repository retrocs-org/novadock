#!/usr/bin/env python3
"""Run volume caching and slow-filesystem responsiveness in an isolated guest."""
import argparse,json,shlex,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--resources',type=Path);p.add_argument('--popup-only',action='store_true');a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 import release,build_provenance
 env=release.credentials.build_environment(release.config(release.default_config()));plan=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True);cmd=shlex.split(next(x for x in plan.splitlines() if 'scripts/compile-amiga.py' in x));cmd=cmd[:cmd.index('--')+1]
 with (out/'compile.log').open('w') as log:subprocess.run(cmd+['-O','-Iinclude','-Iplugin_src/common','-o',str(out/'VolumeQA'),'tests/native_volume_worker.c','src/nd_volume_worker.c','src/nd_volume_query.c','src/nd_volume.c','plugin_src/common/nd_task.c','src/interaction.c','src/control_client.c','src/config.c','src/nd_options.c','src/nd_async_result.c','src/nd_folder.c','-lamiga'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 programs=build_provenance.binaries();dock=out/'Dock';dock.mkdir()
 for name in programs:
  target=dock/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
 for folder in ('Icons','Themes/Nord'):shutil.copytree(ROOT/folder,dock/folder)
 (out/'ISOLATED-QA-ONLY').touch();(out/'test.json').write_text(json.dumps({'application':'QATests:VolumeQA'+(' POPUP' if a.popup_only else ''),'expectedReturnCode':0}))
 cmd=[sys.executable,'tools/run_native_fixture.py',str(out),'--timeout','120','--hidden']
 if a.resources:cmd+=['--resources',str(a.resources)]
 r=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True);run=r.stdout+r.stderr;(out/'run.log').write_text(run);bases=[x for x in run.splitlines() if x.startswith('/private/tmp/nd-native-latest-')]
 if bases:shutil.copy2(Path(bases[0])/'AutoRun/launch.log',out/'native.log')
 passed=r.returncode==0 and (out/'native.log').exists() and ('PASS loading folder cancels immediately' if a.popup_only else 'PASS dock diagnostics and Quit stay responsive') in (out/'native.log').read_text()
 (out/'report.json').write_text(json.dumps({'status':'passed' if passed else 'failed','tested_binaries':programs,'scope':'Real delayed DOS handler, cached readings, independent worker close, and dock control/Quit responsiveness'},indent=2)+'\n');print(run[-2000:]);return 0 if passed else 1
if __name__=='__main__':raise SystemExit(main())
