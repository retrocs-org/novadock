#!/usr/bin/env python3
"""Check the paired AmiHASS SDK and production NovaDock adapter offline in FS-UAE.

Only disposable System/QATests/AutoRun drives are mounted. The service uses a
private RAM configuration and shared sample entities; no real devices are used.
"""
import argparse,hashlib,json,shlex,shutil,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--bridge',type=Path,default=ROOT.parent/'AmiHASS')
 parser.add_argument('--system',type=Path)
 parser.add_argument('--resources',type=Path)
 parser.add_argument('--timeout',type=int,default=240)
 parser.add_argument('--visible',action='store_true',help='show the isolated emulator; hidden by default')
 parser.add_argument('--output',type=Path)
 args=parser.parse_args();bridge=args.bridge.resolve()
 subprocess.run([sys.executable,str(ROOT/'tools/check_amihass_pair.py'),'--bridge',str(bridge)],cwd=ROOT,check=True)
 service=bridge/'AmiHASSService'
 if not service.is_file():parser.error('Build AmiHASSService in the paired repository first')
 fixture=args.output.resolve() if args.output else Path(tempfile.mkdtemp(prefix='nd-hass-fixture-',dir='/private/tmp'))
 fixture.mkdir(parents=True,exist_ok=True)
 if (fixture/'test.json').exists():parser.error('Use a new output directory to preserve earlier evidence')
 print('Fixture:',fixture,flush=True)
 import release
 env=release.credentials.build_environment(release.config(release.default_config()))
 plan=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True)
 cmd=shlex.split(next(line for line in plan.splitlines() if 'scripts/compile-amiga.py' in line))
 cmd=cmd[:cmd.index('--')+1]+['-O','-Iinclude','-Iplugin_src/hass','-o',str(fixture/'Regression'),'tests/native_hass_service03.c','plugin_src/hass/nd_hass.c','plugin_src/hass/nd_hass_client.c','src/interaction.c','-lamiga']
 with (fixture/'compile.log').open('w') as log:subprocess.run(cmd,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 (fixture/'Service').mkdir();shutil.copy2(service,fixture/'Service/AmiHASSService')
 (fixture/'service-source.json').write_text(json.dumps({'path':str(service),'sha256':hashlib.sha256(service.read_bytes()).hexdigest(),'sdk':json.loads((ROOT/'plugin_src/hass/service/sdk.json').read_text()),'offline':True},indent=2)+'\n')
 (fixture/'ISOLATED-QA-ONLY').touch();(fixture/'test.json').write_text(json.dumps({'application':'QATests:Regression','expectedReturnCode':0}))
 cmd=[sys.executable,str(ROOT/'tools/run_native_fixture.py'),str(fixture),'--timeout',str(args.timeout)]
 if not args.visible:cmd+=['--hidden']
 if args.system:cmd+=['--system',str(args.system)]
 if args.resources:cmd+=['--resources',str(args.resources)]
 with (fixture/'run.log').open('w') as log:
  proc=subprocess.Popen(cmd,cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
  for line in proc.stdout:print(line,end='',flush=True);log.write(line);log.flush()
  return proc.wait()
if __name__=='__main__':raise SystemExit(main())
