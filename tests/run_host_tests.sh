#!/bin/sh
set -eu
PYTHON=${PYTHON:-python3}
export PYTHON
cd "$(dirname "$0")/.."
trap 'rm -f tests/nd_appscan_test tests/nd_bitskim_test tests/nd_interface_list_test tests/interface-choice.tmp tests/nd_netcontrol_test tests/net-interface.tmp tests/nd_dock_all.c tests/nd_package_install_test tests/nd_plugin_info_test tests/nd_text_pool_test tests/nd_text_pool_test.cfg tests/nd_text_pool_test.cfg.bak tests/nd_text_pool_test.cfg.old tests/nd_text_pool_test.cfg.good tests/nd_layout_enabled_test tests/nd_startinstaller_test tests/nd_amirc_test tests/nd_save_policy_test tests/nd_config_test tests/nd_theme_test tests/nd_network_test tests/nd_layout_test tests/nd_layout_profiles_test tests/nd_graph_test tests/nd_prefs_test tests/nd_prefs_layout_test tests/nd_roadshow_test tests/nd_musicplayer_test tests/nd_ilbm_test tests/nd_icon_mask_test tests/nd_launchers_test tests/nd_drag_menu_test tests/nd_interaction_test tests/nd_autohide_test tests/nd_unified_test tests/nd_eyes_test tests/nd_metrics_test tests/nd_watchdog_test tests/nd_archive_test tests/nd_fixes_test tests/nd_fixes_test.cfg tests/nd_fixes_test.cfg.bak tests/nd_media_test tests/nd_media_test.cfg tests/nd_media_test.cfg.bak tests/nd_manifest_test tests/nd_manifest_test.cfg tests/nd_manifest_test.cfg.bak tests/nd_tasks_test tests/nd_tasks_test.cfg tests/nd_tasks_test.cfg.bak tests/nd_command_test tests/nd_command_test.cfg tests/nd_command_test.cfg.bak tests/nd_folder_test tests/.drag_menu_test.cfg.good tests/eyes-roundtrip.cfg.good tests/hotkey.cfg.bak tests/hotkey.cfg.good tests/nd_command_test.cfg.good tests/nd_fixes_test.cfg.good tests/nd_launcher_roundtrip.cfg.bak tests/nd_launcher_roundtrip.cfg.good tests/nd_manifest_test.cfg.good tests/nd_media_test.cfg.good tests/nd_prefs_roundtrip.cfg.good tests/nd_tasks_test.cfg.good tests/unified-test.cfg.bak tests/unified-test.cfg.good' EXIT HUP INT TERM
# CC picks the host compiler; SANITIZE=1 adds AddressSanitizer and
# UndefinedBehaviorSanitizer to every test program.
CC=${CC:-cc}
# The Amiga target intentionally supports C89 formatting APIs. Apple's host SDK
# deprecates these independently of the caller's bounds checks. Keep other host
# diagnostics and sanitizer checks enabled; do not change native target flags.
case "$(uname -s)" in
  Darwin) CC="$CC -Wno-deprecated-declarations" ;;
esac
if [ "${SANITIZE:-0}" = 1 ]; then
  CC="$CC -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined"
  ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=0}; export ASAN_OPTIONS
fi
# Lifecycle and fallback regressions run in a private temporary directory.
REGRESSION_TMP=$(mktemp -d "${TMPDIR:-/tmp}/nd-regressions.XXXXXX")
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude tests/test_reply.c -o "$REGRESSION_TMP/reply"
"$REGRESSION_TMP/reply"
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude tests/test_calendar.c -o "$REGRESSION_TMP/calendar"
"$REGRESSION_TMP/calendar"
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude tests/test_config_fallback.c src/config.c src/nd_options.c plugin_src/common/nd_option_runtime.c -o "$REGRESSION_TMP/config"
mkdir -p "$REGRESSION_TMP/ENV:NovaDock"
(cd "$REGRESSION_TMP" && ./config)
rm -rf "$REGRESSION_TMP"
$CC -std=c89 -Wall -Wextra -pedantic tests/test_mode_worker.c -o tests/nd_mode_worker_test
./tests/nd_mode_worker_test
rm -f tests/nd_mode_worker_test
$CC -std=c89 -Wall -Wextra -pedantic tests/test_prefsave.c -o tests/nd_prefsave_test
./tests/nd_prefsave_test
rm -f tests/nd_prefsave_test
$CC -std=c89 -Wall -Wextra -pedantic tests/test_bitskim_requests.c -o tests/nd_bitskim_requests_test
./tests/nd_bitskim_requests_test
rm -f tests/nd_bitskim_requests_test
$CC -std=c89 -Wall -Wextra -pedantic tests/test_appscan.c -o tests/nd_appscan_test
./tests/nd_appscan_test
$CC -std=c89 -Wall -Wextra -pedantic plugin_src/bitskim/nd_bitskim.c tests/test_bitskim.c -o tests/nd_bitskim_test
./tests/nd_bitskim_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude tests/test_interface_list.c -o tests/nd_interface_list_test
./tests/nd_interface_list_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude -Iplugin_src/network \
  plugin_src/network/nd_network.c plugin_src/network/nd_netcontrol.c plugin_src/network/nd_roadshow.c src/config.c src/nd_options.c plugin_src/common/nd_option_runtime.c tests/test_netcontrol.c -o tests/nd_netcontrol_test
./tests/nd_netcontrol_test
rm -f tests/nd_netcontrol_test
$CC -std=c89 -Wall -Wextra -pedantic -Iplugin_src/clipboard \
  plugin_src/clipboard/nd_clipboard.c tests/test_clipboard.c -o tests/nd_clipboard_test
./tests/nd_clipboard_test
rm -f tests/nd_clipboard_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c tests/test_config.c -o tests/nd_config_test
./tests/nd_config_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/theme.c tests/test_theme.c -o tests/nd_theme_test
./tests/nd_theme_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  plugin_src/network/nd_network.c tests/test_network.c -o tests/nd_network_test
./tests/nd_network_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c src/layout.c tests/test_layout.c -o tests/nd_layout_test
./tests/nd_layout_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c src/layout.c tests/test_layout_enabled.c \
  -o tests/nd_layout_enabled_test
