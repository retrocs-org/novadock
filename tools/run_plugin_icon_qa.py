#!/usr/bin/env python3
"""Replay every bundled artwork frame through the dock in an isolated FS-UAE guest."""
import argparse,hashlib,json,shlex,shutil,struct,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path);p.add_argument('--resources',type=Path);p.add_argument('--profile',choices=('a4000-rtg','a500-ecs'),default='a4000-rtg');p.add_argument('--programs',nargs='+');p.add_argument('--side',choices=('BOTTOM','TOP','LEFT','RIGHT'),default='BOTTOM');p.add_argument('--height',type=int,default=76);p.add_argument('--timeout',type=int,default=600);a=p.parse_args()
 out=a.output.resolve() if a.output else Path(tempfile.mkdtemp(prefix='nd-plugin-icons-',dir='/private/tmp'));out.mkdir(parents=True,exist_ok=True)
 if (out/'test.json').exists():p.error('Use a new output directory')
 import release
 env=release.credentials.build_environment(release.config(release.default_config()))
 plan=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True);cmd=shlex.split(next(x for x in plan.splitlines() if 'scripts/compile-amiga.py' in x));cmd=cmd[:cmd.index('--')+1]
 with (out/'compile.log').open('w') as log:
  subprocess.run(cmd+['-O','-Iinclude','-Iplugin_src/common','-o',str(out/'Replay'),'tests/native_icon_replay.c','plugin_src/common/nd_client.c'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
  subprocess.run(cmd+['-O','-Iinclude','-DND_ICON_SIDE="'+a.side+'"','-DND_ICON_HEIGHT='+str(a.height),'-o',str(out/'IconQA'),'tests/native_plugin_icons.c','src/control_client.c','src/config.c','src/nd_options.c','src/prefs_layout.c','-lamiga'],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 dock=out/'Dock';(dock/'Plugins').mkdir(parents=True)
 for name in ('NovaDock','Plugins/Eyes'):shutil.copy2(ROOT/name,dock/name)
 banks=sorted((ROOT/'Plugins').glob('*.assets/Artwork.ndart'));names=[];bank_info={}
 for bank in banks:
  name=bank.parent.name[:-7]
  if a.programs and name not in a.programs:continue
  names.append(name);bank_info[name]={"frames":struct.unpack_from(">H",bank.read_bytes(),6)[0],"sha256":hashlib.sha256(bank.read_bytes()).hexdigest()};shutil.copy2(out/'Replay',dock/'Plugins'/name);shutil.copytree(bank.parent,dock/'Plugins'/bank.parent.name)
 (out/'icon-banks').write_text('\n'.join(names)+'\n');shutil.copytree(ROOT/'Themes/Nord',dock/'Themes/Nord')
 (out/'ISOLATED-QA-ONLY').touch();(out/'test.json').write_text(json.dumps({'application':'QATests:IconQA','expectedReturnCode':0}))
 cmd=[sys.executable,str(ROOT/'tools/run_native_fixture.py'),str(out),'--profile',a.profile,'--timeout',str(a.timeout),'--hidden']
 if a.resources:cmd+=['--resources',str(a.resources.resolve())]
 result=subprocess.run(cmd,cwd=ROOT,env=env,capture_output=True,text=True);(out/'run.log').write_text(result.stdout+result.stderr);print(result.stdout+result.stderr,end='')
 bases=[x for x in result.stdout.splitlines() if x.startswith('/private/tmp/nd-native-latest-')]
 if bases:
  base=Path(bases[0]);shutil.copy2(base/'system-source.json',out/'system-source.json')
  if (base/'AutoRun/launch.log').exists():shutil.copy2(base/'AutoRun/launch.log',out/'native.log')
  for pic in (base/'QATests').glob('*.ppm'):shutil.copy2(pic,out/pic.name)
 frames=sum(info['frames'] for info in bank_info.values())
 marker=f'PASS: plugin banks {frames*2} frame/size cases, {frames*4} idle/magnified native paints, Tiny EYES live artwork.'
 native=(out/'native.log').read_text() if (out/'native.log').exists() else ''
 passed=result.returncode==0 and marker in native
 (out/'report.json').write_text(json.dumps({'status':'passed' if passed else 'failed','profile':a.profile,'side':a.side,'tile_height':a.height,'banks':bank_info,'states':frames,'frame_size_cases':frames*2,'native_paints':frames*4,'tested_binaries':{n:hashlib.sha256((dock/n).read_bytes()).hexdigest() for n in ('NovaDock','Plugins/Eyes')}},indent=2)+'\n')
 return 0 if passed else 1
if __name__=='__main__':raise SystemExit(main())
