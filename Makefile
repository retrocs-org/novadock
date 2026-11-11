# Make on AmigaOS with vbcc m68k target + NDK 3.2 (configured as 'vc').
# No C99 or CPU-specific instructions: targets baseline 68000 with OS 3.x.
VC ?= vc
CFLAGS ?= -O -Iinclude -Iplugin_src/common -Iplugin_src/archive

all: NovaDock NovaDockPrefs Plugins/Clock Plugins/Eyes Plugins/Activity Plugins/Memory Plugins/Network Plugins/MusicPlayer Plugins/DOpus4 Plugins/ArchiveExtractor Plugins/Command Plugins/RunningApps Plugins/Trash Plugins/Games Plugins/AmIRC Plugins/BitSkim Plugins/Clipboard Plugins/Mixer Plugins/Screenshot Plugins/ScreenMode Plugins/Script MakeIcon SetupStartup StartInstaller SaveConfig

NovaDock: src/novadock.c src/dock_arexx.c src/dock_draw.c src/dock_events.c src/dock_icons.c src/dock_input.c src/dock_launch.c src/dock_menu.c src/dock_plugins.c src/dock_popups.c src/dock_reload.c src/dock_save.c src/save_policy.c src/indicator.c src/dock_timers.c src/dock_window.c src/dock_internal.h src/config.c src/theme.c src/nd_options.c src/layout.c src/graph.c src/ilbm.c src/icon_mask.c src/interaction.c src/autohide.c src/eyes.c src/watchdog.c src/control_client.c src/nd_locale.c src/nd_media.c src/nd_tasks.c src/nd_folder.c plugin_src/archive/nd_archive.c include/nd_about.h include/nd_theme.h include/nd_screennotify.h include/nd_option_runtime.h include/nd_control_client.h include/nd_locale.h include/nd_strings.h include/nd_media.h include/nd_tasks.h include/nd_folder.h include/nd_eyes.h include/nd_autohide.h include/nd_interaction.h include/nd_ilbm.h include/nd_icon_mask.h include/nd_control.h include/nd_layout.h include/nd_protocol.h include/nd_config.h include/nd_graph.h include/nd_watchdog.h include/nd_version.h include/nd_save.h include/nd_indicator.h include/nd_options.h plugin_src/archive/nd_archive.h include/nd_reply.h include/nd_calendar.h include/nd_search.h
	$(VC) $(CFLAGS) -o NovaDock src/novadock.c src/dock_save.c src/save_policy.c src/indicator.c src/dock_arexx.c src/dock_draw.c src/dock_events.c src/dock_icons.c src/dock_input.c src/dock_launch.c src/dock_menu.c src/dock_plugins.c src/dock_popups.c src/dock_reload.c src/dock_timers.c src/dock_window.c src/config.c src/theme.c src/nd_options.c src/layout.c src/graph.c src/ilbm.c src/icon_mask.c src/interaction.c src/autohide.c src/eyes.c src/watchdog.c src/control_client.c src/nd_locale.c src/nd_media.c src/nd_tasks.c src/nd_folder.c plugin_src/archive/nd_archive.c -lamiga

NovaDockPrefs: prefs/novadockprefs.c prefs/nd_prefs_help.c prefs/novadockappearanceprefs.c prefs/novadockitemprefs.c prefs/novadocklauncherprefs.c prefs/novadockadditemprefs.c prefs/novadockpluginprefs.c prefs/novadockmanagerprefs.c prefs/novadockdiagnosticsprefs.c prefs/novadockaboutprefs.c prefs/novadocklayoutprefs.c prefs/novadockdiscovery.c prefs/nd_package_install.c src/config.c src/theme.c src/nd_options.c src/layout.c src/layout_profiles.c src/prefs_layout.c src/control_client.c src/nd_manifest.c include/nd_about.h include/nd_theme.h include/nd_manifest.h include/nd_plugin_discovery.h include/nd_control_client.h include/nd_prefs_layout.h include/nd_package_install.h include/nd_version.h prefs/nd_network_picker.h include/nd_interface_list.h include/nd_config.h include/nd_control.h include/nd_layout.h include/nd_layout_profiles.h include/nd_numeric_gadget.h include/nd_options.h include/nd_prefs_help.h include/nd_prefs_nav.h include/nd_prefs_session.h include/nd_protocol.h include/nd_reply.h
	$(VC) $(CFLAGS) -DND_PREFS_EMBEDDED -o NovaDockPrefs prefs/novadockprefs.c prefs/nd_prefs_help.c prefs/novadockappearanceprefs.c prefs/novadockitemprefs.c prefs/novadocklauncherprefs.c prefs/novadockadditemprefs.c prefs/novadockpluginprefs.c prefs/novadockmanagerprefs.c prefs/novadockdiagnosticsprefs.c prefs/novadockaboutprefs.c prefs/novadocklayoutprefs.c prefs/novadockdiscovery.c prefs/nd_package_install.c src/config.c src/theme.c src/nd_options.c src/layout.c src/layout_profiles.c src/prefs_layout.c src/control_client.c src/nd_manifest.c -lamiga