./tests/nd_layout_enabled_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c tests/test_text_pool.c -o tests/nd_text_pool_test
./tests/nd_text_pool_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/nd_manifest.c src/config.c src/nd_options.c tests/test_plugin_info.c \
  -o tests/nd_plugin_info_test
./tests/nd_plugin_info_test
$CC -std=c89 -Wall -Wextra -pedantic -DND_PACKAGE_TEST -Iinclude \
  prefs/nd_package_install.c tests/test_package_install.c -o tests/nd_package_install_test
./tests/nd_package_install_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude tests/test_startinstaller.c -o tests/nd_startinstaller_test
./tests/nd_startinstaller_test
if [ -f NovaDock ]; then
  "$PYTHON" tests/test_installer_script.py
else
  "$PYTHON" tests/test_installer_script.py --source-only
fi
"$PYTHON" tests/test_amigaguide.py
"$PYTHON" tests/test_catalogs.py
"$PYTHON" tests/test_build_scripts.py
"$PYTHON" tools/make_deps.py
"$PYTHON" tests/test_aminet_release.py
"$PYTHON" tests/test_release_tool.py
"$PYTHON" tests/test_release_provenance.py
"$PYTHON" tests/test_release_integration.py
"$PYTHON" tests/test_release_catalog.py
"$PYTHON" tests/test_graph_scroll.py
# Every program, tool and page agrees with the VERSION file.
"$PYTHON" tools/update_version.py --check
# A release needs a CHANGELOG section for its version.
"$PYTHON" tools/release_notes.py >/dev/null
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c src/layout_profiles.c tests/test_layout_profiles.c \
  -o tests/nd_layout_profiles_test
./tests/nd_layout_profiles_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/graph.c tests/test_graph.c -o tests/nd_graph_test
./tests/nd_graph_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c src/layout.c tests/test_prefs.c -o tests/nd_prefs_test
./tests/nd_prefs_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/prefs_layout.c tests/test_prefs_layout.c -o tests/nd_prefs_layout_test
./tests/nd_prefs_layout_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  plugin_src/network/nd_roadshow.c tests/test_roadshow.c -o tests/nd_roadshow_test
./tests/nd_roadshow_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  plugin_src/musicplayer/nd_musicplayer.c tests/test_musicplayer.c -o tests/nd_musicplayer_test
./tests/nd_musicplayer_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/ilbm.c tests/test_ilbm.c -o tests/nd_ilbm_test
./tests/nd_ilbm_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/icon_mask.c tests/test_icon_mask.c -o tests/nd_icon_mask_test
./tests/nd_icon_mask_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c tests/test_launchers.c -o tests/nd_launchers_test
./tests/nd_launchers_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c tests/test_drag_and_menu.c -o tests/nd_drag_menu_test
./tests/nd_drag_menu_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude -Iplugin_src/script src/config.c \
  src/nd_options.c src/interaction.c plugin_src/script/nd_script.c \
  tests/test_script_hooks.c -o tests/nd_script_hooks_test
./tests/nd_script_hooks_test
rm -f tests/nd_script_hooks_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude -Iplugin_src/mixer -Iplugin_src/screenshot \
  -Iplugin_src/screenmode plugin_src/mixer/nd_ahiprefs.c plugin_src/screenshot/nd_shotfile.c \
  plugin_src/screenmode/nd_scrmode.c src/ilbm.c tests/test_newplugins.c -o tests/nd_newplugins_test
./tests/nd_newplugins_test
rm -f tests/nd_newplugins_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude src/interaction.c tests/test_interaction.c -o tests/nd_interaction_test
./tests/nd_interaction_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c src/layout.c src/autohide.c tests/test_autohide.c -o tests/nd_autohide_test
./tests/nd_autohide_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude -Iplugin_src/common \
  src/config.c src/nd_options.c plugin_src/common/nd_option_runtime.c \
  plugin_src/network/nd_network.c plugin_src/network/nd_roadshow.c \
  plugin_src/musicplayer/nd_musicplayer.c tests/test_unified.c -o tests/nd_unified_test
./tests/nd_unified_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/eyes.c src/config.c src/nd_options.c tests/test_eyes.c -o tests/nd_eyes_test
./tests/nd_eyes_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/metrics.c tests/test_metrics.c -o tests/nd_metrics_test
./tests/nd_metrics_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/watchdog.c tests/test_watchdog.c -o tests/nd_watchdog_test
./tests/nd_watchdog_test

$CC -std=c89 -Wall -Wextra -pedantic -Iplugin_src/archive \
  plugin_src/archive/nd_archive.c tests/test_archive.c -o tests/nd_archive_test
./tests/nd_archive_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude -Iplugin_src/common \
  src/config.c src/nd_options.c plugin_src/common/nd_option_runtime.c \
  tests/test_fixes.c -o tests/nd_fixes_test
./tests/nd_fixes_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c src/nd_media.c tests/test_media.c \
  -o tests/nd_media_test
./tests/nd_media_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c src/nd_media.c src/nd_tasks.c tests/test_tasks.c \
  -o tests/nd_tasks_test
./tests/nd_tasks_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude -Iplugin_src/command \
  src/config.c src/nd_options.c plugin_src/command/nd_command.c \
  tests/test_command.c -o tests/nd_command_test
./tests/nd_command_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude src/nd_folder.c tests/test_folder.c \
  -o tests/nd_folder_test
./tests/nd_folder_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/config.c src/nd_options.c src/nd_manifest.c tests/test_manifest.c \
  -o tests/nd_manifest_test
./tests/nd_manifest_test
$CC -std=c89 -Wall -Wextra -pedantic -Iinclude -Iplugin_src/amirc \
  src/config.c src/nd_options.c src/nd_manifest.c plugin_src/amirc/nd_amirc.c \
  tests/test_amirc.c -o tests/nd_amirc_test
./tests/nd_amirc_test

$CC -std=c89 -Wall -Wextra -pedantic -Iinclude src/save_policy.c src/indicator.c \
  tests/test_save_policy.c -o tests/nd_save_policy_test
./tests/nd_save_policy_test

