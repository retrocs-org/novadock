#!/usr/bin/env python3
"""Prepare/check an Aminet submission locally; upload only with --upload.

Official instructions: https://wiki.aminet.net/Uploading_instructions
Readme format: https://wiki.aminet.net/The_Readme_file
No network connection is made by prepare, check, or format-readme.
"""
import argparse
import ftplib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import textwrap

HOST = 'main.aminet.net'
DIRECTORY = '/new'
NAME = 'novadock'


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def readme_fields(path, require_email=True):
    data = path.read_bytes()
    if b'\r' in data or any(byte < 32 and byte not in (9, 10) for byte in data):
        raise ValueError('Readme must use LF line endings and contain no control characters.')
    text = data.decode('latin-1')
    if any(len(line) > 78 for line in text.splitlines()):
        raise ValueError('Readme lines must be at most 78 characters.')
    header, separator, body = text.partition('\n\n')
    if not separator or not body.strip() or not header.startswith('Short:'):
        raise ValueError('Readme must start with Short: and include a description.')
    fields = {}
    for line in header.splitlines():
        key, colon, value = line.partition(':')
        if not colon or not re.fullmatch(r'[A-Za-z]+', key) or key in fields:
            raise ValueError('Malformed or duplicate readme header: ' + line)
        fields[key] = value.strip()
    for key in ('Short', 'Uploader', 'Type', 'Architecture', 'Version'):
        if not fields.get(key):
            raise ValueError('Readme is missing ' + key + ':')
    if len(fields['Short']) > 40:
        raise ValueError('Short: must be at most 40 characters.')
    if not re.fullmatch(r'[a-z0-9]+/[a-z0-9]+', fields['Type']):
        raise ValueError('Type: must be an Aminet category such as util/wb.')
    if fields['Architecture'] != 'm68k-amigaos >= 3.0.0':
        raise ValueError('Architecture: must describe this Classic Amiga build.')
    if fields.get('Distribution', 'Aminet') not in ('Aminet', 'NoCD'):
        raise ValueError('Distribution: must be Aminet or NoCD.')
    if 'Replaces' in fields:
        for old in fields['Replaces'].split(';'):
            if not re.fullmatch(r'[a-z0-9]+/[a-z0-9]+/[A-Za-z0-9][A-Za-z0-9_.?*\-]*', old.strip()):
                raise ValueError('Replaces: needs full Aminet paths, separated by semicolons.')
    if require_email and not re.search(r'[^\s<>@()]+@[^\s<>@()]+\.[^\s<>@()]+', fields['Uploader']):
        raise ValueError('Set UPLOADER to your real email before preparing an Aminet submission.')
    return fields


def format_readme(path, replaces=''):
    """Wrap the generated description; header values must each fit one line."""
    text = path.read_text('utf-8')
    header, separator, body = text.partition('\n\n')
    if not separator:
        raise ValueError('Missing readme description.')
    if replaces:
        if any(c in replaces for c in '\r\n\0'):
            raise ValueError('AMINET_REPLACES must be one header value.')
        header += '\nReplaces:     ' + replaces
    paragraphs = []
    for paragraph in body.strip().split('\n\n'):
        lines = paragraph.splitlines()
        if any(line.startswith('- ') for line in lines):
            items = re.split(r'(?m)^- ', paragraph)
            wrapped = [textwrap.fill(' '.join(item.split()), width=78,
                                     initial_indent='- ', subsequent_indent='  ')
                       for item in items if item.strip()]
            paragraphs.append('\n'.join(wrapped))
        elif all(len(line) <= 78 for line in lines):
            paragraphs.append(paragraph)
        else:
            paragraphs.append(textwrap.fill(' '.join(paragraph.split()), width=78))
    result = header + '\n\n' + '\n\n'.join(paragraphs) + '\n'
    # Write through a temporary file so invalid metadata leaves the original intact.
    with tempfile.TemporaryDirectory(dir=path.parent) as tmp:
        candidate = Path(tmp) / path.name
        candidate.write_bytes(result.encode('latin-1'))
        readme_fields(candidate, require_email=False)
        os.replace(candidate, path)


