#!/usr/bin/env python3
"""Offline native test of the AMINOTIS tile against the real AmiNotis service.

Runs in tools/run_native_fixture.py's disposable guest, which serializes with
other native QA through its host lock. Needs a built AmiNotis runtime drawer."""
import argparse, json, shlex, shutil, subprocess, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--service-drawer', type=Path, default=ROOT.parent/'AmiNotis/build/Amiga')
    p.add_argument('--resources', type=Path)
    p.add_argument('--profile', choices=('a4000-rtg', 'a500-ecs'), default='a4000-rtg')
    a = p.parse_args()
    out = a.output.resolve(); out.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(ROOT/'tools'))
    import release, build_provenance
    env = release.credentials.build_environment(release.config(release.default_config()))
    plan = subprocess.check_output(['make', '-n', '-B', '-f', 'Makefile.cross', 'Plugins/AmiNotis'], cwd=ROOT, env=env, text=True)
    cmd = shlex.split(next(x for x in plan.splitlines() if 'scripts/compile-amiga.py' in x)); cmd = cmd[:cmd.index('--')+1]
    with (out/'compile.log').open('w') as log:
        subprocess.run(cmd+['-O', '-Iinclude', '-Iplugin_src/common', '-o', str(out/'AmiNotisPluginQA'),
                            'tests/native_aminotis_plugin.c', 'plugin_src/aminotis/sdk/aminotis.c', 'src/control_client.c', 'src/config.c', 'src/nd_options.c', '-lamiga'],
                       cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
    dock = out/'Dock'; dock.mkdir()
    for name in build_provenance.binaries():
        target = dock/name; target.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(ROOT/name, target)
    for name in ('Plugins/AmiNotis.ndplugin', 'Plugins/AmiNotis.info'):
        shutil.copy2(ROOT/name, dock/name)
    shutil.copytree(ROOT/'Plugins/AmiNotis.assets', dock/'Plugins/AmiNotis.assets')
    for folder in ('Icons', 'Themes/Nord'):
        shutil.copytree(ROOT/folder, dock/folder)
    service = a.service_drawer.resolve()
    for name in ('AmiNotisService', 'AmiNotisPresenter', 'AmiNotisHistoryWriter'):
        data = (service/name).read_bytes(); assert data[:4] == bytes.fromhex('000003f3'), name
        shutil.copy2(service/name, out/name)
    (out/'ISOLATED-QA-ONLY').touch()
    (out/'test.json').write_text(json.dumps({'application': 'QATests:AmiNotisPluginQA', 'expectedReturnCode': 0}))
    run = [sys.executable, 'tools/run_native_fixture.py', str(out), '--profile', a.profile, '--timeout', '240', '--hidden']
    if a.resources:
        run += ['--resources', str(a.resources)]
    result = subprocess.run(run, cwd=ROOT, env=env, capture_output=True, text=True)
    (out/'run.log').write_text(result.stdout+result.stderr); print(result.stdout+result.stderr, end='')
    bases = [x for x in result.stdout.splitlines() if x.startswith('/private/tmp/nd-native-latest-')]
    native = ''
    if bases and (Path(bases[0])/'AutoRun/launch.log').exists():
        shutil.copy2(Path(bases[0])/'AutoRun/launch.log', out/'native.log'); native = (out/'native.log').read_text(errors='replace')
    passed = result.returncode == 0 and 'PASS AMINOTIS native service checks' in native
    (out/'report.json').write_text(json.dumps({'status': 'passed' if passed else 'failed', 'profile': a.profile,
                                               'plugin': str(ROOT/'Plugins/AmiNotis'), 'service': str(service)}, indent=2)+'\n')
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