# The SDK example manifest must parse and merge cleanly.
cat > tests/nd_manifest_check.c <<'CEOF'
#include <stdio.h>
#include "nd_manifest.h"
int main(int argc,char **argv)
{
    struct NDManifest m; struct NDConfig cfg; char line[300]; FILE *f;
    if (argc<2 || !(f=fopen(argv[1],"r"))) return 1;
    nd_manifest_init(&m,"Hello");
    while (fgets(line,sizeof(line),f)) if (!nd_manifest_line(&m,line)) return 2;
    fclose(f);
    if (!nd_manifest_finish(&m)) { fprintf(stderr,"%s\n",m.error); return 3; }
    nd_default_config(&cfg);
    return nd_manifest_merge(&cfg,&m,"Plugins/Hello")==ND_MANIFEST_ADDED ? 0 : 4;
}
CEOF
$CC -std=c89 -Iinclude src/config.c src/nd_options.c src/nd_manifest.c \
  tests/nd_manifest_check.c -o tests/nd_manifest_check
./tests/nd_manifest_check plugin_src/template/Hello.ndplugin ||
  { echo "SDK Hello.ndplugin manifest is invalid." >&2; exit 1; }
rm -f tests/nd_manifest_check tests/nd_manifest_check.c

# Structural checks for review fixes that only exist in AmigaOS code paths.
fail_review() { echo "Review regression: $1" >&2; exit 1; }
# The dock is split over src/dock_internal.h, src/novadock.c and src/dock_*.c;
# review checks read
# them as one file, so counts and function ranges work as before.
DOCK_ALL=tests/nd_dock_all.c
cat src/dock_internal.h src/novadock.c src/dock_*.c > "$DOCK_ALL"
sed -n '/^\(static \)*int send_rexx_action/,/^}/p' "$DOCK_ALL" | grep -q 'WaitPort' &&
  fail_review "ARexx Launcher Actions must not wait synchronously."
grep -q 'process_rexx_replies(dock, 1)' "$DOCK_ALL" &&
  grep -q 'stop_rexx_replies(dock)' "$DOCK_ALL" ||
  fail_review "asynchronous ARexx replies must be collected and drained."
grep -q 'cli_CommandName' "$DOCK_ALL" ||
  fail_review "running-app detection must check CLI command names."
grep -q 'ScreenToFront(owner)' "$DOCK_ALL" ||
  fail_review "click-to-front must revalidate and raise the owning screen."
grep -q 'NP_StackSize,stack' "$DOCK_ALL" && grep -q '"Stack %lu' "$DOCK_ALL" ||
  fail_review "launches must set an explicit stack."
grep -q 'WLayer->front' "$DOCK_ALL" ||
  fail_review "Keep front must skip needless layer operations."
sed -n '/^\(static \)*int toggle_monitor_view/,/^}/p' "$DOCK_ALL" | grep -q '"ENVARC:' &&
  fail_review "CPU/RAM toggles must defer ENVARC writes."
grep -q 'ND_CONTROL_SHOW' "$DOCK_ALL" && grep -q 'COF_SHOW_HIDE' "$DOCK_ALL" ||
  fail_review "second launch and Exchange Show/Hide must reveal the dock."
grep -q 'Use defaults|Quit' "$DOCK_ALL" ||
  fail_review "an invalid configuration must offer built-in defaults."
grep -q 'nd_publish_config_source' "$DOCK_ALL" ||
  fail_review "the dock must publish the configuration source for plugins."
grep -q 'g->SpecialInfo' include/nd_numeric_gadget.h ||
  fail_review "Preferences numeric fields must read the native StringInfo buffer."
if [ "$(grep -c 'copy_complete_file' src/config.c)" -lt 4 ]; then
  fail_review "atomic configuration saves need the staged-copy fallback."
fi
for f in "$DOCK_ALL" prefs/novadockprefs.c plugin_src/clock/clock.c \
  plugin_src/eyes/eyes.c plugin_src/activity/activity.c plugin_src/memory/memory.c \
  plugin_src/network/network.c \
  plugin_src/musicplayer/musicplayer.c plugin_src/dopus4/dopus4.c \
  plugin_src/archive/archive.c plugin_src/runningapps/runningapps.c \
  tools/makeicon.c tools/setupstartup.c; do
  grep -q '^const char nd_version\[\] *= *ND_VER_STRING("' "$f" ||
    fail_review "$f needs a global \$VER string from ND_VER_STRING (VERSION)."
done
# Badges (ABI 4): older docks QUIT plugins on unknown commands, so the client
# may send ND_MSG_BADGE only after the dock advertised ND_EVENT_HOST_BADGES.
grep -q 'return c->host_badges ? send_badge(c) : ND_EVENT_NONE;' plugin_src/common/nd_client.c &&
  grep -q 'c->badge_pending && c->host_badges' plugin_src/common/nd_client.c ||
  fail_review "badges must wait for ND_EVENT_HOST_BADGES (older docks quit the plugin)."
[ "$(grep -c 'send_badge(c)' plugin_src/common/nd_client.c)" -eq 2 ] ||
  fail_review "ND_MSG_BADGE may only be sent from the two capability-gated paths."
grep -q 'm->event|=ND_EVENT_HOST_BADGES;' "$DOCK_ALL" ||
  fail_review "the dock must advertise badge support in its replies."
[ "$(grep -c "badge\[0\] *= *'\\\\0'" "$DOCK_ALL")" -ge 2 ] ||
  fail_review "goodbye and watchdog restarts must clear plugin badges."
sed -n '/^\(static \)*int scan_media/,/^}/p' "$DOCK_ALL" | grep -v DosList |
  grep -q 'Lock(' && fail_review "media scans must not Lock() volumes (requesters, floppy motors)."
sed -n '/^\(static \)*void refresh_volume_meters/,/^}/p' "$DOCK_ALL" |
  grep -q 'quiet_requesters' ||
  fail_review "volume meters must never raise insert-volume requesters."
grep -q 'IECLASS_DISKINSERTED' "$DOCK_ALL" && grep -q 'ND_MEDIA_SCAN_TICKS' "$DOCK_ALL" ||
  fail_review "removable media needs both disk events and a periodic rescan."
grep -q 'G_MEDIA,"Media:"' prefs/novadockprefs.c ||
  fail_review "Preferences must expose the removable-media switch."
grep -q 'nd_discover_plugins(session' prefs/novadockprefs.c ||
  fail_review "Preferences must discover self-describing plugins at startup."
