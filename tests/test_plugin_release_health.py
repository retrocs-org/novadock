#!/usr/bin/env python3
"""Release gate rejects broken packages; Amiga missing-file errors are optional."""
import contextlib
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import check_plugin_health as health


class PluginHealthTests(unittest.TestCase):
    def test_native_missing_file_error_policy(self):
        # Exercise the real Amiga branch even when stdio leaves errno unset.
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp)
            (p/'dos').mkdir(); (p/'proto').mkdir()
            (p/'dos/dos.h').write_text('typedef long BPTR; typedef long LONG; typedef char *STRPTR;\n#define ACCESS_READ -2\n#define ERROR_OBJECT_NOT_FOUND 205\n#define ERROR_DIR_NOT_FOUND 204\n')
            (p/'proto/dos.h').write_text('BPTR Lock(STRPTR p,long access); void UnLock(BPTR p); LONG IoErr(void);\n')
            source = r'''
#include <assert.h>
#include "nd_artwork.h"
static long error;
static int present;
BPTR Lock(STRPTR p,long access){(void)p;(void)access;return present;}
void UnLock(BPTR p){(void)p;}
LONG IoErr(void){return error;}
static FILE *failed_open(const char *p,const char *mode){(void)p;(void)mode;errno=0;return NULL;}
'''
            # Only the health function's fopen is replaced. Use its production body.
            header = (ROOT/'include/nd_artwork.h').read_text()
            start = header.index('static int nd_artwork_package_problem(')
            end = header.index('/* A reserved caption', start)
            source += '#define fopen failed_open\n#define nd_artwork_package_problem amigalibc_health\n'+header[start:end]+r'''
int main(void){char why[128];
 error=205;assert(!amigalibc_health("Plugins/Network","meter,graph",why,sizeof(why)));
 error=204;assert(!amigalibc_health("Plugins/Network","meter,graph",why,sizeof(why)));
 assert(amigalibc_health("Plugins/HASS","artwork",why,sizeof(why))&&strstr(why,"missing"));
 error=222;assert(amigalibc_health("Plugins/Network","meter",why,sizeof(why))&&strstr(why,"cannot be read"));
 present=1;error=205;assert(amigalibc_health("Plugins/Network","meter",why,sizeof(why)));
 return 0;}
'''
            (p/'probe.c').write_text(source)
            subprocess.run([os.environ.get('CC', 'cc'), '-std=c89', '-DAMIGA',
                            '-I'+str(p), '-I'+str(ROOT/'include'), str(p/'probe.c'),
                            '-o', str(p/'probe')], check=True)
            subprocess.run([str(p/'probe')], check=True)

    def test_extracted_package_gate(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, drawer = Path(tmp)/'source', Path(tmp)/'extracted'
            for base in (source, drawer):
                (base/'Plugins').mkdir(parents=True)
            manifest = b'NOVADOCK_ID=NET\nNOVADOCK_NAME=Network\nNOVADOCK_PROGRAM=Network\nNOVADOCK_FEATURES=meter,graph\n'
            for base in (source, drawer):
                (base/'Plugins/Network.ndplugin').write_bytes(manifest)
                (base/'Plugins/Network').write_bytes(bytes.fromhex('000003f3'))
            self.assertEqual(health.check_plugins(drawer, source), 1)
            bank = drawer/'Plugins/Network.assets/Artwork.ndart'
            bank.parent.mkdir(); bank.write_bytes(b'broken')
            with self.assertRaises(subprocess.CalledProcessError): health.check_plugins(drawer, source)
            bank.unlink(); bank.parent.rmdir()
            for base in (source, drawer):
                (base/'Plugins/Network.ndplugin').write_bytes(manifest+b'NOVADOCK_FEATURES=artwork\n')
            with self.assertRaises(subprocess.CalledProcessError): health.check_plugins(drawer, source)
            for base in (source, drawer):
                (base/'Plugins/Network.ndplugin').write_bytes(manifest+b'NOVADOCK_ABI=9\n')
            with self.assertRaises(subprocess.CalledProcessError): health.check_plugins(drawer, source)
            for base in (source, drawer):
                (base/'Plugins/Network.ndplugin').write_bytes(manifest)
                (base/'Plugins/Other.ndplugin').write_bytes(manifest)
                (base/'Plugins/Other').write_bytes(bytes.fromhex('000003f3'))
            with self.assertRaises(subprocess.CalledProcessError): health.check_plugins(drawer, source)
            (drawer/'Plugins/Other.ndplugin').unlink()
            with self.assertRaisesRegex(ValueError, 'inventory'): health.check_plugins(drawer, source)
            (source/'Plugins/Other.ndplugin').unlink()
            (drawer/'Plugins/Network').unlink()
            with self.assertRaisesRegex(ValueError, 'program'): health.check_plugins(drawer, source)
            (drawer/'Plugins/Network').write_bytes(b'bad executable')
            with self.assertRaises(subprocess.CalledProcessError): health.check_plugins(drawer, source)

    def test_release_runs_gate_before_publishing_archive(self):
        script = (ROOT/'scripts/make-release.sh').read_text()
        gate = script.index('tools/check_plugin_health.py')
        self.assertLess(script.index('"$lha" -x'), gate)
        self.assertLess(gate, script.index('mv -f "$archive"'))


if __name__ == '__main__':
    unittest.main()
