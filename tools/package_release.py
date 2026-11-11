#!/usr/bin/env python3
"""Build NovaDock source and PHP-website ZIPs with no nested website ZIP.

Run from any directory: python3 tools/package_release.py --out /path/to/output
This is a host-side maintainer tool, not an AmigaOS executable.
"""
import argparse
import hashlib
import re
import shutil
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / 'public-web'
ROOT_FILES = ('.gitignore', 'README.md', 'CHANGELOG.md', 'DEVELOPMENT.md', 'LICENSE', 'Makefile',
              'Makefile.cross', 'build.amiga', 'install.amiga', 'novadock.cfg',
              'examples.rexx', 'NovaDock.info', 'NovaDockPrefs.info',
              'Plugins/MusicPlayer.info', 'Plugins/RunningApps.info',
              'Plugins/BitSkim.ndplugin', 'Plugins/BitSkim.info', 'Plugins/AmIRC.ndplugin', 'Plugins/AmIRC.info',
              'Plugins/Clipboard.ndplugin', 'Plugins/Clipboard.info',
              'Plugins/Network.ndplugin', 'Plugins/Network.info', 'Plugins/RunningApps.ndplugin', 'Plugins/Trash.ndplugin', 'Plugins/Games.ndplugin',
              'Plugins/Mixer.ndplugin', 'Plugins/Screenshot.ndplugin', 'Plugins/ScreenMode.ndplugin', 'Install-NovaDock',
              'Install-NovaDock.info', 'NovaDock.guide', 'NovaDock.guide.info',
              'BitSkim-icon-LICENSE.txt', 'VERSION')
SOURCE_DIRS = ('Rexx', 'src', 'prefs', 'plugin_src', 'include', 'Icons', 'Backgrounds', 'Themes',
               'docs', 'tests', 'tools', 'catalogs', 'scripts', 'plugin-sdk', 'examples')
# The release version comes from VERSION (see tools/update_version.py).
VERSION = next(line.split('=', 1)[1].strip()
               for line in (ROOT / 'VERSION').read_text('ascii').splitlines()
               if line.startswith('VERSION='))

SDK_FILES = (
    ('LICENSE', 'LICENSE'),
    ('README.md', 'docs/PLUGIN_SDK_QUICKSTART.md'),
    ('PLUGIN_SDK_SETTINGS.md', 'docs/PLUGIN_SDK_SETTINGS.md'),
    ('Makefile', 'plugin_src/template/Makefile.sdk'),
    ('include/nd_protocol.h', 'include/nd_protocol.h'),
    ('include/nd_reply.h', 'include/nd_reply.h'),
    ('include/nd_client.h', 'plugin_src/common/nd_client.h'),
    ('src/nd_client.c', 'plugin_src/common/nd_client.c'),
    ('include/nd_option_runtime.h', 'include/nd_option_runtime.h'),
    ('src/nd_option_runtime.c', 'plugin_src/common/nd_option_runtime.c'),
    ('examples/hello.c', 'plugin_src/template/hello.c'),
    ('examples/Hello.ndplugin', 'plugin_src/template/Hello.ndplugin'),
)


def kept(path):
    return (not any(p.startswith('.') or p == '__pycache__' or p.endswith('.dSYM') for p in path.parts)
            and path.suffix.lower() not in ('.o', '.a', '.pyc', '.exe')
            and not path.name.endswith(('~', '.swp'))
            and not (path.parent.name == 'tests'
                     and path.name.startswith('nd_')
                     and path.name.endswith('_test')))


def _source_files():
    record = ROOT/'build/provenance/amiga.json'
    if record.is_file():
        yield record
    for name in ROOT_FILES:
        path = ROOT / name
        if not path.is_file():
            raise FileNotFoundError(path)
        yield path
    for dirname in SOURCE_DIRS:
        directory = ROOT / dirname
        for path in sorted(directory.rglob('*')):
            if path.is_file() and kept(path.relative_to(ROOT)):
                if dirname == 'plugin-sdk' and path.name == 'Hello':
                    continue
                # Toolchains may retain generated assembly beside C files.
                # Ship only assembly explicitly used by the native build.
                if path.suffix == '.asm' and path.relative_to(ROOT).as_posix() not in \
                        re.findall(r'[\w/.-]+\.asm', (ROOT/'Makefile').read_text()):
                    continue
                yield path


def source_files():
    seen = set()
    metadata = sorted((ROOT/'Plugins').glob('*.info')) + sorted((ROOT/'Plugins').glob('*.ndplugin'))
    for path in list(_source_files()) + metadata:
        if path not in seen:
            seen.add(path)
            yield path