sed -n '/^\(static \)*void about_field/,/^}/p' prefs/novadockaboutprefs.c |
  grep -q 'ng->ng_VisualInfo=p->vi' ||
  fail_review "the About section must pass visual info to every GadTools gadget."
grep -q "GA_Disabled,(ULONG)(text\[0\]=='>')" prefs/novadockaboutprefs.c ||
  fail_review "the active About sidebar button must use the shared disabled-selection style."
grep -q 'eyes_last_valid\[ND_MAX_ITEMS\]' "$DOCK_ALL" &&
  sed -n '/^\(static \)*void eyes_timer_tick/,/^}/p' "$DOCK_ALL" |
  grep -q 'for (index=0;index<dock->conf.count;++index)' ||
  fail_review "every Eyes instance needs independent pointer tracking and repainting."
if grep -q 'nd_tasks_append(&dock->conf' "$DOCK_ALL" ||
   grep -q '"Off", "Mark", "Task"' prefs/novadockappearanceprefs.c; then
  fail_review "unlisted applications must never become automatic dock tiles."
fi
grep -q '"Off", "Mark", NULL' prefs/novadockappearanceprefs.c ||
  fail_review "Appearance must offer only Off and Mark for launcher indicators."
grep -q 'cfg->running_section=0' src/config.c ||
  fail_review "legacy RUNNING_SECTION values must be normalised to off."
grep -q 'Plugins/RunningApps' Makefile &&
  grep -q 'Plugins/RunningApps' Makefile.cross &&
  grep -q 'Plugins/RunningApps' build.amiga &&
  grep -q 'Plugins/RunningApps' install.amiga &&
  grep -q 'collect_apps' plugin_src/runningapps/runningapps.c &&
  grep -q 'raise_app' plugin_src/runningapps/runningapps.c ||
  fail_review "the optional Running Apps popup plugin must be built and installed."
grep -q '"RUNNINGAPPS"' src/nd_manifest.c &&
  grep -q '"Running Apps","Plugins/RunningApps"' src/config.c ||
  fail_review "Running Apps must remain in the bundled disabled plugin catalogue."
grep -q 'load_active_theme' plugin_src/runningapps/runningapps.c &&
  grep -q 'draw_app_glyph' plugin_src/runningapps/runningapps.c &&
  grep -q 'running_apps_icon' "$DOCK_ALL" &&
  grep -q 'draw_running_apps_tile' "$DOCK_ALL" ||
  fail_review "Running Apps must use the themed launcher and icon-grid presentation."
sed -n '/^\(static \)*int plugin_popup_action/,/^}/p' "$DOCK_ALL" |
  grep -q 'char args\[80\]' ||
  fail_review "the plugin editor command buffer must fit the longest bundled plugin ID."
if grep -q 'A_ROWS\|appearance_rows\|g_rows' prefs/novadockappearanceprefs.c ||
   ! grep -q 'G_ROWS,"Dock rows:"' prefs/novadockitemprefs.c ||
   ! grep -q 'Dock Row %d' prefs/novadockitemprefs.c ||
   grep -q 'if (!row_items) continue' prefs/novadockitemprefs.c ||
   ! grep -q 'active_display_map' prefs/novadockitemprefs.c; then
  fail_review "Organize must show every configured row, including empty rows."
fi
grep -q 'G_ADD,"Add\.\.\."' prefs/novadockitemprefs.c &&
  grep -q 'p->result=ND_PREF_NAV_ADD_ITEM' prefs/novadockitemprefs.c &&
  grep -q 'result==ND_PREF_NAV_ADD_ITEM' prefs/novadockprefs.c &&
  grep -q 'return_from_add' prefs/novadockprefs.c ||
  fail_review "Organize Add must reuse Add Item and return to the refreshed dock list."
grep -q 'layout->rows=rows' src/layout.c ||
  fail_review "The dock must preserve every configured row, including empty rows."
grep -q 'SYS_Asynch,TRUE' plugin_src/command/command.c &&
  ! grep -q 'Delay(' plugin_src/command/command.c ||
  fail_review "the command tile must run commands asynchronously and wake on clicks."
grep -q 'Plugins/Command' install.amiga && grep -q 'Plugins/Command:' Makefile &&
  grep -q 'Plugins/Command' build.amiga ||
  fail_review "the Command plugin must be built and installed."
grep -q '"Add item..."' "$DOCK_ALL" &&
  grep -q 'PAGE=ADD FOCUSED=1' "$DOCK_ALL" &&
  grep -q 'nd_additem_prefs_run' prefs/novadockprefs.c ||
  fail_review "the dock menu must open the unified Add Item page."
grep -q 'A_TYPE_PLUGIN' prefs/novadockadditemprefs.c &&
  grep -q 'A_APP_KIND' prefs/novadockadditemprefs.c &&
  grep -q 'application_kinds\[\].*"Application","Command tile"' prefs/novadockadditemprefs.c &&
  grep -q 'LISTVIEW_KIND.*p->last' prefs/novadockadditemprefs.c &&
  grep -q 'GTLV_Labels.*plugin_rows' prefs/novadockadditemprefs.c &&
  grep -q 'GTLV_ShowSelected' prefs/novadockadditemprefs.c &&
  grep -q 'Drawers and volumes are always opened by Workbench' prefs/novadockadditemprefs.c &&
  grep -q 'A_ADD,"Add item"' prefs/novadockadditemprefs.c &&
  grep -q 'case A_ADD: commit_add(p,1)' prefs/novadockadditemprefs.c &&
  grep -q 'A_CANCEL,"Cancel"' prefs/novadockadditemprefs.c &&
  grep -q 'case A_CANCEL:p->done=1;p->result=0' prefs/novadockadditemprefs.c &&
  grep -q 'p->type!=ADD_PLUGIN.*ND_MAX_ITEMS' prefs/novadockadditemprefs.c &&
  grep -q 'nd_set_item_enabled' prefs/novadockadditemprefs.c ||
  fail_review "Add Item needs persistent Add/Cancel actions, a visibly selected plugin row, and a drawer-specific form."
if grep -q 'A_TYPE_COMMAND' prefs/novadockadditemprefs.c ||
   grep -q 'add_button.*"Command tile"' prefs/novadockadditemprefs.c; then
  fail_review "Command tiles must share the Application category and its Type selector."
