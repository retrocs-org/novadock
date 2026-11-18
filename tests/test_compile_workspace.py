#!/usr/bin/env python3
"""Cross-builds leave source files untouched on success and compiler failure."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('compile_amiga', ROOT/'scripts/compile-amiga.py')
compile_amiga = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compile_amiga)


class WorkspaceTests(unittest.TestCase):
    def fixture(self, root):
        (root/'src').mkdir(); (root/'include').mkdir()
        (root/'src/local.h').write_text('#define LOCAL 17\n')
        (root/'include/shared.h').write_text('#define SHARED 25\n')
        (root/'src/main.c').write_text('#include "local.h"\n#include "shared.h"\nint main(void){return LOCAL+SHARED==42 ? 0:1;}\n')
        (root/'src/hooks.asm').write_text('; handwritten assembly\n')
        (root/'bin').mkdir()

    def test_native_host_compile_preserves_quoted_headers_and_paths(self):
        compiler = shutil.which('cc')
        self.assertTrue(compiler)
        with tempfile.TemporaryDirectory(prefix='novadock compile ') as tmp:
            root = Path(tmp); self.fixture(root)
            result = compile_amiga.compile_command(compiler,
                ['-Iinclude', '-o', 'bin/check', 'src/main.c'], root)
            self.assertEqual(result, 0)
            subprocess.run([str(root/'bin/check')], check=True)
            self.assertFalse((root/'src/main.asm').exists())
            self.assertFalse(list((root/'build/intermediates').iterdir()))

    def test_vbcc_style_sidecars_stay_isolated_and_failure_cleans_up(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); self.fixture(root)
            compiler = root/'fake.py'
            compiler.write_text('''import sys
from pathlib import Path
args=sys.argv[1:]
for value in args:
 if value.endswith(('.c','.asm')):
  p=Path(value);assert 'build/intermediates/compile-' in str(p)
  if p.suffix=='.c':
   assert p.read_text().startswith('#include "')
   p.with_suffix('.asm').write_text('compiler output')
  else:assert p.read_text()=='; handwritten assembly\\n'
  p.with_suffix('.o').write_bytes(b'object')
if '-DFAIL' in args:sys.exit(7)
Path(args[args.index('-o')+1]).write_bytes(b'linked')
''')
            command = subprocess.list2cmdline([sys.executable, str(compiler)])
            arguments = ['-o', 'bin/check', 'src/main.c', 'src/hooks.asm']
            for failure in (False, True):
                result = compile_amiga.compile_command(command, arguments+(['-DFAIL'] if failure else []), root)
                self.assertEqual(result, 7 if failure else 0)
                self.assertEqual((root/'src/hooks.asm').read_text(), '; handwritten assembly\n')
                self.assertFalse((root/'src/main.asm').exists())
                self.assertFalse(list(root.rglob('*.o')))
                self.assertFalse(list((root/'build/intermediates').iterdir()))

    def test_git_tracks_only_handwritten_assembly(self):
        repository = subprocess.run(['git', 'rev-parse', '--show-toplevel'], cwd=ROOT, capture_output=True, text=True)
        if repository.returncode or Path(repository.stdout.strip()).resolve() != ROOT:
            self.skipTest('Git inventory applies only to a repository checkout')
        make = (ROOT/'Makefile').read_text()
        tracked = subprocess.check_output(['git', 'ls-files', '*.asm'], cwd=ROOT, text=True).splitlines()
        for path in tracked:
            self.assertIn(path, make)
            self.assertNotIn('.c"', (ROOT/path).read_text().splitlines()[0])


if __name__ == '__main__': unittest.main()