Plugins/Clock: plugin_src/clock/clock.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_version.h include/nd_option_runtime.h include/nd_reply.h
	$(VC) $(CFLAGS) -o Plugins/Clock plugin_src/clock/clock.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/Eyes: plugin_src/eyes/eyes.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c include/nd_eyes.h include/nd_protocol.h include/nd_version.h include/nd_option_runtime.h plugin_src/common/nd_client.h include/nd_reply.h
	$(VC) $(CFLAGS) -o Plugins/Eyes plugin_src/eyes/eyes.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/Activity: plugin_src/activity/activity.c plugin_src/activity/cpu_idle_hooks.asm src/metrics.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_metrics.h include/nd_version.h include/nd_option_runtime.h include/nd_reply.h
	$(VC) $(CFLAGS) -o Plugins/Activity plugin_src/activity/activity.c plugin_src/activity/cpu_idle_hooks.asm src/metrics.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c -lamiga

Plugins/Memory: plugin_src/memory/memory.c src/metrics.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_metrics.h include/nd_version.h include/nd_option_runtime.h include/nd_reply.h
	$(VC) $(CFLAGS) -o Plugins/Memory plugin_src/memory/memory.c src/metrics.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/Network: plugin_src/network/network.c plugin_src/network/nd_network.c plugin_src/network/nd_netcontrol.c plugin_src/network/nd_netcontrol.h plugin_src/network/nd_netdetails.h plugin_src/network/nd_roadshow.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c include/nd_version.h include/nd_network.h include/nd_option_runtime.h include/nd_protocol.h include/nd_roadshow.h plugin_src/common/nd_client.h include/nd_reply.h
	$(VC) $(CFLAGS) -Iplugin_src/network -o Plugins/Network plugin_src/network/network.c plugin_src/network/nd_network.c plugin_src/network/nd_netcontrol.c plugin_src/network/nd_roadshow.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/MusicPlayer: plugin_src/musicplayer/musicplayer.c plugin_src/musicplayer/nd_musicplayer.c include/nd_musicplayer.h plugin_src/common/nd_client.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_version.h include/nd_option_runtime.h include/nd_reply.h
	$(VC) $(CFLAGS) -o Plugins/MusicPlayer plugin_src/musicplayer/musicplayer.c plugin_src/musicplayer/nd_musicplayer.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/DOpus4: plugin_src/dopus4/dopus4.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c include/nd_protocol.h include/nd_option_runtime.h include/nd_version.h plugin_src/common/nd_client.h include/nd_reply.h
	$(VC) $(CFLAGS) -o Plugins/DOpus4 plugin_src/dopus4/dopus4.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/ArchiveExtractor: plugin_src/archive/archive.c plugin_src/archive/nd_archive.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c include/nd_protocol.h include/nd_option_runtime.h include/nd_version.h plugin_src/archive/nd_archive.h plugin_src/common/nd_client.h include/nd_reply.h
	$(VC) $(CFLAGS) -o Plugins/ArchiveExtractor plugin_src/archive/archive.c plugin_src/archive/nd_archive.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/Command: plugin_src/command/command.c plugin_src/command/nd_command.c plugin_src/command/nd_command.h plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c include/nd_protocol.h include/nd_option_runtime.h include/nd_version.h plugin_src/common/nd_client.h include/nd_reply.h
	$(VC) $(CFLAGS) -Iplugin_src/command -o Plugins/Command plugin_src/command/command.c plugin_src/command/nd_command.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/RunningApps: plugin_src/runningapps/runningapps.c plugin_src/runningapps/nd_appscan.h plugin_src/common/nd_client.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_version.h include/nd_reply.h include/nd_search.h
	$(VC) $(CFLAGS) -o Plugins/RunningApps plugin_src/runningapps/runningapps.c plugin_src/common/nd_client.c

Plugins/Mixer: plugin_src/mixer/mixer.c plugin_src/mixer/nd_prefsave.h plugin_src/mixer/nd_ahiprefs.c plugin_src/mixer/nd_ahiprefs.h plugin_src/mixer/nd_ahi.h plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_version.h include/nd_option_runtime.h include/nd_reply.h
	$(VC) $(CFLAGS) -Iplugin_src/mixer -o Plugins/Mixer plugin_src/mixer/mixer.c plugin_src/mixer/nd_ahiprefs.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/Screenshot: plugin_src/screenshot/screenshot.c plugin_src/screenshot/nd_shotfile.c plugin_src/screenshot/nd_shotfile.h plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_version.h include/nd_option_runtime.h include/nd_reply.h
	$(VC) $(CFLAGS) -Iplugin_src/screenshot -o Plugins/Screenshot plugin_src/screenshot/screenshot.c plugin_src/screenshot/nd_shotfile.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/ScreenMode: plugin_src/screenmode/screenmode.c plugin_src/screenmode/nd_mode_worker.h plugin_src/screenmode/nd_scrmode.c src/interaction.c plugin_src/screenmode/nd_scrmode.h include/nd_interaction.h plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_version.h include/nd_option_runtime.h include/nd_reply.h
	$(VC) $(CFLAGS) -Iplugin_src/screenmode -o Plugins/ScreenMode plugin_src/screenmode/screenmode.c plugin_src/screenmode/nd_scrmode.c src/interaction.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/Script: plugin_src/script/script.c plugin_src/script/nd_script.c plugin_src/script/nd_script.h plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_version.h include/nd_option_runtime.h include/nd_reply.h
	$(VC) $(CFLAGS) -Iplugin_src/script -o Plugins/Script plugin_src/script/script.c plugin_src/script/nd_script.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