fi
if grep -q 'A_USE\|A_SAVE' prefs/novadockadditemprefs.c; then
  fail_review "Add Item must not present the generic Use/Save button pair."
fi
for prefs_file in \
  prefs/novadockappearanceprefs.c \
  prefs/novadockprefs.c \
  prefs/novadockitemprefs.c \
  prefs/novadockpluginprefs.c; do
  if grep -q 'create_button.*"Cancel"\|button.*"Cancel"\|sp_button.*"Cancel"' "$prefs_file"; then
    fail_review "Preferences pages must use the window close gadget instead of Cancel buttons."
  fi
done
# Launcher editors explicitly offer Cancel with rollback of staged edits.
grep -q 'G_CANCEL,"Cancel"' prefs/novadocklauncherprefs.c ||
  fail_review "Launcher editing must offer Cancel."
grep -q 'create_button.*"Defaults"' prefs/novadockappearanceprefs.c &&
  fail_review "Appearance must keep the compact Revert/Test/Use/Save action row."
for prefs_file in \
  prefs/novadockappearanceprefs.c \
  prefs/novadockprefs.c \
  prefs/novadockitemprefs.c \
  prefs/novadocklauncherprefs.c \
  prefs/novadockpluginprefs.c \
  prefs/novadockdiagnosticsprefs.c; do
  grep -Eq '"(> )?Organize"' "$prefs_file" ||
    fail_review "Every Preferences page must use the Organize section name."
done
grep -q 'preview_normal_sample' prefs/novadockappearanceprefs.c &&
  grep -q 'preview_tiny_icon(p,x,y+2,w,h-18,2,p->cfg->running_apps)' prefs/novadockappearanceprefs.c &&
  grep -q 'preview_tiny_clock(p,x,y+10,w,h-12)' prefs/novadockappearanceprefs.c &&
  grep -q 'preview_meter(p,meter_x,y+h/2,meter_w,5,28)' prefs/novadockappearanceprefs.c &&
  grep -q 'preview_meter(p,meter_x,y+h-7,meter_w,7,61)' prefs/novadockappearanceprefs.c ||
  fail_review "Appearance preview must show launcher artwork, a clock and both RAM meters."
if grep -q '"[>] Application"\|"[>] Drawer / disk"\|"[>] Plugins"' \
  prefs/novadockadditemprefs.c; then
  fail_review "selected Add Item categories must not change their label width."
fi
sed -n '/^\(static \)*int read_folder/,/^}/p' "$DOCK_ALL" | grep -q 'quiet_requesters' &&
  sed -n '/^\(static \)*void folder_icons_tick/,/^}/p' "$DOCK_ALL" | grep -q 'quiet_requesters' ||
  fail_review "drawer popups must not raise insert-volume requesters."
grep -q 'IDCMP_VANILLAKEY' "$DOCK_ALL" && grep -q 'folder_type_ahead' "$DOCK_ALL" ||
  fail_review "drawer popups need keyboard navigation and type-ahead."
grep -q 'LayoutIconA\|DrawIconStateA' "$DOCK_ALL" &&
  fail_review "drawer popup icons must not remap the Workbench palette."
"$PYTHON" tools/generate_catalog.py --check
grep -q 'nd_client_wait' plugin_src/dopus4/dopus4.c ||
  fail_review "click-driven plugins must use the interruptible wait."

# Theme pens are deliberately owned so 8-bit Workbench screens render the
# configured colours instead of arbitrary nearest palette entries.  They must
# also be released. Icons still borrow existing pens. Smooth refresh may use
# SMART_REFRESH, but only as an explicit option; low-memory remains default.
if ! grep -q 'ObtainBestPenA' "$DOCK_ALL" ||
   ! grep -q 'ReleasePen' "$DOCK_ALL"; then
  echo "Theme-colour test failed: owned pens must be acquired and released." >&2
  exit 1
fi
if grep -Eq 'LayoutIconA|DrawIconStateA|GetIconRectangleA|PDTA_Remap,TRUE' "$DOCK_ALL"; then
  echo "Palette-safety test failed: icon remapping returned." >&2
  exit 1
fi
if ! grep -q 'ND_REFRESH_SMOOTH' "$DOCK_ALL" ||
   ! grep -q 'WFLG_SMART_REFRESH' "$DOCK_ALL" ||
   ! grep -q 'WFLG_SIMPLE_REFRESH' "$DOCK_ALL" ||
   ! grep -q 'refresh_mode = ND_REFRESH_LOW_MEMORY' src/config.c; then
  echo "Refresh-mode test failed: optional Smooth and safe default are required." >&2
  exit 1
fi
if ! grep -q '#define G_REFRESH 37' prefs/novadockprefs.c ||
   ! grep -q 'GTCY_Labels,(ULONG)refresh_modes' prefs/novadockprefs.c ||
   grep -q 'A_REFRESH' prefs/novadockappearanceprefs.c ||
   ! grep -q 'G_MATCH_TASK,"Task:"' prefs/novadocklauncherprefs.c ||
   ! grep -q 'G_MATCH_WINDOW,"Window:"' prefs/novadocklauncherprefs.c; then
  echo "Preferences layout test failed: refresh placement or compact launcher labels regressed." >&2
  exit 1
fi
if ! grep -q 'struct NDPrefsLayout layout' prefs/novadockprefs.c ||
   ! grep -q 'struct NDPrefsLayout layout' prefs/novadockappearanceprefs.c ||
   ! grep -q 'struct NDPrefsLayout layout' prefs/novadockitemprefs.c ||
   ! grep -q 'struct NDPrefsLayout layout' prefs/novadocklauncherprefs.c ||
   ! grep -q 'struct NDPrefsLayout layout' prefs/novadockpluginprefs.c ||
   ! grep -q 'struct NDPrefsLayout layout' prefs/novadockdiagnosticsprefs.c ||
   ! grep -q 'struct NDPrefsLayout layout' prefs/novadocklayoutprefs.c; then
  echo "Preferences layout test failed: every page must use the font-aware helper." >&2
  exit 1
fi
focused_plugin_editor=$(sed -n '/if (p->focused) {$/,/^    } else {$/p' \
  prefs/novadockpluginprefs.c)
