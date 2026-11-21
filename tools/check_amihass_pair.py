#!/usr/bin/env python3
"""Verify the independent AmiHASS release paired with NovaDock's embedded SDK."""
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def check(bridge):
    values=dict(line.split('=',1) for line in (bridge/'VERSION').read_text().splitlines() if '=' in line)
    snapshot=json.loads((ROOT/'plugin_src/hass/service/sdk.json').read_text())
    if values['VERSION']!=snapshot['version']:raise ValueError('AmiHASS release differs from the tested SDK pair; review and update the pairing explicitly')
    mapping={'amihass.h':'SDK/amihass.h','amihass_core.h':'SDK/amihass_core.h','amihass_config.h':'SDK/amihass_config.h','amihass_client.c':'SDK/amihass_client.c','core.c':'src/core.c','nd_reply.h':'SDK/nd_reply.h'}
    hashes={}
    for name,path in mapping.items():
        data=(bridge/path).read_bytes();digest=hashlib.sha256(data).hexdigest()
        if digest!=snapshot['files'][name]:raise ValueError('AmiHASS public SDK differs: '+path)
        hashes[path]=digest
    return {'novadock_version':dict(line.split('=',1) for line in (ROOT/'VERSION').read_text().splitlines() if '=' in line)['VERSION'],'amihass_version':values['VERSION'],'abi':snapshot['abi'],'sdk_hashes':hashes}
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--bridge',type=Path,required=True);p.add_argument('--output',type=Path);a=p.parse_args()
    try:result=check(a.bridge)
    except (ValueError,OSError,KeyError) as e:p.error(str(e))
    if a.output:a.output.write_text(json.dumps(result,indent=2)+'\n')
    print('NovaDock '+result['novadock_version']+' / AmiHASS '+result['amihass_version']+': ABI and all public SDK hashes match.')
if __name__=='__main__':main()
