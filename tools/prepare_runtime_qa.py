#!/usr/bin/env python3
"""Prepare native release checks from an exact LHA and sample-only AmiHASS service."""
import argparse, hashlib, json, shutil, subprocess, sys
from pathlib import Path
import release
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/native_six.c','src/control_client.c','src/config.c','src/nd_options.c','src/interaction.c',
         'plugin_src/common/nd_panel.c','plugin_src/common/nd_list.c','plugin_src/common/nd_task.c',
         'plugin_src/common/nd_text.c','plugin_src/clipboard/nd_clipboard.c','plugin_src/tooltypes/nd_tooltypes.c',
         'plugin_src/hass/nd_hass.c','plugin_src/hass/nd_hass_client.c','plugin_src/screenmode/nd_scrmode.c']
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--package',type=Path,required=True)
    ap.add_argument('--bridge',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args();package=args.package.resolve();bridge=args.bridge.resolve();out=args.output.resolve()
    if bridge.read_bytes()[:4]!=b'\0\0\3\xf3':raise ValueError('Bridge must be a native Amiga HUNK executable.')
    out.mkdir(parents=True,exist_ok=False);dest=out/'Release04';dest.mkdir()
    subprocess.run(['lha','xw='+str(dest),str(package)],check=True,stdout=subprocess.DEVNULL)
    drawer=dest/'NovaDock'
    for name in ('HASS','Calc','ToolTypes'):
        shutil.copy2(drawer/'Plugins'/name,out/name)
        shutil.copytree(drawer/'Plugins'/(name+'.assets'),out/(name+'.assets'))
    shutil.copy2(bridge,out/'AmiHASSService')
    env=release.credentials.build_environment(release.config(release.default_config()))
    subprocess.run([sys.executable,str(ROOT/'scripts/compile-amiga.py'),'--compiler',env['VC'],'--','-O',
                    '-Iinclude','-Iplugin_src/common','-Iplugin_src/tooltypes','-Iplugin_src/hass','-Iplugin_src/screenmode',
                    '-o',str(out/'SixCheck'),*SOURCES,'-lamiga'],cwd=ROOT,env=env,check=True)
    (out/'ISOLATED-QA-ONLY').write_text('Run only with AmigaMac --test and disposable guest disks.\n')
    definition={'name':'NovaDock 0.4 constrained release runtime','application':'QATests:SixCheck',
                'timeoutSeconds':300,'expectedReturnCode':0}
    (out/'test.json').write_text(json.dumps(definition,indent=2)+'\n')
    definition.update(name='NovaDock 0.4 native release runtime at 640x480',arguments=['SCREEN480'])
    (out/'test480.json').write_text(json.dumps(definition,indent=2)+'\n')
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    (out/'inputs.json').write_text(json.dumps({'package_sha256':sha(package),'bridge_sha256':sha(bridge),
                                              'harness_sha256':sha(out/'SixCheck')},indent=2)+'\n')
    print('Prepared exact-package native runtime checks:',out)
if __name__=='__main__':main()
