#!/usr/bin/env python3
"""AMINOTIS: manifest, artwork states, SDK snapshot and non-blocking design."""
import hashlib, re, struct, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT/'plugin_src/aminotis'
# AmiNotis v2 SDK files copied unchanged from the AmiNotis project.
SDK = {
    'aminotis.h': '26eb5dd69e4fd32ec99557b827e86e2fec10ba7c1ab5eaf81095b410ec3e874d',
    'aminotis.c': '55e92bbbbcbc0ed07360ca9f0046d2686ded2089e032b86e704b1303ce1a0849',
    'aminotis_wire.h': '6218db306919d75109b04932825acf5a2cd0fa1a8db6ddefda1e9494da1b93b1',
}


def main():
    source = (SRC/'aminotis.c').read_text()
    manifest = (ROOT/'Plugins/AmiNotis.ndplugin').read_text()
    fields = dict(line.split('=', 1) for line in manifest.splitlines() if '=' in line and not line.startswith('NOVADOCK_OPTION'))
    assert fields['NOVADOCK_ID'] == 'AMINOTIS' and fields['NOVADOCK_PROGRAM'] == 'AmiNotis'
    assert set(fields['NOVADOCK_FEATURES'].split(',')) >= {'click', 'popup', 'badge', 'artwork'}
    versions = (ROOT/'include/nd_plugin_versions.h').read_text()
    assert '#define ND_PLUGIN_VERSION_AMINOTIS "%s"' % fields['NOVADOCK_VERSION'] in versions

    # Every option the program reads is declared with the same choices and default.
    options = {o.split('|')[0]: o.split('|') for o in re.findall(r'^NOVADOCK_OPTION=(.+)$', manifest, re.M)}
    assert options['CLICK'][2:] == ['CHOICE', 'PANEL,INBOX', 'PANEL']
    assert options['COUNT'][2:] == ['CHOICE', 'BADGE,UNREAD', 'BADGE']
    assert options['BEEP'][2:] == ['BOOL', '', 'NO']
    for key, default in (('CLICK', 'PANEL'), ('BEEP', 'NO'), ('COUNT', 'BADGE')):
        assert re.search(r'key="%s";options\[\d\]\.fallback="%s"' % (key, default), source), key
    assert '"INBOX"' in source and '"UNREAD"' in source and '"YES"' in source

    # The bank is generated, current, and covers every state the program sends.
    subprocess.run([sys.executable, str(ROOT/'tools/generate_aminotis_art.py'), '--check'], check=True,
                   stdout=subprocess.DEVNULL)
    sys.path.insert(0, str(ROOT/'tools'))
    from check_plugin_art import check_bank  # noqa: E402
    bank = ROOT/'Plugins/AmiNotis.assets/Artwork.ndart'
    check_bank(bank)
    data = bank.read_bytes()
    header = 72 if data[5] == 2 else 56
    banked = set()
    for i in range(struct.unpack_from('>H', data, 6)[0]):
        name, state = struct.unpack_from('>24sB', data, header+i*32)
        banked.add((name.split(b'\0', 1)[0].decode(), state))
    states = {int(v) for v in re.findall(r'#define ART_[A-Z]+ (\d+)U', source)}
    assert states == {0, 1, 2, 3}, states
    assert '"aminotis",art' in source
    for state in states:
        assert ('aminotis', state) in banked, state
    assert re.search(r'ART_OFFLINE 3U', source), 'offline uses the sad face'

    # SDK snapshot is unchanged and the dock-side copy stays independent.
    for name, digest in SDK.items():
        assert hashlib.sha256((SRC/'sdk'/name).read_bytes()).hexdigest() == digest, name
    upstream = ROOT.parent/'AmiNotis/SDK'
    if upstream.is_dir():
        for name in SDK:
            if (upstream/name).read_bytes() != (SRC/'sdk'/name).read_bytes():
                print('note: AmiNotis SDK %s changed upstream; refresh plugin_src/aminotis/sdk' % name)
    wrapper = (SRC/'nd_aminotis_sdk.c').read_text()
    assert '#include "nd_plugin_resources.h"' in wrapper and '#include "sdk/aminotis.c"' in wrapper

    # Dock clients must never block on the service.
    assert 'an_call' not in source
    assert 'nd_client_wait_signals' in source and 'client_signal(&watcher)' in source
    assert 'AN_WATCH' in source and 'an_watch_resync' in source
    assert re.search(r'nd_client_update_art_quiet\([^;]*,elapsed,250\)', source)
    for makefile in ('Makefile', 'Makefile.cross'):
        text = (ROOT/makefile).read_text()
        assert re.search(r'^all:.*Plugins/AmiNotis', text, re.M | re.S), makefile
        assert 'plugin_src/aminotis/nd_aminotis_sdk.c' in text and 'plugin_src/aminotis/sdk/aminotis.c plugin' not in text.split('\t')[0]
    print('AMINOTIS manifest, %d artwork states, SDK snapshot and asynchronous design checked.' % len(states))


if __name__ == '__main__':
    main()