if printf '%s\n' "$focused_plugin_editor" | grep -q '"Previous"\|"Next"' ||
   ! grep -q 'SCROLLER_KIND' prefs/novadockpluginprefs.c ||
   ! grep -q 'sprintf(p->title,"%s settings",p->id)' prefs/novadockpluginprefs.c; then
  echo "Focused item editor test failed: plugin settings must be item-specific without paging buttons." >&2
  exit 1
fi
if ! grep -q 'left=(screen->Width-width)/2' plugin_src/runningapps/runningapps.c ||
   ! grep -q 'screen->Height-screen->BarHeight-height' plugin_src/runningapps/runningapps.c ||
   ! grep -q 'This plugin has no configurable settings' prefs/novadockpluginprefs.c ||
   grep -q 'if (!p->count) return 0;' prefs/novadockpluginprefs.c; then
  echo "Running Apps test failed: popup centering and optionless plugin editing must stay safe." >&2
  exit 1
fi
if ! grep -q 'OpenWorkbenchObject' plugin_src/dopus4/dopus4.c ||
   ! grep -q 'number<=9' plugin_src/dopus4/dopus4.c ||
   ! grep -q 'dopus_hotkeez' plugin_src/dopus4/dopus4.c ||
   ! grep -q 'SIGBREAKF_CTRL_E' plugin_src/dopus4/dopus4.c ||
   ! grep -q 'dopus_visible' plugin_src/dopus4/dopus4.c ||
   ! grep -q 'dopus_frontmost' plugin_src/dopus4/dopus4.c ||
   ! grep -q 'LockPubScreen' plugin_src/dopus4/dopus4.c ||
   ! grep -q 'ScreenToFront' plugin_src/dopus4/dopus4.c ||
   ! grep -q 'status=visible ? "READY" : "ICONIFIED"' plugin_src/dopus4/dopus4.c; then
  echo "DOpus launch test failed: default launch and resident restore paths are required." >&2
  exit 1
fi
if ! grep -q '"DOPUS4","PROGRAM","Program path","FILE"' src/nd_options.c ||
   ! grep -q 'AllocAslRequestTags' prefs/novadockpluginprefs.c ||
   ! grep -q 'G_FILE_FIRST' prefs/novadockpluginprefs.c; then
  echo "DOpus settings test failed: Program path must offer an ASL file chooser." >&2
  exit 1
fi
if ! grep -q 'Archive Extractor is a drop target' "$DOCK_ALL"; then
  echo "Archive interaction test failed: the drop target must ignore ordinary clicks." >&2
  exit 1
fi
for popup_handler in popup_action spacer_popup_action plugin_popup_action; do
  if sed -n "/^\(static \)*int $popup_handler(/,/^}/p" "$DOCK_ALL" |
     grep -q 'require_edit_mode'; then
    echo "Popup removal test failed: explicit item-menu edits must work while locked." >&2
    exit 1
  fi
done
if ! grep -q 'toggle_monitor_view' "$DOCK_ALL" ||
   ! grep -q '"DROP HERE"' plugin_src/archive/archive.c ||
   grep -q '"DROP ARCHIVE"' plugin_src/archive/archive.c; then
  echo "Dock-click test failed: CPU/RAM toggles and compact Extract text are required." >&2
  exit 1
fi
if ! grep -q 'format_ram_tooltip' "$DOCK_ALL" ||
   ! grep -q 'CHIP: %lu.%luM used' "$DOCK_ALL" ||
   ! grep -q 'fill(dock,ND_ACCENT2,x+10,fast_y' "$DOCK_ALL" ||
   ! grep -q 'caption="CHIP USED"' plugin_src/memory/memory.c; then
  echo "Chip-memory display test failed: RAM needs two coloured meters and a detailed tooltip." >&2
  exit 1
fi
if grep -q '"Add dock row"' "$DOCK_ALL" ||
   grep -q '"Remove dock row"' "$DOCK_ALL" ||
   grep -q 'G_PREV_ROW\|G_NEXT_ROW\|Row up\|Row down' prefs/novadockitemprefs.c ||
   ! grep -q 'p->active_grid_row\[adjacent\]==current' prefs/novadockitemprefs.c ||
   ! grep -q 'nd_move_visible_to_row(p->cfg,p->active_selected' prefs/novadockitemprefs.c ||
   ! grep -q 'nd_layout_item_cell' prefs/novadockitemprefs.c; then
  echo "Multi-row editing test failed: Preferences Up/Down must cross row boundaries without separate row buttons." >&2
  exit 1
fi
if ! grep -q 'plugin_can_launch' "$DOCK_ALL" ||
   ! grep -Fq 'return plugin_actions[item + (plugin_can_launch' "$DOCK_ALL" ||
   ! sed -n '/^\(static \)*const char \*plugin_actions\[\]/,/^};/p' "$DOCK_ALL" |
      grep -q '"Launch", "Edit settings...", "Remove"' ||
   ! sed -n '/^\(static \)*int plugin_popup_action(/,/^}/p' "$DOCK_ALL" |
      grep -q 'remove_dragged_item(dock,index)' ||
   ! grep -q 'status="STARTING"' plugin_src/dopus4/dopus4.c ||
   ! grep -q 'restore_and_front' plugin_src/dopus4/dopus4.c ||
   ! grep -q '"UNICONIFY"' plugin_src/dopus4/dopus4.c; then
  echo "Plugin menu test failed: launcher-style actions and DOpus restore support are required." >&2
  exit 1
fi
if ! grep -q 'nd_client_update_quiet' plugin_src/common/nd_client.c ||
   ! grep -q 'nd_client_update_quiet' plugin_src/clock/clock.c ||
   ! grep -q 'nd_client_update_quiet' plugin_src/eyes/eyes.c ||
   ! grep -q 'nd_client_update_quiet' plugin_src/archive/archive.c ||
   ! grep -q 'nd_client_update_quiet' plugin_src/dopus4/dopus4.c; then
  echo "Idle plugin traffic test failed: stable tiles must use change-driven updates." >&2
  exit 1