clean:
	-delete >NIL: SaveConfig
	-delete >NIL: Plugins/AmIRC
	-delete >NIL: Plugins/Mixer
	-delete >NIL: Plugins/Screenshot
	-delete >NIL: Plugins/ScreenMode
	-delete >NIL: Plugins/Script
	-delete >NIL: NovaDock
	-delete >NIL: NovaDockPrefs
	-delete >NIL: NovaDockPluginPrefs
	-delete >NIL: NovaDockLauncherPrefs
	-delete >NIL: StartInstaller
	-delete >NIL: SetupStartup
	-delete >NIL: Plugins/Clock
	-delete >NIL: Plugins/Eyes
	-delete >NIL: Plugins/Activity
	-delete >NIL: Plugins/Memory
	-delete >NIL:
	-delete >NIL: Plugins/MusicPlayer
	-delete >NIL: Plugins/DOpus4
	-delete >NIL: Plugins/ArchiveExtractor
	-delete >NIL: Plugins/Network
	-delete >NIL: Plugins/Command
	-delete >NIL: Plugins/RunningApps
	-delete >NIL: MakeIcon

MakeIcon: tools/makeicon.c include/nd_version.h
	$(VC) $(CFLAGS) -o MakeIcon tools/makeicon.c

SetupStartup: tools/setupstartup.c include/nd_version.h
	$(VC) $(CFLAGS) -o SetupStartup tools/setupstartup.c

Plugins/Trash: plugin_src/trash/trash.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_version.h include/nd_option_runtime.h include/nd_reply.h
	$(VC) $(CFLAGS) -o Plugins/Trash plugin_src/trash/trash.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/Games: plugin_src/games/games.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c plugin_src/common/nd_client.h include/nd_protocol.h include/nd_option_runtime.h include/nd_version.h include/nd_reply.h
	$(VC) $(CFLAGS) -o Plugins/Games plugin_src/games/games.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c -lamiga

StartInstaller: tools/startinstaller.c include/nd_version.h
	$(VC) $(CFLAGS) -o $@ tools/startinstaller.c

Plugins/AmIRC: plugin_src/amirc/amirc.c plugin_src/amirc/nd_amirc.c plugin_src/amirc/nd_amirc.h plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c include/nd_version.h include/nd_option_runtime.h include/nd_protocol.h plugin_src/common/nd_client.h include/nd_reply.h
	$(VC) $(CFLAGS) -Iplugin_src/amirc -o Plugins/AmIRC plugin_src/amirc/amirc.c plugin_src/amirc/nd_amirc.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

SaveConfig: tools/saveconfig.c src/config.c src/nd_options.c include/nd_save.h include/nd_config.h include/nd_options.h include/nd_version.h
	$(VC) $(CFLAGS) -o SaveConfig tools/saveconfig.c src/config.c src/nd_options.c

Plugins/Clipboard: plugin_src/clipboard/clipboard.c plugin_src/clipboard/nd_clipboard_popup.c plugin_src/clipboard/nd_clipboard.c plugin_src/clipboard/nd_clipboard.h plugin_src/clipboard/nd_clipboard_popup.h plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c include/nd_version.h include/nd_option_runtime.h include/nd_protocol.h plugin_src/common/nd_client.h include/nd_reply.h include/nd_search.h
	$(VC) $(CFLAGS) -Iplugin_src/clipboard -o $@ plugin_src/clipboard/clipboard.c plugin_src/clipboard/nd_clipboard_popup.c plugin_src/clipboard/nd_clipboard.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c

Plugins/BitSkim: plugin_src/bitskim/nd_bitskim_ui.h plugin_src/bitskim/bitskim.c plugin_src/bitskim/nd_bitskim.c plugin_src/bitskim/nd_bitskim.h plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c include/nd_protocol.h include/nd_option_runtime.h include/nd_version.h plugin_src/bitskim/nd_bs_request.h plugin_src/common/nd_client.h include/nd_reply.h
	$(VC) $(CFLAGS) -Iplugin_src/bitskim -o $@ plugin_src/bitskim/bitskim.c plugin_src/bitskim/nd_bitskim.c plugin_src/common/nd_client.c plugin_src/common/nd_option_runtime.c
