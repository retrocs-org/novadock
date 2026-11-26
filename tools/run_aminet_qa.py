#!/usr/bin/env python3
"""Run a live Aminet search/download test in a disposable FS-UAE guest.

Downloads JustName.lha; never installs, extracts or executes the package.
Requires the normal toolchain, FS-UAE and an imported AmigaOS 3.2.3 system.
"""
import argparse,hashlib,json,shlex,shutil,subprocess,sys,tempfile,urllib.request
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--no-build',action='store_true');parser.add_argument('--system',type=Path);parser.add_argument('--resources',type=Path);args=parser.parse_args()
 import release
 env=release.credentials.build_environment(release.config(release.default_config()))
 fixture=Path(tempfile.mkdtemp(prefix='nd-aminet-fixture-',dir='/private/tmp'));print('Fixture:',fixture,flush=True)
 if not args.no_build:
  with (fixture/'build.log').open('w') as log:subprocess.run(['sh','scripts/build-amiga.sh'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 output=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True)
 cmd=shlex.split(next(line for line in output.splitlines() if 'scripts/compile-amiga.py' in line));cmd=cmd[:cmd.index('--')+1]+['-O','-Iinclude','-Iplugin_src/aminet','-o',str(fixture/'Regression'),'tests/native_aminet_browser.c','plugin_src/aminet/nd_aminet.c','-lamiga']
 with (fixture/'compile.log').open('w') as log:subprocess.run(cmd,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 (fixture/'Dock/Plugins').mkdir(parents=True);shutil.copy2(ROOT/'Plugins/Aminet',fixture/'Dock/Plugins/Aminet')
 (fixture/'ISOLATED-QA-ONLY').touch();(fixture/'test.json').write_text(json.dumps({'application':'QATests:Regression'}))
 cmd=[sys.executable,str(ROOT/'tools/run_native_fixture.py'),str(fixture),'--network','--timeout','240']
 if args.system:cmd+=['--system',str(args.system)]
 if args.resources:cmd+=['--resources',str(args.resources)]
 result=subprocess.run(cmd,cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 print(result.stdout,flush=True);(fixture/'native.log').write_text(result.stdout)
 if result.returncode:return result.returncode
 guest=next(Path(line) for line in result.stdout.splitlines() if line.startswith('/private/tmp/nd-native-latest-'))
 archive=guest/'QATests/Downloads/JustName.lha';data=archive.read_bytes()
 with urllib.request.urlopen('https://m68k.aminet.net/util/cli/JustName.lha',timeout=30) as response:expected=response.read(1048577)
 if len(expected)>1048576 or data!=expected:raise ValueError('Native download differs from independently fetched HTTPS archive')
 subprocess.run([env.get('LHA','lha'),'-t',str(archive)],check=True)
 print('PASS native archive matches independent HTTPS download; LHA CRC passed; bytes=%d sha256=%s'%(len(data),hashlib.sha256(data).hexdigest()))
 return 0
if __name__=='__main__':raise SystemExit(main())
