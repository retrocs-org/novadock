#!/usr/bin/env python3
"""Build and test SENSOR/WHDLoad in an isolated, offline FS-UAE installation.

Uses ordinary AK_Sensors ENV inputs; never requires sensor hardware, patches a
hardware driver, launches WHDLoad, or mounts the user's persistent guest drives.
"""
import argparse,json,shlex,shutil,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--no-build',action='store_true',help='use existing normal binaries')
 parser.add_argument('--system',type=Path,help='imported AmigaOS 3.2.3 System drawer')
 parser.add_argument('--timeout',type=int,default=180)
 parser.add_argument('--resources',type=Path,help='FS-UAE app resources / Kickstarts directory parent')
 args=parser.parse_args()
 import release
 env=release.credentials.build_environment(release.config(release.default_config()))
 fixture=Path(tempfile.mkdtemp(prefix='nd-sensor-fixture-',dir='/private/tmp'))
 print('Fixture:',fixture,flush=True)
 if not args.no_build:
  with (fixture/'build.log').open('w') as log:subprocess.run(['sh','scripts/build-amiga.sh'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 output=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True)
 command=shlex.split(next(line for line in output.splitlines() if 'scripts/compile-amiga.py' in line))
 command=command[:command.index('--')+1]+['-O','-Iinclude','-o',str(fixture/'Regression'),'tests/native_sensor_display.c','src/control_client.c','src/config.c','src/nd_options.c','-lamiga']
 with (fixture/'compile.log').open('w') as log:subprocess.run(command,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 dock=fixture/'Dock';(dock/'Plugins').mkdir(parents=True)
 for name in ('NovaDock','Plugins/Sensors','Plugins/Games','Plugins/Sensors.ndplugin','Plugins/Games.ndplugin'):
  shutil.copy2(ROOT/name,dock/name)
 for name in ('Icons','Themes/Nord','Plugins/Sensors.assets','Plugins/Games.assets'):
  shutil.copytree(ROOT/name,dock/name)
 (fixture/'ISOLATED-QA-ONLY').touch()
 (fixture/'test.json').write_text(json.dumps({'application':'QATests:Regression'}))
 command=[sys.executable,str(ROOT/'tools/run_native_fixture.py'),str(fixture),'--timeout',str(args.timeout)]
 if args.system:command+=['--system',str(args.system)]
 if args.resources:command+=['--resources',str(args.resources)]
 return subprocess.run(command,cwd=ROOT,env=env).returncode
if __name__=='__main__':raise SystemExit(main())
