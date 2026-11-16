#!/usr/bin/env python3
"""Offline checks for synchronized release descriptions and Aminet readmes."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('notes', ROOT/'tools/release_notes.py')
n = importlib.util.module_from_spec(spec); spec.loader.exec_module(n)
spec = importlib.util.spec_from_file_location('credentials', ROOT/'tools/release_credentials.py')
c = importlib.util.module_from_spec(spec); spec.loader.exec_module(c)


class ReleaseNotesTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root/'VERSION').write_text('VERSION=0.2\n')
        (self.root/'Makefile').write_text('all: NovaDock Plugins/Weather Plugins/RunningApps\n')
        (self.root/'CHANGELOG.md').write_text('''# Changelog

## NovaDock 0.2.1

- Future version only.

## NovaDock 0.2

### Features

- Weather and **Notes**.
- Multiple-file archives with Workbench icons.

### Enhancements

- Faster selection
  while dragging.

### Bug fixes

- Settings survive restart.

## NovaDock 0.1

- Initial version only.
''')

    def test_exact_current_section_and_normalized_groups(self):
        groups = n.changes(self.root)
        self.assertEqual(groups['Enhancements'], ['Faster selection while dragging.'])
        self.assertEqual(groups['Bugs'], ['Settings survive restart.'])
        text = n.markdown(self.root)
        self.assertNotIn('Future version only', text)
        self.assertNotIn('Initial version only', text)
        self.assertIn('### Bugs', text)
        n.validate_notes(text, self.root)

    def test_incomplete_extra_headings_or_unfinished_notes_rejected(self):
        original = n.markdown(self.root)
        for text in (original.replace('- Settings survive restart.\n', ''),
                     '# Version Changelog\n\n'+original,
                     '## Version 0.2\n\n'+original,
                     original+'\nIn development\n'):
            with self.assertRaises(ValueError): n.validate_notes(text, self.root)
        text = (self.root/'CHANGELOG.md').read_text()
        (self.root/'CHANGELOG.md').write_text(text.replace('## NovaDock 0.2\n', '## NovaDock 0.2 (In development)\n'))
        with self.assertRaisesRegex(ValueError, 'unfinished'): n.markdown(self.root)

    def test_custom_tag_and_first_public_features_only(self):
        (self.root/'VERSION').write_text('VERSION=0.1\n')
        text = n.markdown(self.root, 'review/0.1')
        self.assertTrue(text.startswith('### Features\n'))
        self.assertEqual(text,n.markdown(self.root))
        self.assertNotIn('### Bugs', text)
        self.assertNotIn('### Enhancements', text)
        self.assertIn('### Features', text)
        n.validate_notes(text, self.root, 'review/0.1')

    def test_repository_notes_only_contain_current_changelog(self):
        text = n.markdown(self.root)
        self.assertTrue(text.startswith('### Features\n'))
        for excluded in ('Version Changelog','## Version','## Installation','Source and Licence','Bundled plugins:',n.DESCRIPTION):
            self.assertNotIn(excluded,text)
        with self.assertRaises(ValueError): n.validate_notes(text+'\nApplication overview\n',self.root)

    def test_plain_aminet_layout_contains_every_current_change(self):
        text = n.aminet(self.root, 'maintainer@example.org')
        text.encode('ascii')
        self.assertTrue(all(len(line) <= 76 for line in text.splitlines()))
        self.assertIn('Unpack novadock.lha', text)
        self.assertNotIn('NovaDock-0.2.lha', text)
        positions = [text.index('\n'+title+'\n') for title in
                     ('FEATURES', 'INSTALLATION', 'SOURCE AND LICENCE', 'VERSION CHANGELOG')]
        self.assertEqual(positions, sorted(positions))
        compact = ' '.join(text.split())
        for entries in n.changes(self.root).values():
            for entry in entries: self.assertIn(n.plain(entry), compact)
        self.assertNotIn('**', text)
        self.assertNotIn('Future version only', text)
        self.assertNotIn('Initial version only', text)

    def test_bad_contact_or_header_injection_rejected(self):
        for uploader, author in (('', 'Contributors'), ('placeholder', 'Contributors'),
                                 ('x@example.org\nType: invalid', 'Contributors'),
                                 ('x@example.org', 'Contributors\nType: invalid')):
            with self.assertRaises(ValueError): n.aminet(self.root, uploader, author)

    def test_explicit_empty_contact_is_rejected_with_release_environment(self):
        from unittest.mock import patch
        with patch.dict(n.os.environ, {'UPLOADER':'release@example.org','AUTHOR':'Contributors'}):
            for uploader,author in (('', 'Contributors'), ('release@example.org','')):
                with self.assertRaises(ValueError): n.aminet(self.root,uploader,author)
            self.assertIn('Uploader:     release@example.org',n.aminet(self.root))

    def test_committed_notes_and_real_plugin_inventory(self):
        self.assertEqual((ROOT/'docs/RELEASE-NOTES.md').read_text(), n.markdown(ROOT))
        names = n.plugin_names(ROOT)
        self.assertEqual(len(names), 32)
        for name in ('Weather', 'Notes', 'Timer', 'SCREENS', 'SHOT', 'SMODE', 'ARCHIVE', 'MUSIC', 'SENSOR', 'ACTIONS'):
            self.assertIn(name, names)
        self.assertIn('LumiAMP', n.aminet(ROOT, 'maintainer@example.org'))
        self.assertNotIn('Christmas', names)
        self.assertNotIn('Christmas tree', n.markdown(ROOT))
        self.assertNotIn('Christmas tree', n.aminet(ROOT, 'maintainer@example.org'))
        subprocess.run(['python3', str(ROOT/'tools/release_notes.py'), '--check'], check=True)

    def test_aminet_formatter_preserves_complete_generated_changes(self):
        path = self.root/'NovaDock.readme'
        path.write_text(n.aminet(self.root, 'maintainer@example.org'), encoding='ascii')
        subprocess.run(['python3', str(ROOT/'tools/aminet_release.py'),
                        'format-readme', str(path)], check=True)
        formatted = path.read_text('ascii')
        self.assertTrue(all(len(line) <= 76 for line in formatted.splitlines()))
        for entries in n.changes(self.root).values():
            for entry in entries:
                self.assertIn(n.plain(entry), ' '.join(formatted.split()))
        self.assertIn('    - Settings survive restart.', formatted)

    def test_credential_lookup_forms_must_be_unambiguous(self):
        with self.assertRaisesRegex(ValueError, 'not both'):
            c.validate({'github': {'provider': '1password',
                                  'reference': 'op://vault/item/field', 'vault': 'different'}})

    def test_detect_common_secrets_without_printing_values(self):
        for secret in ('gh'+'p_'+'A'*36, 'github_'+'pat_'+'A'*40,
                       'gl'+'pat-'+'a'*24, '-----BEGIN RSA '+'PRIVATE KEY-----',
                       '-----BEGIN OPENSSH '+'PRIVATE KEY-----'):
            self.assertTrue(c.contains_secret(secret.encode()))
        self.assertFalse(c.contains_secret(b'GITHUB_TOKEN and op://vault/item/field are references.'))


if __name__ == '__main__': unittest.main()
