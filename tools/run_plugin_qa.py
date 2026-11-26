#!/usr/bin/env python3
"""Run the bundled-plugin release gate: host domain tests + offline native QA.

Default: build, test every plugin, and write machine-readable evidence. --plugin
is a focused development run; --skip-host cannot produce release evidence.
--package packages the exact tested binaries only after every check passes.
"""
import argparse,datetime,hashlib,json,os,re,shlex,shutil,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import check_plugin_qa as coverage

def native_results(text,cases,strict=True):
 result={c['id']:{'status':'missing'} for c in cases}
 for line in text.splitlines():
  fields=line.split('|')
  if fields[0]=='METRIC' and len(fields)==12 and fields[1] in result:
   names=['idle_packets','tracked_ram','peak_ram','cpu_hits','cpu_samples','opens','reads','writes','events']
   if 'metrics' in result[fields[1]]:raise ValueError('Duplicate native metric: '+fields[1])
   result[fields[1]]['metrics']=dict(zip(names,map(int,fields[3:])))
   result[fields[1]]['metric_passed']=fields[2]=='1'
  elif fields[0]=='CASE' and len(fields)==3 and fields[1] in result:
   if result[fields[1]]['status']!='missing':raise ValueError('Duplicate native case: '+fields[1])
   result[fields[1]]['status']=fields[2]
 for case in result.values():
  if not case.get('metric_passed') or 'metrics' not in case:case['status']='failed'
 if strict and not re.search(r'^SUMMARY cases=%d failures=0$'%len(cases),text,re.M):raise ValueError('Native summary is incomplete or failed')
 if strict and any(c['status']!='passed' for c in result.values()):raise ValueError('Native plugin results incomplete or failed')
 return result

