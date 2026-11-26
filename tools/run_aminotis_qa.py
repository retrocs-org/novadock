#!/usr/bin/env python3
"""Offline native test of General Preferences and real AmiNotis SDK forwarding."""
import argparse,hashlib,json,shlex,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--resources',type=Path);p.add_argument('--service-drawer',type=Path,help='Optional real AmiNotis runtime; copied into the isolated guest');p.add_argument('--profile',choices=('a4000-rtg','a500-ecs'),default='a4000-rtg');a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 import release,build_provenance
 env=release.credentials.build_environment(release.config(release.default_config()));plan=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True);cmd=shlex.split(next(x for x in plan.splitlines() if 'scripts/compile-amiga.py' in x));cmd=cmd[:cmd.index('--')+1]
 with (out/'compile.log').open('w') as log:
  for name,args in [('NotisSink',['tests/native_aminotis_sink.c','-lamiga']),('NoticeProducer',['tests/native_notice_sender.c','plugin_src/common/nd_client.c','plugin_src/common/nd_option_runtime.c']),('NotisObserver',['tests/native_aminotis_observer.c','src/aminotis_sdk/aminotis.c','-lamiga']),('NotisQA',['tests/native_aminotis.c','src/control_client.c','src/config.c','src/nd_options.c','-lamiga'])]:
   subprocess.run(cmd+['-O','-Iinclude','-Iplugin_src/common','-o',str(out/name)]+args,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 dock=out/'Dock';dock.mkdir()
 programs=build_provenance.binaries()
 for name in programs:
  target=dock/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
 for folder in ('Icons','Themes/Nord'):shutil.copytree(ROOT/folder,dock/folder)
 shutil.copy2(out/'NoticeProducer',dock/'Plugins/NoticeProducer')
 service_hashes={}
 if a.service_drawer:
  service=a.service_drawer.resolve()
  for name in ('AmiNotisService','AmiNotisPresenter','AmiNotisHistoryWriter'):
   data=(service/name).read_bytes();assert data[:4]==bytes.fromhex('000003f3'),name
   shutil.copy2(service/name,out/name);service_hashes[name]=hashlib.sha256(data).hexdigest()
  shutil.copy2(service/'AmiNotisService',out/'NotisSink')
 (out/'ISOLATED-QA-ONLY').touch();(out/'test.json').write_text(json.dumps({'application':'QATests:NotisQA'+(' OBSERVER' if a.service_drawer else ''),'expectedReturnCode':0}))
 cmd=[sys.executable,'tools/run_native_fixture.py',str(out),'--profile',a.profile,'--timeout','180','--hidden']
 if a.resources:cmd+=['--resources',str(a.resources)]
 result=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True);(out/'run.log').write_text(result.stdout+result.stderr);print(result.stdout+result.stderr,end='');bases=[x for x in result.stdout.splitlines() if x.startswith('/private/tmp/nd-native-latest-')]
 if bases:
  base=Path(bases[0]);shutil.copy2(base/'AutoRun/launch.log',out/'native.log')
  for name in ('notis-received.log',):
   if (base/'QATests'/name).exists():shutil.copy2(base/'QATests'/name,out/name)
 passed=result.returncode==0 and (out/'native.log').exists() and 'PASS General checkbox saves' in (out/'native.log').read_text()
 (out/'report.json').write_text(json.dumps({'status':'passed' if passed else 'failed','profile':a.profile,'tested_binaries':programs,'service_binaries':service_hashes,'scope':('real AmiNotis service' if service_hashes else 'protocol-v2 test receiver')+'; local collection, forwarding, coalescing, restart and preferences'},indent=2)+'\n')
 return 0 if passed else 1
if __name__=='__main__':raise SystemExit(main())
