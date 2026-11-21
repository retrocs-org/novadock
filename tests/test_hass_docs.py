"""Public HASS setup must match the paired SDK and current installer entry point."""
from pathlib import Path
import json,re,subprocess
ROOT=Path(__file__).resolve().parents[1]
def validate(text,version):
 section=text.split('## Connect through AmiHASS\n',1)[1].split('\n## ',1)[0]
 found=re.search(r'Install the separate \*\*AmiHASS ([0-9.]+)\*\* package using \*\*([^*]+)\*\*',section)
 return bool(found and found[1]==version and found[2]=='Install')
version=json.loads((ROOT/'plugin_src/hass/service/sdk.json').read_text())['version']
text=(ROOT/'docs/HASS_PLUGIN.md').read_text();assert validate(text,version)
assert not validate(text.replace('**AmiHASS '+version+'**','**AmiHASS 0.0**'),version)
assert not validate(text.replace('package using **Install**','package using **Install-AmiHASS**'),version)
assert 'Install-AmiHASS' not in text
# Historical descriptions of old services are legitimate; current setup/support claims are not.
for line in text.splitlines():
 if re.search(r'AmiHASS 0\.1',line):assert 'older AmiHASS 0.1' in line,line
web=(ROOT/'public-web/includes/plugin_catalog.php').read_text();hass=next(line for line in web.splitlines() if "'executable' => 'HASS'" in line)
assert 'Requires AmiHASS '+version+';' in hass
subprocess.run(['python3',str(ROOT/'tools/make_amigaguide.py'),'--check'],check=True)
guide=(ROOT/'NovaDock.guide').read_text('latin-1');assert 'Install-AmiHASS' not in guide and 'AmiHASS '+version in guide
print('HASS setup, website requirements and generated guide match the SDK pair and Install entry point; stale variants are rejected.')
