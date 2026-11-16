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
               'plugin_src/screenmode/screenmode.c','plugin_src/aminet/nd_aminet_ui.h'):
   with self.subTest(window=name):
    source=(ROOT/name).read_text()
    start=source.index('CreateGadget(LISTVIEW_KIND')
    flags=re.search(r'WA_IDCMP,([^,]+),',source[start:]).group(1)
    self.assertIn('LISTVIEWIDCMP',flags)
    self.assertIn('IDCMP_CLOSEWINDOW',flags)
    self.assertIn('IDCMP_REFRESHWINDOW',flags)
 def test_settings_scroller_is_vertical_and_reports_release(self):
  source=(ROOT/'prefs/novadockpluginprefs.c').read_text()
  start=source.index('CreateGadget(SCROLLER_KIND')
  tags=source[start:source.index('TAG_DONE)',start)]
  self.assertIn('PGA_Freedom,LORIENT_VERT',tags)
  self.assertNotIn('FREEVERT',tags)
  self.assertIn('GA_RelVerify,TRUE',tags)
  self.assertIn('GTSC_Arrows,nd_prefs_layout_h',tags)
if __name__=='__main__':unittest.main()