def zip_bytes(entries):
    """Entries are (ZIP member name, file bytes). Constant metadata for repeatability."""
    from io import BytesIO
    stream = BytesIO()
    with ZipFile(stream, 'w', compression=ZIP_DEFLATED, compresslevel=9) as archive:
        for name, payload in sorted(entries):
            info = ZipInfo(name, date_time=(2020, 1, 1, 0, 0, 0))
            info.compress_type = ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, payload, compress_type=ZIP_DEFLATED, compresslevel=9)
    return stream.getvalue()


def archive_entries(files, prefix=''):
    return [(prefix + path.relative_to(ROOT).as_posix(), path.read_bytes())
            for path in files]


def website_files():
    for path in sorted(WEB.rglob('*')):
        if path.is_file() and kept(path.relative_to(WEB)):
            yield path


def sdk_entries():
    prefix = 'NovaDock-PluginSDK/'
    return [(prefix + member, (ROOT / source).read_bytes())
            for member, source in SDK_FILES]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT.parent / 'dist',
                        help='Destination for outer project and website archives')
    parser.add_argument('--release-dir', type=Path, default=ROOT.parent / 'dist' / 'fixed',
                        help='Optional verified LHA/readme directory for site binary downloads')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    # The site's source download deliberately excludes public-web to avoid recursion.
    from generate_info_icons import generic_icon
    drawer_entry=('NovaDock.info',generic_icon('SoftwarePackage.info',kind=2,show_icons=True))
    # Keep the browsable Git SDK tree identical to the standalone download.
    for member, source_name in SDK_FILES:
        target = ROOT/'plugin-sdk'/member
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((ROOT/source_name).read_bytes())
    source = zip_bytes(archive_entries(source_files(), 'NovaDock/')+[drawer_entry])
    download = WEB / 'downloads' / f'NovaDock-{VERSION}-source.zip'
    download.parent.mkdir(parents=True, exist_ok=True)
    # After a version change the previous release's archives go away.
    for stale in download.parent.glob('NovaDock-*.zip'):
        if stale.name not in (download.name, f'NovaDock-{VERSION}-plugin-sdk.zip'):
            stale.unlink()
    download.write_bytes(source)
    checksum = hashlib.sha256(source).hexdigest()
    sdk = zip_bytes(sdk_entries())
    sdk_download = WEB / 'downloads' / f'NovaDock-{VERSION}-plugin-sdk.zip'
    sdk_download.write_bytes(sdk)
    sdk_checksum = hashlib.sha256(sdk).hexdigest()
    sums = f'{checksum}  {download.name}\n{sdk_checksum}  {sdk_download.name}\n'
    release = args.release_dir / f'NovaDock-{VERSION}.lha'
    readme = release.with_suffix('.readme')
    proof = release.with_suffix('.lha.sha256')
    if release.is_file() and readme.is_file() and proof.is_file():
        digest = hashlib.sha256(release.read_bytes()).hexdigest()
        if proof.read_text('ascii').split()[0] != digest:
            raise SystemExit('Refusing to package an LHA with a mismatched checksum')
        shutil.copyfile(release, download.parent / release.name)
        shutil.copyfile(readme, download.parent / readme.name)
    bundled = download.parent / release.name
    if bundled.is_file():
        sums += hashlib.sha256(bundled.read_bytes()).hexdigest() + '  ' + bundled.name + '\n'
        bundled_readme = bundled.with_suffix('.readme')
        if not bundled_readme.is_file():
            raise SystemExit('Runnable download is missing its release readme')
        sums += hashlib.sha256(bundled_readme.read_bytes()).hexdigest() + '  ' + bundled_readme.name + '\n'
    (download.parent / 'SHA256SUMS.txt').write_text(sums, encoding='ascii')

    web_entries = [(p.relative_to(WEB).as_posix(), p.read_bytes())
                   for p in website_files()]
    web_archive = args.out / f'NovaDock-{VERSION}-public-web-updated.zip'
    web_archive.write_bytes(zip_bytes(web_entries))

    full_entries = archive_entries(source_files(), 'NovaDock/')+[drawer_entry]
    full_entries.extend(('NovaDock/public-web/' + name, data)
                        for name, data in web_entries)
    full_archive = args.out / f'NovaDock-{VERSION}-continued-source-and-site.zip'
    full_archive.write_bytes(zip_bytes(full_entries))
    print(f'Website source download: {download}')
    print(f'SHA-256: {checksum}')
    print(f'Plugin SDK download: {sdk_download}')
    print(f'SDK SHA-256: {sdk_checksum}')
    print(f'Full project: {full_archive}')
    print(f'Public website: {web_archive}')


if __name__ == '__main__':
    main()