fi
if ! grep -q 'nd_client_update_graph' plugin_src/activity/activity.c ||
   ! grep -q 'nd_client_update_graph' plugin_src/memory/memory.c ||
   ! grep -q 'nd_client_update_graph' plugin_src/network/network.c ||
   grep -q 'nd_client_update_quiet' plugin_src/activity/activity.c ||
   grep -q 'nd_client_update_quiet' plugin_src/memory/memory.c ||
   ! grep -q 'online && sampler->fresh' plugin_src/network/network.c; then
  echo "Idle plugin traffic test failed: live graphs must keep every sample." >&2
  exit 1
fi
if grep -q 'fopen(filename, "wb")' src/config.c ||
   ! grep -q 'rename(tmp, filename)' src/config.c ||
   ! grep -q 'preserve_config_backup(filename,backup)' src/config.c ||
   ! grep -q 'rename(swap,filename)' src/config.c; then
  echo "Config safety test failed: saves must replace a staged complete file." >&2
  exit 1
fi
if ! grep -q 'nd_config_runtime_equal(next,&dock->conf)' "$DOCK_ALL" ||
   ! grep -q 'memcmp(&next_theme,&dock->theme_spec' "$DOCK_ALL" ||
   ! grep -q 'diagnostic_reload_skipped' "$DOCK_ALL" ||
   ! grep -q 'Unchanged IPC:' prefs/novadockdiagnosticsprefs.c ||
   ! grep -q 'Update outcomes:' prefs/novadockdiagnosticsprefs.c; then
  echo "Reload diagnostics test failed: no-op reloads and avoided work must be visible." >&2
  exit 1
fi
if ! grep -q 'ND_TIMER_IDLE_QUANTA 5' include/nd_control.h ||
   ! grep -q 'timer_quanta=fast ? 1 : ND_TIMER_IDLE_QUANTA' "$DOCK_ALL" ||
   ! grep -q 'watchdog_timer_ticks+=quantum' "$DOCK_ALL" ||
   ! grep -q 'adaptive timer %s' prefs/novadockdiagnosticsprefs.c; then
  echo "Adaptive timer test failed: idle housekeeping must back off without changing deadlines." >&2
  exit 1
fi
if sed -n '/case A_THEME:/,/break;/p' prefs/novadockappearanceprefs.c |
     grep -q 'apply_appearance'; then
  echo "Appearance theme test failed: browsing a theme must not change the live dock." >&2
  exit 1
fi
if ! grep -q 'case A_TEST: apply_appearance(p,0,0)' prefs/novadockappearanceprefs.c ||
   ! grep -q 'case A_USE: apply_appearance(p,0,1)' prefs/novadockappearanceprefs.c ||
   ! grep -q 'case A_SAVE: apply_appearance(p,1,1)' prefs/novadockappearanceprefs.c; then
  echo "Appearance action test failed: Test must stay open; Use and Save must finish." >&2
  exit 1
fi
if ! grep -q 'appearance_item_sizes' prefs/novadockappearanceprefs.c ||
   ! grep -q 'draw_tiny_plugin' "$DOCK_ALL" ||
   ! grep -q 'ITEM_SIZE=%s' src/config.c ||
   ! grep -q 'item_size==ND_ITEM_SIZE_TINY) return 0' "$DOCK_ALL" ||
   ! grep -q 'centered_text_tight(dock,ND_TEXT,"EXTR"' "$DOCK_ALL" ||
   ! grep -q 'if (!tiny)' "$DOCK_ALL" ||
   ! grep -q 'GA_Disabled,(ULONG)tiny' prefs/novadockappearanceprefs.c ||
   ! grep -q 'preview_tiny_sample' prefs/novadockappearanceprefs.c ||
   ! grep -q 'preview_tiny_clock' prefs/novadockappearanceprefs.c ||
   ! grep -q 'nd_option_get(&dock->conf,"CLOCK","FORMAT"),"ANALOG"' "$DOCK_ALL" ||
   ! grep -q 'draw_plugin_running_indicator' "$DOCK_ALL" ||
   ! grep -q 'nd_indicator_mode(item->id,item->online' "$DOCK_ALL" ||
   ! grep -q 'centered_text(dock,ND_SUBTLE,"MUSIC"' "$DOCK_ALL" ||
   ! grep -q 'button_w=15,button_h=12' "$DOCK_ALL"; then
  echo "Tiny item mode test failed: Appearance, persistence, and compact rendering are required." >&2
  exit 1
fi
if ! grep -q 'tiny_kinds\[4\]={2,7,6,5}' prefs/novadockappearanceprefs.c ||
   ! grep -q 'columns=4,samples=4' prefs/novadockappearanceprefs.c; then
  echo "Appearance preview test failed: Normal and Tiny must compare the same four representative items." >&2
  exit 1
fi
if grep -q '"Available"' prefs/novadockitemprefs.c ||
   grep -q '"Add >>"' prefs/novadockitemprefs.c ||
   grep -q '"Graph / Bar"' prefs/novadockitemprefs.c ||
   ! grep -q 'new_rows=(old_rows%ND_ROWS_MAX)+1' prefs/novadockitemprefs.c; then
  echo "Organize UI test failed: use one dock list and a forward-cycling row selector." >&2
  exit 1
fi
if grep -q '"Items"\|"> Items"' prefs/novadockprefs.c \
                                      prefs/novadockappearanceprefs.c \
                                      prefs/novadockitemprefs.c \
                                      prefs/novadockdiagnosticsprefs.c \
                                      prefs/novadockpluginprefs.c \
                                      prefs/novadocklauncherprefs.c ||
   ! grep -q 'launcher_argv\[2\]=(char \*)"FOCUSED=1"' prefs/novadockprefs.c ||
   ! grep -q 'spacer_argv\[2\]=(char \*)"FOCUSED=1"' prefs/novadockprefs.c ||
   ! grep -q 'result=nd_plugin_prefs_run(3,plugin_argv)' prefs/novadockprefs.c ||
   ! grep -q 'p->result=ND_PREF_NAV_LAUNCHER' prefs/novadockitemprefs.c; then
  echo "Focused Organize editor test failed: obsolete Items navigation must stay removed." >&2
  exit 1
