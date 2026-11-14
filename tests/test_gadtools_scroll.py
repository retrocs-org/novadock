#!/usr/bin/env python3
"""GadTools' list arrows need the complete LISTVIEWIDCMP event contract."""
from pathlib import Path
import re,unittest
ROOT=Path(__file__).resolve().parents[1]
class ScrollEvents(unittest.TestCase):
 def test_list_windows_receive_arrow_and_repeat_events(self):
  for name in ('prefs/novadockadditemprefs.c','prefs/novadocklayoutprefs.c',
               'prefs/novadocklauncherprefs.c','prefs/novadockdiagnosticsprefs.c',
               'prefs/novadockitemprefs.c','prefs/novadockmanagerprefs.c',
               'plugin_src/clipboard/clipboard.c','plugin_src/games/games.c',
               'plugin_src/screenmode/screenmode.c'):
   with self.subTest(window=name):
    source=(ROOT/name).read_text()
    start=source.index('CreateGadget(LISTVIEW_KIND')
    flags=re.search(r'WA_IDCMP,([^,]+),',source[start:]).group(1)
    self.assertIn('LISTVIEWIDCMP',flags)
    self.assertIn('IDCMP_CLOSEWINDOW',flags)
    self.assertIn('IDCMP_REFRESHWINDOW',flags)
if __name__=='__main__':unittest.main()
