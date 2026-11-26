#!/usr/bin/env python3
"""Require explicit domain and native QA coverage for every bundled client."""
import argparse,hashlib,json,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def inventory(root=ROOT):
 lines=(root/'Makefile.cross').read_text().splitlines()
 return {line.split(':',1)[0] for n,line in enumerate(lines[:-1]) if line.startswith('Plugins/') and ':' in line and 'nd_client.c' in lines[n+1]}
def load(root=ROOT):
 data=json.loads((root/'tests/plugin_qa.json').read_text());cases=data['plugins'];programs=[c['program'] for c in cases];ids=[c['id'] for c in cases]
 if data.get('schema')!=1 or len(set(programs))!=len(programs) or len(set(ids))!=len(ids):raise ValueError('Invalid or duplicate QA cases')
 if set(programs)!=inventory(root):raise ValueError('QA coverage differs from bundled clients: '+str(sorted(set(programs)^inventory(root))))
 suite=(root/'tests/run_host_tests.sh').read_text()
 for c in cases:
  if not re.fullmatch(r'Plugins/[A-Za-z0-9]+',c['program']) or not re.fullmatch(r'[A-Z0-9_-]{1,19}',c['id']):raise ValueError('Invalid QA identity')
  if not c.get('scenario') or not c.get('host_tests') or not isinstance(c.get('panel'),bool) or not 1<=c['idle_packet_budget']<=200:raise ValueError('Incomplete case: '+c['program'])
  for test in c['host_tests']:
   if not re.fullmatch(r'tests/test_[A-Za-z0-9_]+\.(?:py|c)',test) or not (root/test).is_file() or test not in suite:raise ValueError('Domain test missing from host suite: '+test)
  manifest=root/(c['program']+'.ndplugin')
  if manifest.exists():
   match=re.search(r'^NOVADOCK_ID=(.+)$',manifest.read_text(),re.M)
   if not match or match[1]!=c['id']:raise ValueError('Manifest ID differs: '+c['program'])
 return cases
def hashes(drawer,cases):
 names=['NovaDock','SupportLog']+[c['program'] for c in cases]
 makefile=drawer/'Makefile'
 if makefile.exists():
  match=re.search(r'^all:\s*(.+)$',makefile.read_text(),re.M)
  if match:names+=match[1].split()
 else:
  # Extracted runtime drawers omit Makefile; use the source target inventory.
  for name in ('NovaDockPrefs','SupportReport','MakeIcon','SetupStartup','StartInstaller','SaveConfig','Plugins/SensorsI2C'):
   if (drawer/name).exists():names.append(name)
 # Include runtime dependencies and defaults: evidence must describe the files used.
 for prefix in ('Plugins','Themes','Icons'):
  names += [p.relative_to(drawer).as_posix() for p in (drawer/prefix).rglob('*') if p.is_file() and p.suffix.lower() in ('.ndplugin','.ndart','.iff','.cfg')]
 return {name:sha(drawer/name) for name in sorted(set(names))}
def sources(root=ROOT):
 result={}
 for folder in ('include','src','prefs','plugin_src','tests','tools','scripts'):
  for path in (root/folder).rglob('*'):
   if path.is_file() and path.suffix in ('.c','.h','.py','.sh','.asm','.json') and not any(x.startswith('.') or x=='__pycache__' for x in path.relative_to(root).parts):result[path.relative_to(root).as_posix()]=sha(path)
 return result
def validate(report,drawer,root=ROOT):
 cases=load(root);data=json.loads(Path(report).read_text())
 if data.get('schema')!=1 or data.get('status')!='passed' or data.get('host')!='passed' or data.get('native')!='passed':raise ValueError('Complete passing host/native plugin QA is required')
 if data.get('component_lookup')!='passed':raise ValueError('Passing portable and split Install lookup QA is required')
 if data.get('coverage_sha256')!=sha(root/'tests/plugin_qa.json') or data.get('sources')!=sources(root) or data.get('binaries')!=hashes(Path(drawer),cases):raise ValueError('QA evidence is stale: binaries, assets or coverage changed')
 if set(data.get('results',{}))!={c['id'] for c in cases} or any(v.get('status')!='passed' for v in data['results'].values()):raise ValueError('Missing/failing native plugin results')
 for name,digest in data.get('evidence',{}).items():
  if Path(name).name!=name or sha(Path(report).parent/name)!=digest:raise ValueError('QA log changed: '+name)
 if not {'host.log','native.log','plugin-results.log','compile.log','directory.log'}<=set(data.get('evidence',{})):raise ValueError('QA logs are missing')
 return data
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--drawer',type=Path);p.add_argument('--evidence',type=Path);a=p.parse_args()
 try:
  cases=load()
  if a.evidence:
   if not a.drawer:p.error('--evidence needs --drawer')
   validate(a.evidence,a.drawer)
  print('Plugin QA coverage: %d explicit cases; domain tests and native lifecycle required.'%len(cases))
 except (OSError,ValueError,KeyError,TypeError) as e:p.exit(1,'Plugin QA failed: '+str(e)+'\n')
if __name__=='__main__':main()
