#!/bin/sh
set -eu
PYTHON=${PYTHON:-python3}
export PYTHON

usage()
{
    echo "Usage: $0 DESTINATION_ROOT" >&2
    echo "Builds NovaDock and stages it in DESTINATION_ROOT/NovaDock." >&2
    exit 2
}

if [ "$#" -ne 1 ]; then
    usage
fi

project_root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
destination_root=$1

case "$destination_root" in
    ""|/)
        echo "NovaDock: refusing unsafe destination root: '$destination_root'" >&2
        exit 2
        ;;
esac

mkdir -p "$destination_root"
destination_root=$(CDPATH= cd "$destination_root" && pwd)
destination="$destination_root/NovaDock"

# ND_SKIP_BUILD=1 stages the binaries already built (used by make-release.sh
# right after a build, for example in CI).
if [ "${ND_SKIP_BUILD:-0}" = 1 ]; then
    echo "NovaDock: staging the existing binaries (ND_SKIP_BUILD=1)."
else
    echo "NovaDock: rebuilding Classic AmigaOS binaries..."
    sh "$project_root/scripts/build-amiga.sh" -B
fi

required_binaries="
NovaDock
NovaDockPrefs
MakeIcon
SetupStartup
StartInstaller
SaveConfig
Plugins/Clock
Plugins/Eyes
Plugins/Activity
Plugins/Memory
Plugins/Network
Plugins/MusicPlayer
Plugins/DOpus4
Plugins/ArchiveExtractor
Plugins/Command
Plugins/RunningApps
Plugins/Trash
Plugins/Games
Plugins/AmIRC
Plugins/BitSkim
Plugins/Clipboard
Plugins/Mixer
Plugins/Screenshot
Plugins/ScreenMode
Plugins/Script
"

for relative_path in $required_binaries; do
    if [ ! -f "$project_root/$relative_path" ]; then
        echo "NovaDock: build did not produce $relative_path" >&2
        exit 1
    fi
done

mkdir -p "$destination/Plugins" "$destination/Icons" "$destination/Themes"
"$PYTHON" "$project_root/tools/generate_info_icons.py" --drawer-icon "$destination.info"

for relative_path in $required_binaries; do
    target_directory=$destination
    case "$relative_path" in
        Plugins/*) target_directory="$destination/Plugins" ;;
    esac
    cp -p "$project_root/$relative_path" "$target_directory/"
done

"$PYTHON" "$project_root/tools/generate_info_icons.py"
# Remove retired files when refreshing an existing staging drawer.
rm -f "$destination/Plugins/Roadshow" "$destination/Plugins/Roadshow.info" "$destination/Plugins/Roadshow.ndplugin"
cp -p "$project_root"/Plugins/*.info "$destination/Plugins/"

# Copy all self-describing plugin manifests, including optional components.
cp -p "$project_root"/Plugins/*.ndplugin "$destination/Plugins/"

# Workbench entry points and the optional in-Amiga installer.
cp -p "$project_root/NovaDock.info" "$destination/NovaDock.info"
cp -p "$project_root/NovaDockPrefs.info" "$destination/NovaDockPrefs.info"
cp -p "$project_root/Plugins/MusicPlayer.info" \
    "$destination/Plugins/MusicPlayer.info"
cp -p "$project_root/Plugins/RunningApps.info" \
    "$destination/Plugins/RunningApps.info"
cp -p "$project_root/Plugins/RunningApps.ndplugin" \
    "$destination/Plugins/RunningApps.ndplugin"
cp -p "$project_root/Plugins/Trash.ndplugin" \
    "$destination/Plugins/Trash.ndplugin"
cp -p "$project_root/Plugins/Games.ndplugin" \
    "$destination/Plugins/Games.ndplugin"
cp -p "$project_root/install.amiga" "$destination/install.amiga"
# The GUI installer (double-click Install-NovaDock) and the AmigaGuide
# manual that Help in Preferences opens (PROGDIR:NovaDock.guide here).
cp -p "$project_root/Install-NovaDock" "$destination/Install-NovaDock"
cp -p "$project_root/Install-NovaDock.info" "$destination/Install-NovaDock.info"
cp -p "$project_root/Icons/SoftwarePackage.info" "$destination/Icons/SoftwarePackage.info"
cp -p "$project_root/NovaDock.guide" "$destination/NovaDock.guide"
cp -p "$project_root/NovaDock.guide.info" "$destination/NovaDock.guide.info"
for extra in README.md LICENSE BitSkim-icon-LICENSE.txt CHANGELOG.md examples.rexx; do
    cp -p "$project_root/$extra" "$destination/$extra"
done

mkdir -p "$destination/Rexx"
cp -p "$project_root"/Rexx/*.amirx "$destination/Rexx/"
cp -p "$project_root"/Rexx/*.rexx "$destination/Rexx/"

# Launcher artwork used by the stock configuration. PNG authoring sources are
# deliberately omitted from this runnable staging directory.
for icon in "$project_root"/Icons/*.iff; do
    cp -p "$icon" "$destination/Icons/"
done

# Themes are colour configuration at runtime. Copy only the files NovaDock
# reads; graphic backdrop sources are not part of the current theme system.
for theme_directory in "$project_root"/Themes/*; do
    if [ -f "$theme_directory/theme.cfg" ]; then
        theme_name=$(basename "$theme_directory")
        mkdir -p "$destination/Themes/$theme_name"
        cp -p "$theme_directory/theme.cfg" \
            "$destination/Themes/$theme_name/theme.cfg"
    fi
done

# Translations: compiled with FlexCat when FLEXCAT points to it (CI builds it;
# Aminet: dev/misc/FlexCat.lha).  Without it the dock simply stays English.
if [ -n "${FLEXCAT:-}" ] && [ -x "$FLEXCAT" ]; then
    for ct in "$project_root"/catalogs/*.ct; do
        language=$(basename "$ct" .ct)
        mkdir -p "$destination/Catalogs/$language"
        status=0
        "$FLEXCAT" "$project_root/catalogs/novadock.cd" "$ct" \
            CATALOG "$destination/Catalogs/$language/novadock.catalog" \
            >/dev/null 2>&1 || status=$?
        # 5 = warnings only (deliberate %.0s plurals; see catalogs/README.txt).
        if [ "$status" -ne 0 ] && [ "$status" -ne 5 ]; then
            echo "NovaDock: FlexCat failed on $ct" >&2
            exit 1
        fi
    done
    echo "NovaDock: compiled translations into $destination/Catalogs"
else
    echo "NovaDock: FLEXCAT not set; translations not compiled."
fi

# Do not overwrite a configuration that may already have been edited while
# testing from the shared drive. Always publish the latest packaged default.
cp -p "$project_root/novadock.cfg" "$destination/novadock.cfg.default"
if [ ! -f "$destination/novadock.cfg" ]; then
    cp -p "$project_root/novadock.cfg" "$destination/novadock.cfg"
else
    echo "NovaDock: preserving existing $destination/novadock.cfg"
fi

echo "NovaDock: staged runnable AmigaOS files in:"
echo "  $destination"
echo "NovaDock: from AmigaOS, double-click NovaDock/Install-NovaDock to install,"
echo "NovaDock: or run NovaDock/NovaDock directly from the staged drawer."
