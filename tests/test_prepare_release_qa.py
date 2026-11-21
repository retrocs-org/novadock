"""QA identities follow archive bytes, including renamed installer entry points."""
from pathlib import Path
import importlib.util,json,tempfile,shutil
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('prepare_release_qa',ROOT/'tools/prepare_release_qa.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as tmp:
 base=Path(tmp);fixtures={}
 for name,version,installer in [('old', '0.4','Install-NovaDock'),('candidate','0.5','Install')]:
  folder=base/name;folder.mkdir();drawer=folder/'NovaDock';drawer.mkdir();(drawer/'NovaDock').write_bytes(b'\0$VER: NovaDock '+version.encode()+b' (20.11.2026)\0');(drawer/installer).write_text((ROOT/'Install').read_text());(drawer/'NovaDockPrefs').write_bytes(b'preferences fixture');archive=base/(name+'.arbitrary');archive.write_bytes(name.encode());fixtures[str(archive.resolve())]=drawer
 def extract(command,**kwargs):
  assert command[0]=='lha' and command[1].startswith('xw=');shutil.copytree(fixtures[command[2]],Path(command[1][3:])/'NovaDock')
 m.subprocess.run=extract
 record=m.prepare(base/'candidate.arbitrary',base/'old.arbitrary',base/'qa')
 assert record['previous_version']=='0.4' and record['current_version']=='0.5'
 assert record['packages']['Previous']['installer_name']=='Install-NovaDock' and record['packages']['Current']['installer_name']=='Install'
 assert (base/'qa/versions.txt').read_text()=='0.4\n0.5\n'
 assert '0.4 to 0.5' in json.loads((base/'qa/test.json').read_text())['name']
 assert 'QATests:Current/NovaDock' in (base/'qa/Current-Install').read_text()
 for level in (0,1,2):
  script=(base/('qa/Current-Install-'+str(level))).read_text()
  assert '@user-level' not in script and '(> '+str(level)+' 0)' in script
  assert '(user #remove-level)' not in script and '(user #maintenance-level)' not in script
  assert '(copyfiles ' in script and '(delete ' in script and '(run ' in script
 assert record['packages']['Current']['logical_user_levels']==[0,1,2]
 assert 'Current/NovaDock/NovaDock\tSYS:Utilities/NovaDock' in (base/'qa/Current-files.tsv').read_text()
 custom=m.prepare(base/'candidate.arbitrary',base/'old.arbitrary',base/'custom','custom')
 assert list(custom['layouts'])==[1,2]
 for level in (1,2):
  script=(base/('custom/Current-Install-'+str(level))).read_text()
  volume='SYS:' if level==1 else 'QATests:'
  assert '"'+volume+'QA App Drawers"' in script and '"'+volume+'QA Pref Drawers"' in script
  assert ('"QATests:QA Shared Data"' if level==2 else '"SYS:"') in script
  files=(base/('custom/Current-'+str(level)+'-files.tsv')).read_text()
  assert volume+'QA App Drawers/NovaDock' in files
  assert volume+'QA Pref Drawers/NovaDockPrefs' in files
 assert m.installed_path('Plugins/Bench',custom['layouts'][2])=='QATests:QA Shared Data/NovaDock/Plugins/Bench'
 assert not (base/'custom/Current-Install-0').exists()
 assert '1\tSYS:QA App Drawers\tSYS:QA Pref Drawers\tSYS:NovaDock' in (base/'custom/layouts.tsv').read_text()
 try:m.prepare(base/'old.arbitrary',base/'candidate.arbitrary',base/'backwards')
 except ValueError:pass
 else:raise AssertionError('Reverse upgrade accepted')
 try:m.prepare(base/'candidate.arbitrary',base/'old.arbitrary',base/'qa')
 except FileExistsError:pass
 else:raise AssertionError('Existing QA evidence overwritten')
 drawer=fixtures[str((base/'candidate.arbitrary').resolve())];(drawer/'NovaDock').write_bytes(b'$VER: NovaDock 0.5 (x)\0$VER: NovaDock 0.6 (x)')
 try:m.package_version(drawer)
 except ValueError:pass
 else:raise AssertionError('Ambiguous executable version accepted')
print('Release QA: package-derived versions, legacy/current Install names, hashes, reverse-upgrade rejection and preserved evidence passed.')

spec=importlib.util.spec_from_file_location('update_version',ROOT/'tools/update_version.py');up=importlib.util.module_from_spec(spec);spec.loader.exec_module(up)
text=(ROOT/'docs/RELEASE_CHECKLIST.md').read_text();future=up.checklist_text(text,'0.6','0.5')
assert 'Prepare the 0.6 candidate' in future and 'notes at 0.6.' in future and 'exact tag 0.6' in future and 'Upgrade from published 0.5' in future
print('Release checklist follows the current candidate and previous published version.')