fi
if ! grep -q 'ND_PLUGIN_IPC_TIMEOUT_SECONDS' plugin_src/common/nd_client.c ||
   ! grep -q 'TR_ADDREQUEST' plugin_src/common/nd_client.c ||
   ! grep -q 'PA_IGNORE' plugin_src/common/nd_client.c ||
   ! grep -q 'clear_timeout_completion' plugin_src/common/nd_client.c ||
   ! grep -q 'SetSignal(0L,signal)' plugin_src/common/nd_client.c ||
   [ "$(grep -c 'AllocVec(sizeof(c->packet)' plugin_src/common/nd_client.c)" -ne 1 ]; then
  echo "Plugin IPC test failed: bounded waits must reuse one startup packet." >&2
  exit 1
fi
if ! grep -q 'wake_pending=1' plugin_src/common/nd_client.c ||
   ! grep -q 'changed=c->wake_pending' plugin_src/common/nd_client.c ||
   ! grep -q 'nd_client_update_media_quiet' plugin_src/musicplayer/musicplayer.c ||
   ! grep -q 'elapsed_ticks,500U' plugin_src/clock/clock.c; then
  echo "Quiet plugin IPC test failed: stable tiles must back off without delaying dock events." >&2
  exit 1
fi
if grep -q 'strcpy(item->value,"STALE")' "$DOCK_ALL" ||
   ! grep -q 'mp_Flags==PA_IGNORE' "$DOCK_ALL" ||
   ! grep -q 'abandoned\[index\]=1;' "$DOCK_ALL" ||
   ! grep -q 'if (!packet_abandoned)' "$DOCK_ALL" ||
   [ "$(grep -c 'if (dock->ipc) process_ipc(dock,0)' "$DOCK_ALL")" -lt 4 ]; then
  echo "Plugin recovery test failed: reloads must service IPC and replace abandoned clients without flashing STALE." >&2
  exit 1
fi
if sed -n '/if (!quitting) for (i=0;i<dock->conf.count;++i)/,/timer coalesces/p' "$DOCK_ALL" |
     grep -q 'launch_plugin(dock,i,1)'; then
  echo "Plugin recovery test failed: abandoned IPC must wait for watchdog backoff." >&2
  exit 1
fi
if ! grep -q 'ND_CONTROL_TIMEOUT_SECONDS' src/control_client.c ||
   ! grep -q 'ND_CONTROL_RELOAD_TIMEOUT_SECONDS' include/nd_control.h ||
   ! sed -n '/^LONG nd_control_reload/,/^}/p' src/control_client.c |
       grep -q 'ND_CONTROL_RELOAD_TIMEOUT_SECONDS' ||
   ! grep -q 'TR_ADDREQUEST' src/control_client.c ||
   ! grep -q 'PA_IGNORE' src/control_client.c ||
   grep -q 'WaitPort(reply)' src/control_client.c ||
   ! grep -q 'dock reload timed out' prefs/novadockprefs.c; then
  echo "Control IPC test failed: Preferences must use bounded command and reload waits." >&2
  exit 1
fi
if ! grep -q 'ND_EVENT_THROTTLE' include/nd_protocol.h ||
   ! grep -q 'dock->hidden.*ND_EVENT_QUIT' "$DOCK_ALL" ||
   ! grep -q 'nd_client_delay_ticks' plugin_src/common/nd_client.c ||
   ! grep -q 'nd_client_delay_ticks' plugin_src/activity/activity.c ||
   ! grep -q 'nd_client_delay_ticks' plugin_src/memory/memory.c ||
   ! grep -q 'nd_client_delay_ticks' plugin_src/network/network.c ||
   ! grep -q 'Plugin pacing:' prefs/novadockdiagnosticsprefs.c; then
  echo "Hidden pacing test failed: headless plugins must back off safely." >&2
  exit 1
fi
if ! grep -q 'nd_client_update_graph_quiet' plugin_src/common/nd_client.c ||
   ! grep -q 'nd_client_update_graph_quiet' plugin_src/activity/activity.c ||
   ! grep -q 'nd_client_update_graph_quiet' plugin_src/memory/memory.c; then
  echo "Quiet graph IPC test failed: stable CPU/RAM samples must avoid needless messages." >&2
  exit 1
fi
if ! grep -q 'ND_ARCHIVE_IDLE_TICKS' plugin_src/archive/archive.c ||
   ! sed -n '/^\(static \)*int launch_plugin_helper/,/^}/p' "$DOCK_ALL" |
       grep -q 'wake_plugin(dock,index)' ||
   ! sed -n '/^static void wake_plugin(struct NDDock/,/^}/p' "$DOCK_ALL" |
       grep -q 'ND_WAKE_SIGNAL' ||
   ! sed -n '/^\(static \)*int launch_archive_helper/,/^}/p' "$DOCK_ALL" |
       grep -q 'launch_plugin_helper(dock,index,"EXTRACT"' ||
   ! grep -q 'nd_client_wait(&client,effective_wait)' \
       plugin_src/musicplayer/musicplayer.c; then
  echo "Idle plugin pacing test failed: passive tiles must sleep and wake on demand." >&2
  exit 1
fi
if ! grep -q 'ND_CONTROL_DIAGNOSTICS_RESET' include/nd_control.h ||
   ! grep -q 'reset_diagnostic_counters(dock)' "$DOCK_ALL" ||
   ! grep -q 'D_REPORT_PATH "RAM:NovaDock-Diagnostics.txt"' prefs/novadockdiagnosticsprefs.c ||
   ! grep -q 'D_SAVE_REPORT,"Save report"' prefs/novadockdiagnosticsprefs.c; then
  echo "Diagnostics controls test failed: reset and text export are required." >&2
  exit 1
fi
if ! grep -q 'novadock.cfg.bak' src/config.c ||
   ! grep -q 'nd_effective_config_source' src/config.c ||
   ! grep -q 'Recovered config:' prefs/novadockdiagnosticsprefs.c; then
  echo "Config recovery test failed: valid backups must be retained and reported." >&2
  exit 1
fi
echo "Shared-screen colour and refresh safety checks passed."

# Optional website checks for hosts with Python 3 (not required on AmigaOS).
if command -v "$PYTHON" >/dev/null 2>&1; then
  "$PYTHON" tests/test_info_icons.py
  if [ "${ND_DEFER_WEB_CHECKS:-0}" = 1 ]; then
    echo "Website package checks deferred until release packaging completes."
  elif [ -f public-web/index.php ]; then
    "$PYTHON" tests/test_public_web.py
  else
    echo "Website checks skipped in the source-only archive."
  fi
fi