def archive_crc(path, lha):
    result = subprocess.run([lha, 't', str(path.resolve())], stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    if result.returncode:
        raise ValueError('LHA CRC verification failed: ' + result.stdout[-1200:])


def check_checksum(path):
    checksum = Path(str(path) + '.sha256')
    parts = checksum.read_text('ascii').split()
    if len(parts) != 2 or parts[1] != path.name or parts[0] != sha256(path):
        raise ValueError('Archive checksum is missing, changed or names a different file.')


def prepare(archive, output, lha):
    archive = archive.resolve()
    if archive.suffix != '.lha':
        raise ValueError('Expected a .lha release archive.')
    readme = archive.with_suffix('.readme')
    fields = readme_fields(readme)
    check_checksum(archive)
    archive_crc(archive, lha)
    if (output/'upload-receipt.json').exists():
        raise ValueError('Existing Aminet upload receipt must be preserved; use a new preparation directory.')
    output = output.resolve()
    if output == archive.parent:
        raise ValueError('Use a separate Aminet submission directory.')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.aminet-prepare-', dir=output.parent) as tmp:
        stage = Path(tmp)
        for source, filename in ((archive, NAME+'.lha'), (readme, NAME+'.readme')):
            shutil.copy2(source, stage / filename)
        manifest = {'host': HOST, 'directory': DIRECTORY, 'version': fields['Version'],
                    'files': {name: sha256(stage/name) for name in (NAME+'.lha', NAME+'.readme')}}
        (stage/'submission.json').write_text(json.dumps(manifest, indent=2)+'\n')
        output.mkdir(parents=True, exist_ok=True)
        for file in stage.iterdir():
            os.replace(file, output/file.name)
    return check(output, lha)


def check(folder, lha):
    folder = folder.resolve()
    manifest = json.loads((folder/'submission.json').read_text())
    names = [NAME+'.lha', NAME+'.readme']
    if not isinstance(manifest, dict) or not isinstance(manifest.get('files'), dict):
        raise ValueError('Malformed submission manifest.')
    if manifest.get('host') != HOST or manifest.get('directory') != DIRECTORY:
        raise ValueError('Submission destination differs from the Aminet endpoint.')
    if set(manifest.get('files', {})) != set(names):
        raise ValueError('Submission must contain exactly the archive and matching readme.')
    for name in names:
        if len(name) > 30 or not re.fullmatch(r'[a-z0-9_.-]+', name):
            raise ValueError('Invalid Aminet upload filename: '+name)
        if sha256(folder/name) != manifest['files'][name]:
            raise ValueError(name+' changed since preparation. Prepare the submission again.')
    fields = readme_fields(folder/names[1])
    if fields['Version'] != manifest.get('version'):
        raise ValueError('Readme version does not match the prepared submission.')
    archive_crc(folder/names[0], lha)
    return manifest, fields


def submission_status(folder, manifest):
    path = folder/'upload-receipt.json'
    if not path.exists():
        return 'new'
    if path.is_symlink():
        raise ValueError('Symlinked Aminet upload receipt.')
    receipt = json.loads(path.read_text())
    binding = hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest()
    if receipt.get('submission_sha256') != binding:
        raise ValueError('Aminet receipt belongs to different submission bytes; preserve and inspect it.')
    if receipt.get('status') != 'completed':
        raise ValueError('Partial Aminet submission requires inspection; automatic resubmission refused.')
    return 'completed'


def save_receipt(folder, receipt):
    path = folder/'upload-receipt.json'
    temporary = folder/'upload-receipt.json.tmp'
    if path.is_symlink() or temporary.exists() or temporary.is_symlink():
        raise ValueError('Unsafe Aminet receipt path.')
    with temporary.open('x') as stream:
        json.dump(receipt, stream, indent=2)
        stream.write('\n')
        stream.flush()
        os.fsync(stream.fileno())
    temporary.replace(path)


def upload(folder, manifest, fields, ftp_factory=ftplib.FTP):
    """Transfer only the checked pair, with readme last; never list/delete files."""
    # Snapshot bytes and verify them before connecting: publishing never races
    # a simultaneous rebuild or an edit to the external readme.
    payloads = []
    for name in (NAME+'.lha', NAME+'.readme'):
        data = (folder/name).read_bytes()
        if hashlib.sha256(data).hexdigest() != manifest['files'][name]:
            raise ValueError(name+' changed before upload; nothing was sent.')
        payloads.append((name, data))
    if submission_status(folder, manifest) == 'completed':
        print('Aminet submission already completed; no files resent.')
        return
    receipt = {'submission_sha256': hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest(),
               'status': 'in_progress', 'completed_files': []}
    save_receipt(folder, receipt)
    import io
    email = re.search(r'[^\s<>@()]+@[^\s<>@()]+\.[^\s<>@()]+', fields['Uploader']).group()
    with ftp_factory(timeout=60) as ftp:
        ftp.connect(HOST, 21)
        ftp.login('anonymous', email)
        ftp.cwd(DIRECTORY)
        ftp.set_pasv(True)
        for name, data in payloads:
            ftp.storbinary('STOR '+name, io.BytesIO(data))
            receipt['completed_files'].append(name)
            save_receipt(folder, receipt)
    receipt['status'] = 'completed'
    save_receipt(folder, receipt)


def report(folder, manifest, fields):
    print('Aminet submission preview (no files uploaded):')
    print('  Destination: ftp://'+HOST+DIRECTORY+'/')
    print('  Version: '+fields['Version']+'; category: '+fields['Type'])
    print('  Uploader: '+fields['Uploader'])
    if fields.get('Replaces'):
        print('  Replaces: '+fields['Replaces'])
    for name, digest in manifest['files'].items():
        print('  '+name+' ('+str((folder/name).stat().st_size)+' bytes), SHA-256 '+digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    fmt = sub.add_parser('format-readme'); fmt.add_argument('readme', type=Path)
    fmt.add_argument('--replaces', default='')
    prep = sub.add_parser('prepare'); prep.add_argument('archive', type=Path)
    prep.add_argument('output', type=Path); prep.add_argument('--lha', default=os.getenv('LHA', 'lha'))
    verify = sub.add_parser('check'); verify.add_argument('folder', type=Path)
    verify.add_argument('--lha', default=os.getenv('LHA', 'lha'))
    verify.add_argument('--upload', action='store_true', help='Actually submit the checked pair to Aminet.')
    args = parser.parse_args()
    try:
        if args.command == 'format-readme':
            format_readme(args.readme, args.replaces); return
        if args.command == 'prepare':
            manifest, fields = prepare(args.archive, args.output, args.lha)
            folder = args.output.resolve()
        else:
            folder = args.folder.resolve()
            manifest, fields = check(folder, args.lha)
        report(folder, manifest, fields)
        if getattr(args, 'upload', False):
            upload(folder, manifest, fields)
            print('Submitted archive and readme to Aminet incoming; publication awaits Aminet processing.')
    except (OSError, ValueError, UnicodeError, ftplib.Error) as error:
        parser.exit(1, 'Aminet: '+str(error)+'\n')


if __name__ == '__main__':
    main()