def copy_runtime(drawer,destination,cases):
 destination.mkdir(parents=True)
 for name in ['NovaDock','SupportLog','SaveConfig']+[c['program'] for c in cases]:
  target=destination/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(drawer/name,target)
 for c in cases:
  manifest=drawer/(c['program']+'.ndplugin')
  if manifest.exists():shutil.copy2(manifest,destination/(c['program']+'.ndplugin'))
  assets=drawer/(c['program']+'.assets')
  if assets.exists():shutil.copytree(assets,destination/(c['program']+'.assets'))
 for name in ['Icons','Themes/Nord']:
  shutil.copytree(drawer/name,destination/name,ignore=shutil.ignore_patterns('*.png','*.c','*.o'))

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--output',type=Path,required=True,help='new directory for evidence and disposable fixture')
 p.add_argument('--drawer',type=Path,default=ROOT,help='runtime drawer to test')
 p.add_argument('--hidden',action='store_true',help='hide the host emulator window; guest rendering remains enabled')
 p.add_argument('--no-build',action='store_true');p.add_argument('--skip-host',action='store_true',help='development only; not valid release evidence')
 p.add_argument('--plugin',action='append',default=[],help='ID or program name; repeat to select cases')
 p.add_argument('--system',type=Path);p.add_argument('--resources',type=Path);p.add_argument('--timeout',type=int,default=900)
 p.add_argument('--package',type=Path,help='package output directory; requires complete QA')
 a=p.parse_args();cases=coverage.load();selected=[c for c in cases if not a.plugin or c['id'] in a.plugin or Path(c['program']).name in a.plugin]
 if not selected:p.error('No matching plugin cases')
 if a.plugin and set(a.plugin)-{v for c in selected for v in (c['id'],Path(c['program']).name)}:p.error('Unknown plugin selection')
 if a.package and (a.skip_host or len(selected)!=len(cases)):p.error('--package requires the complete host/native run')
 import build_identity
 try:build_identity.sync(check=a.no_build)
 except ValueError as error:p.error(str(error))
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);drawer=a.drawer.resolve()
 report={'schema':1,'status':'running','host':'pending','native':'pending','coverage_sha256':coverage.sha(ROOT/'tests/plugin_qa.json'),'sources':coverage.sources(),'results':{},'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'scope':[c['id'] for c in selected], 'limitations':['CPU is a statistical main-task estimate; RAM is tracked allocations.','Offline unavailable-provider paths do not validate physical hardware or live services.']}
 def save(): (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 save()
 try:
  import release,build_provenance
  env=release.credentials.build_environment(release.config(release.default_config()))
  if not a.no_build:
   if drawer!=ROOT:p.error('Use --no-build for an extracted/staged drawer')
   with (out/'build.log').open('w') as f:subprocess.run(['sh','scripts/build-amiga.sh'],cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,check=True)
  report['binaries']=coverage.hashes(drawer,cases)
  if not a.skip_host:
   # A source-archive test copy must not inherit the checkout's Git parent.
   source=Path(tempfile.mkdtemp(prefix='nd-plugin-host-',dir='/private/tmp'))
   for name in set(build_provenance.source_inputs())|set(build_provenance.native_programs()):
    dest=source/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dest)
   host_env=dict(env,ND_DEFER_WEB_CHECKS='1')
   print('Running host feature tests for all plugin cases...',flush=True)
   try:
    with (out/'host.log').open('w') as f:subprocess.run(['sh','tests/run_host_tests.sh'],cwd=source,env=host_env,stdout=f,stderr=subprocess.STDOUT,check=True)
   finally:shutil.rmtree(source)
   report['host']='passed'
  else:report['host']='skipped'
  save();fixture=out/'fixture';fixture.mkdir();copy_runtime(drawer,fixture/'Dock',cases)
  (fixture/'qa-cases.tsv').write_text(''.join('%s\t%s\t%d\t%d\n'%(c['id'],c['program'],c['panel'],c['idle_packet_budget']) for c in selected))
  (fixture/'ISOLATED-QA-ONLY').touch();(fixture/'test.json').write_text(json.dumps({'application':'QATests:AllPluginsQA'}))
  plan=subprocess.check_output(['make','-n','-B','-f','Makefile.cross','NovaDock'],cwd=ROOT,env=env,text=True)
  command=shlex.split(next(l for l in plan.splitlines() if 'scripts/compile-amiga.py' in l));command=command[:command.index('--')+1]
  command+=['-O','-Iinclude','-o',str(fixture/'AllPluginsQA'),'tests/native_all_plugins.c','tests/native_rexx_save_barrier.c','src/control_client.c','src/config.c','src/nd_options.c','src/nd_support.c','-lamiga']
  with (out/'compile.log').open('w') as f:subprocess.run(command,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,check=True)
  command=[sys.executable,'tools/run_native_fixture.py',str(fixture),'--timeout',str(a.timeout)]
  if a.hidden:command+=['--hidden']
  if a.system:command+=['--system',str(a.system)]
  if a.resources:command+=['--resources',str(a.resources)]
  print('Running offline FS-UAE checks for %d plugins; evidence: %s'%(len(selected),out),flush=True)
  with (out/'native.log').open('w') as f:native_process=subprocess.run(command,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT)
  native=(out/'native.log').read_text();base=Path(next(line for line in native.splitlines() if line.startswith('/private/tmp/nd-native-latest-')))
  # Preserve complete results; the generic native runner prints only its tail.
  log=(base/'AutoRun/launch.log').read_text();(out/'plugin-results.log').write_text(log)
  report['results']=native_results(log,selected,strict=False)
  native_results(log,selected)
  if native_process.returncode:raise ValueError('Native fixture returned '+str(native_process.returncode))
  # Exercise the real dock executable in both portable and split Install layouts.
  command=[sys.executable,'tools/run_program_directory_qa.py','--no-build','--output',str(out/'directory-fixture')]
  if not a.hidden:command+=['--visible']
  if a.resources:command+=['--resources',str(a.resources)]
  with (out/'directory.log').open('w') as f:subprocess.run(command,cwd=ROOT,env=env,stdout=f,stderr=subprocess.STDOUT,check=True)
  directory=(out/'directory-fixture/native.log').read_text()
  if 'PASS: component lookup, 6 Shell/Workbench launch cases, 30 live plugins' not in directory:raise ValueError('Component directory regression did not finish')
  report['component_lookup']='passed'
  report['native']='passed'
  report['status']='passed' if report['host']=='passed' and len(selected)==len(cases) else 'partial'
  report['runtime_system']=json.loads((base/'system-source.json').read_text())
  report['evidence']={name:coverage.sha(out/name) for name in ['compile.log','native.log','plugin-results.log','directory.log']+(['host.log'] if report['host']=='passed' else [])}
  save()
  if a.package:
   coverage.validate(out/'report.json',drawer)
   package_env=dict(env,ND_SKIP_BUILD='1',ND_REQUIRE_PLUGIN_QA='1',ND_PLUGIN_QA_EVIDENCE=str(out/'report.json'))
   subprocess.run(['sh','scripts/make-release.sh',str(a.package.resolve())],cwd=ROOT,env=package_env,check=True)
  print('%s: %d plugin cases; %s'% (report['status'].upper(),len(selected),out/'report.json'))
  return 0
 except (OSError,ValueError,KeyError,subprocess.CalledProcessError) as error:
  report['status']='failed';report['error']=str(error);save();print('Plugin QA failed: '+str(error),file=sys.stderr);return 1
if __name__=='__main__':raise SystemExit(main())
