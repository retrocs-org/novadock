#!/usr/bin/env python3
"""Build verified release bundles and explicitly publish them. See docs/RELEASING.md."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import urllib.error
import urllib.parse
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
import build_provenance as provenance
import release_credentials as credentials
import release_source
Q = lambda value: urllib.parse.quote(str(value), safe='')


def run(*args, capture=False, env=None):
    return subprocess.run(args, cwd=ROOT, check=True, text=True,
                          stdout=subprocess.PIPE if capture else None,
                          stderr=subprocess.PIPE if capture else None, env=env).stdout


digest = provenance.digest

def version():
    value = next(s.split('=', 1)[1].strip() for s in (ROOT/'VERSION').read_text().splitlines() if s.startswith('VERSION='))
    if not re.fullmatch(r'[0-9]+(?:\.[0-9]+)*(?:[-.][A-Za-z0-9]+)*', value):
        raise ValueError('Unsupported release version.')
    return value


def commit(clean=False):
    try:
        if Path(run('git', 'rev-parse', '--show-toplevel', capture=True).strip()).resolve() != ROOT.resolve():
            raise ValueError('Release source must be the root of its own Git checkout.')
        value = run('git', 'rev-parse', 'HEAD', capture=True).strip()
    except subprocess.CalledProcessError:
        if clean:
            raise ValueError('Publishing requires a committed Git checkout.')
        return None
    if clean and run('git', 'status', '--porcelain', '--untracked-files=all', capture=True).strip():
        raise ValueError('Commit or remove working-tree changes before publishing.')
    if clean:
        release_source.validate_tracked(ROOT)
    return value


class NoRedirect(urllib.request.HTTPRedirectHandler):
    # Never forward credentials to storage hosts or a different API origin.
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


class API:
    def __init__(self, base, token, github=False):
        self.base = base.rstrip('/')
        self.token = token
        self.headers = ({'Authorization': 'Bearer '+token,
                         'Accept': 'application/vnd.github+json',
                         'X-GitHub-Api-Version': '2026-03-10'} if github else
                        {'PRIVATE-TOKEN': token})
        self.opener = urllib.request.build_opener(NoRedirect())

    def call(self, method, path, payload=None, raw=False, missing=False, upload=None):
        headers = dict(self.headers)
        if upload is not None:
            data = upload.read_bytes()
            headers['Content-Type'] = 'application/octet-stream'
        elif payload is not None:
            data = json.dumps(payload).encode()
            headers['Content-Type'] = 'application/json'
        else:
            data = None
        try:
            with self.opener.open(urllib.request.Request(self.base+path, data=data,
                                                       headers=headers, method=method), timeout=120) as r:
                body = r.read()
                return body if raw else (json.loads(body) if body else {})
        except urllib.error.HTTPError as error:
            if missing and error.code == 404:
                return None
            # Do not print request headers, tokens or server response bodies.
            raise ValueError('Release API returned HTTP %d for %s %s' %
                             (error.code, method, path)) from None

    def file_digest(self, path, missing=False):
        try:
            with self.opener.open(urllib.request.Request(self.base+path, headers=self.headers), timeout=120) as response:
                result = hashlib.sha256()
                for chunk in iter(lambda: response.read(1024*1024), b''):
                    result.update(chunk)
                return result.hexdigest()
        except urllib.error.HTTPError as error:
            if missing and error.code == 404:
                return None
            raise ValueError('Release API returned HTTP %d while verifying an asset' % error.code) from None


def repo_url(value, github=False):
    u = urllib.parse.urlsplit(value)
    path = u.path.rstrip('/').removesuffix('.git')
    if (u.scheme != 'https' or not u.hostname or u.username or u.password
            or u.query or u.fragment or not re.fullmatch(r'/[A-Za-z0-9_.-]+(?:/[A-Za-z0-9_.-]+)+', path)
            or any(p in ('.', '..') for p in path.split('/')) or u.port or '%' in u.netloc
            or not re.fullmatch(r'[A-Za-z0-9.-]+', u.netloc)):
        raise ValueError('Repository URLs must be credential-free HTTPS URLs.')
    if github and (u.netloc != 'github.com' or len(path.strip('/').split('/')) != 2):
        raise ValueError('Public GitHub repository must be https://github.com/OWNER/REPO.')
    return 'https://'+u.netloc+path, path.strip('/'), 'https://'+u.netloc


def load_json(path):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError('Duplicate JSON key: '+key)
            result[key] = value
        return result
    return json.loads(path.read_text(), object_pairs_hook=unique)


def config(path):
    cfg = load_json(path) if path.is_file() else {}
    allowed = {'VBCC', 'VC', 'LHA', 'CC', 'PHP', 'FLEXCAT', 'AUTHOR', 'AMINET_REPLACES', 'NOVADOCK_RELEASE_PYTHON'}
    allowed_keys = {'github', 'github_git', 'gitlab', 'gitlab_git', 'uploader', 'author', 'env', 'credentials', 'source_branch'}
    if not isinstance(cfg, dict) or set(cfg) - allowed_keys:
        raise ValueError('Unsupported configuration key; credentials belong in the process environment.')
    env = cfg.get('env', {})
    if not isinstance(env, dict) or set(env) - allowed or not all(isinstance(v, str) for v in env.values()):
        raise ValueError('Unsupported build environment setting in release configuration.')
    credentials.validate(cfg.get('credentials', {}))
    if not isinstance(cfg.get('source_branch', ''), str):
        raise ValueError('source_branch must be a branch name or empty.')
    credentials.SECRETS.update(credentials.known(cfg))
    return cfg


def destinations(cfg):
    result = {}
    for target in ('github', 'gitlab'):
        item = cfg.get(target)
        if not item:
            continue
        if isinstance(item, str):
            item = {'url': item, 'push_url': cfg.get(target+'_git', item)}
        url, project, origin = repo_url(item['url'], github=target == 'github')
        push = item.get('push_url', url)
        expected = 'git@'+urllib.parse.urlsplit(origin).hostname+':'+project+'.git'
        if push not in (url, expected):
            raise ValueError('Git push destination must address the same repository as HTTPS URL.')
        result[target] = {'url': url, 'push_url': push}
    return result


def build_commands(out, python=None):
    py = python or sys.executable
    return [
        (py, 'tools/update_version.py', '--check'),
        (py, 'tools/update_site_chrome.py', '--check'),
        (py, 'tools/generate_catalog.py'),
        (py, 'tools/make_amigaguide.py'),
        (py, 'tools/sync_github_releases.py', '--upcoming'),
        (py, 'tools/generate_info_icons.py'),
        ('sh', 'tests/run_host_tests.sh'),
        ('sh', 'scripts/make-release.sh', str(out), '--aminet'),
        (py, 'tools/package_release.py', '--out', str(out), '--release-dir', str(out)),
        (py, 'tests/test_public_web.py'),
    ]


def plan(args):
    import shlex
    cfg = config(args.config)  # No credential lookup, build or API call.
    destinations(cfg)
    print('Target: AmigaOS m68k (68000, OS 3.0+); native runtime checks are separate.')
    for command in build_commands(args.output.resolve(), cfg.get('env', {}).get('NOVADOCK_RELEASE_PYTHON')):
        print(shlex.join(command))
    print('Record source fingerprint, native binaries, compiler and exact package checksums.')
    print('No uploads, signing or notarization are applicable to this target.')


def required_assets(v):
    return {'RELEASE-NOTES.md', 'BUILD-INFO.json', 'SHA256SUMS.txt',
            'NovaDock-'+v+'.lha', 'NovaDock-'+v+'.readme', 'NovaDock-'+v+'.lha.sha256',
            'NovaDock-'+v+'-source.zip', 'NovaDock-'+v+'-plugin-sdk.zip',
            'NovaDock-'+v+'-public-web-updated.zip', 'NovaDock-'+v+'-continued-source-and-site.zip'}


def write_json(path, data):
    if path.is_symlink():
        raise ValueError('Refusing symlinked metadata: '+path.name)
    temporary = path.with_name(path.name+'.tmp')
    if temporary.exists() or temporary.is_symlink():
        raise ValueError('Metadata temporary file exists: '+temporary.name)
    with temporary.open('x') as stream:
        stream.write(json.dumps(data, indent=2, sort_keys=True)+'\n')
        stream.flush()
        os.fsync(stream.fileno())
    temporary.replace(path)


def seal_assets(out):
    names = required_assets(version()) - {'SHA256SUMS.txt'}
    for name in names:
        payload = (out/name).read_bytes()
        for secret in credentials.known():
            if secret.encode() in payload:
                raise ValueError('A credential appears in release output; remove it and build again.')
    (out/'SHA256SUMS.txt').write_text(''.join(digest(out/name)+'  '+name+'\n' for name in sorted(names)))
    return {name: digest(out/name) for name in sorted(names | {'SHA256SUMS.txt'})}


def build(args):
    cfg = config(args.config)
    out = args.output.resolve()
    if out == ROOT or ROOT in out.parents and not out.is_relative_to(ROOT/'outputs'):
        raise ValueError('Use outputs/release or a build directory outside the source tree.')
    if out.exists() and any(out.iterdir()):
        raise ValueError('Use an empty output directory; preserve failed release evidence.')
    out.mkdir(parents=True, exist_ok=True)
    env = credentials.build_environment(cfg)
    py = env.get('NOVADOCK_RELEASE_PYTHON') or sys.executable
    if not Path(py).is_file():
        raise ValueError('Release Python executable does not exist.')
    env['PYTHON'] = py
    env['PATH'] = str(Path(py).parent)+os.pathsep+env.get('PATH', '')
    if env.get('VBCC'):
        env['PATH'] = str(Path(env['VBCC'])/'bin')+os.pathsep+env.get('PATH', '')
    env['UPLOADER'] = getattr(args, 'uploader', None) or cfg.get('uploader', 'support@retrocs.org')
    env['AUTHOR'] = cfg.get('author', 'NovaDock contributors')
    env['SANITIZE'] = '1'
    env['ND_DEFER_WEB_CHECKS'] = '1'
    env.pop('ND_SKIP_BUILD', None)
    commands = build_commands(out, py)
    # Normalise generated inputs before taking the pre-build fingerprint.
    for command in commands[:-4]:
        run(*command, env=env)
    import package_release as package
    for member, source in package.SDK_FILES:
        target = ROOT/'plugin-sdk'/member
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((ROOT/source).read_bytes())
    inputs = provenance.source_inputs(ROOT)
    for name in inputs:
        payload = (ROOT/name).read_bytes()
        for secret in credentials.known(cfg):
            if secret.encode() in payload:
                raise ValueError('A credential appears in a release input; remove it before building.')
    before = provenance.fingerprint(inputs)
    write_json(out/'build-state.json', {'phase': 'building', 'source_sha256': before, 'inputs': inputs})
    sha = commit()
    if sha:
        commit(clean=True)
        provenance.tracked_inputs(inputs, ROOT)
    run(*commands[-4], env=env)
    run(*commands[-3], env=env)
    after_inputs = provenance.source_inputs(ROOT)
    if provenance.fingerprint(after_inputs) != before:
        changed = sorted(name for name in set(inputs) | set(after_inputs) if inputs.get(name) != after_inputs.get(name))
        raise ValueError('Source changed during the native build: '+', '.join(changed)+'. Finish generation and build again.')
    compiler = env.get('VC') or 'vc'
    executable = shutil.which(compiler, path=env.get('PATH'))
    if not executable:
        raise ValueError('Cannot identify compiler executable; set VC to its executable path.')
    identity = subprocess.run([executable, '-version'], cwd=ROOT, env=env,
                              capture_output=True, text=True, timeout=20)
    record = {'schema': 1, 'version': version(), 'commit': sha, 'prebuilt': False,
              'source_sha256': before, 'inputs': inputs,
              'target': {'name': 'amiga-m68k', 'status': 'available', 'cpu': '68000', 'os': '3.0+'},
              'binaries': provenance.binaries(ROOT),
              'compiler': {'name': Path(executable).name, 'sha256': digest(Path(executable).resolve()),
                           'identity': identity.stdout.strip()[:2000] if identity.returncode == 0 else 'Version probe unavailable; executable identified by SHA-256',
                           'components': {name: digest(Path(env['VBCC'])/'bin'/name)
                               for name in ('vc', 'vbccm68k', 'vasmm68k_mot', 'vlink')
                               if env.get('VBCC') and (Path(env['VBCC'])/'bin'/name).is_file()}},
              'checks': {'host_tests': 'passed', 'native_build': 'passed',
                         'runtime': 'not recorded; complete docs/RELEASE_CHECKLIST.md separately'}}
    proof = ROOT/'build/provenance/amiga.json'
    proof.parent.mkdir(parents=True, exist_ok=True)
    write_json(proof, record)
    write_json(out/'BUILD-INFO.json', record)
    run(*commands[-2], env=env)
    run(*commands[-1], env=env)
    for name in ('source.zip', 'plugin-sdk.zip'):
        shutil.copyfile(ROOT/'public-web/downloads'/f'NovaDock-{version()}-{name}', out/f'NovaDock-{version()}-{name}')
    (out/'RELEASE-NOTES.md').write_text(run(py, 'tools/release_notes.py', capture=True, env=env))
    if provenance.fingerprint(provenance.source_inputs(ROOT)) != before or provenance.binaries(ROOT) != record['binaries']:
        raise ValueError('Release inputs or binaries changed during packaging; build again.')
    write_json(out/'build-state.json', {'assets': seal_assets(out), 'source_sha256': before})
    check_build(out)
    print('Build verified. Run prepare, review manifest.json, then explicitly publish.')


def inventory(directory, assets):
    if set(assets) != required_assets(version()):
        raise ValueError('Incomplete or unexpected release bundle assets.')
    allowed = set(assets) | {'build-state.json', 'manifest.json', 'release.json', 'receipt.json', 'aminet'}
    if directory.is_symlink() or any(p.is_symlink() for p in directory.rglob('*')):
        raise ValueError('Symlinked release assets are not allowed.')
    if {p.name for p in directory.iterdir()} - allowed:
        raise ValueError('Unexpected files in release directory.')
    for name, expected in assets.items():
        if Path(name).name != name or not re.fullmatch(r'[a-f0-9]{64}', expected):
            raise ValueError('Invalid asset manifest entry.')
        if not (directory/name).is_file() or digest(directory/name) != expected:
            raise ValueError('Missing or modified release asset: '+name)
    expected = ''.join(assets[name]+'  '+name+'\n' for name in sorted(set(assets)-{'SHA256SUMS.txt'}))
    if (directory/'SHA256SUMS.txt').read_text() != expected:
        raise ValueError('Checksum inventory differs from the exact release inventory.')


def package_contents(directory):
    """Expected bytes for each package, including the website's downloadable files."""
    import package_release as package
    import generate_info_icons as icons
    previous = icons.ROOT
    icons.ROOT = ROOT
    try:
        drawer = icons.generic_icon('SoftwarePackage.info', kind=2, show_icons=True)
    finally:
        icons.ROOT = previous
    source = {'NovaDock/'+p.relative_to(ROOT).as_posix(): p.read_bytes() for p in package.source_files()}
    source['NovaDock.info'] = drawer
    sdk = {'NovaDock-PluginSDK/'+member: (ROOT/name).read_bytes() for member, name in package.SDK_FILES}
    web = {p.relative_to(ROOT/'public-web').as_posix(): p.read_bytes()
           for p in (ROOT/'public-web').rglob('*') if p.is_file()
           and 'downloads' not in p.relative_to(ROOT/'public-web').parts
           and package.kept(p.relative_to(ROOT/'public-web'))}
    download_names = [f'NovaDock-{version()}-'+suffix for suffix in ('source.zip', 'plugin-sdk.zip')]
    download_names += [f'NovaDock-{version()}'+suffix for suffix in ('.lha', '.readme')]
    for name in download_names:
        web['downloads/'+name] = (directory/name).read_bytes()
    web['downloads/SHA256SUMS.txt'] = ''.join(digest(directory/name)+'  '+name+'\n' for name in download_names).encode()
    full = dict(source)
    full.update({'NovaDock/public-web/'+name: data for name, data in web.items()})
    return {'source.zip': source, 'plugin-sdk.zip': sdk, 'public-web-updated.zip': web,
            'continued-source-and-site.zip': full}


def check_packages(directory):
    from zipfile import ZipFile
    for suffix, expected in package_contents(directory).items():
        with ZipFile(directory/f'NovaDock-{version()}-{suffix}') as archive:
            names = archive.namelist()
            if len(names) != len(set(names)):
                raise ValueError('Duplicate source archive entries.')
            if set(names) != set(expected):
                raise ValueError('Unexpected or missing package contents: '+suffix)
            for name, payload in expected.items():
                if archive.read(name) != payload:
                    raise ValueError('Package content differs from current release: '+name)


def check_build(directory):
    state = load_json(directory/'build-state.json')
    inventory(directory, state['assets'])
    info = load_json(directory/'BUILD-INFO.json')
    inputs = provenance.source_inputs(ROOT)
    current = provenance.fingerprint(inputs)
    if info.get('prebuilt') or info.get('version') != version() or info.get('source_sha256') != current or state.get('source_sha256') != current or info.get('inputs') != inputs:
        raise ValueError('Stale source fingerprint or unverified build; rebuild from current inputs.')
    if provenance.binaries(ROOT) != info['binaries']:
        raise ValueError('Native binaries changed since compilation; rebuild.')
    sha = commit()
    if sha != info['commit']:
        raise ValueError('Source commit changed; rebuild before preparing.')
    if sha:
        commit(clean=True)
        provenance.tracked_inputs(inputs, ROOT)
    check_packages(directory)
    return state, info


def check(args):
    if (args.output/'manifest.json').exists():
        bundle(args.output.resolve())
    else:
        check_build(args.output.resolve())
    print('Source, native build records, exact asset inventory and checksums verified.')
    print('Native runtime validation is a separate release checklist.')


def prepare(args):
    out = args.output.resolve()
    # Compatibility: the previous prepare command also built an empty directory.
    if not (out/'build-state.json').exists():
        build(args)
    state, info = check_build(out)
    cfg = config(args.config)
    targets = destinations(cfg)
    if getattr(args, 'notes', None):
        if (out/'manifest.json').exists():
            raise ValueError('Manifest is immutable; use a fresh release directory for new notes.')
        (out/'RELEASE-NOTES.md').write_text(args.notes.read_text())
        state['assets'] = seal_assets(out)
        write_json(out/'build-state.json', state)
    manifest = {'schema': 2, 'version': version(), 'tag': 'v'+version(),
                'commit': info['commit'], 'source_sha256': info['source_sha256'],
                'prebuilt': False, 'destinations': targets,
                'notes': (out/'RELEASE-NOTES.md').read_text(), 'assets': state['assets'],
                'tag_message': getattr(args, 'tag_message', None) or 'NovaDock '+version()}
    if not manifest['tag_message'].strip() or '\0' in manifest['tag_message']:
        raise ValueError('Tag annotation must not be empty or contain NUL.')
    for secret in credentials.known(cfg):
        if secret in json.dumps(manifest):
            raise ValueError('A credential appears in release metadata; remove it before preparing.')
    for name in ('manifest.json', 'release.json'):
        path = out/name
        if path.is_symlink():
            raise ValueError('Symlinked manifest.')
        if path.exists() and load_json(path) != manifest:
            raise ValueError('Prepared manifest differs; use a new release directory.')
        if not path.exists():
            write_json(path, manifest)
    bundle(out)
    print('Prepared', out, '- nothing uploaded. Review manifest.json and RELEASE-NOTES.md.')
    if not targets:
        print('No repository destinations configured; configure them before preparing a publishable bundle.')


def bundle(directory):
    manifest = load_json(directory/'manifest.json')
    if manifest.get('schema') != 2 or manifest['version'] != version() or manifest['tag'] != 'v'+version():
        raise ValueError('Bundle version/tag or schema does not match VERSION.')
    state, info = check_build(directory)
    if state['assets'] != manifest['assets'] or any(info.get(k) != manifest.get(k) for k in ('commit', 'version', 'source_sha256', 'prebuilt')):
        raise ValueError('Build provenance differs from the release manifest.')
    if manifest['notes'] != (directory/'RELEASE-NOTES.md').read_text():
        raise ValueError('Release notes differ from prepared manifest.')
    if (directory/'release.json').exists() and load_json(directory/'release.json') != manifest:
        raise ValueError('Manifest aliases differ.')
    return manifest


def publish_github(api, project, directory, manifest, dry):
    path = '/repos/'+project+'/releases'
    releases = api.call('GET', path+'?per_page=100')
    release = next((r for r in releases if r['tag_name'] == manifest['tag']), None)
    if release is None:
        release = api.call('GET', path+'/tags/'+Q(manifest['tag']), missing=True)
    if release:
        if release.get('body', '') != (directory/'RELEASE-NOTES.md').read_text():
            raise ValueError('Existing GitHub release notes differ.')
        assets = api.call('GET', path+'/'+str(release['id'])+'/assets?per_page=100')
        existing = {a['name']: a for a in assets}
        if len(existing) != len(assets) or set(existing) - set(manifest['assets']):
            raise ValueError('Unexpected or duplicate assets in GitHub release.')
        for name, sha in manifest['assets'].items():
            if name in existing and existing[name].get('digest') != 'sha256:'+sha:
                raise ValueError('GitHub already has different/unverifiable asset: '+name)
        if not release['draft'] and set(manifest['assets']) - set(existing):
            raise ValueError('Existing published release is incomplete; refusing to modify it.')
    else:
        existing = {}
    if dry:
        return
    if not release:
        release = api.call('POST', path, {'tag_name': manifest['tag'], 'target_commitish': manifest['commit'],
                           'name': 'NovaDock '+manifest['version'], 'body': (directory/'RELEASE-NOTES.md').read_text(),
                           'draft': True, 'prerelease': manifest['version'].startswith('0.')})
    uploads = API('https://uploads.github.com', getattr(api, 'token', None) or os.environ['GITHUB_TOKEN'], github=True)
    for name in manifest['assets']:
        if name not in existing:
            response = uploads.call('POST', path+'/'+str(release['id'])+'/assets?name='+Q(name), upload=directory/name)
            if response.get('digest') != 'sha256:'+manifest['assets'][name]:
                raise ValueError('GitHub upload digest mismatch: '+name)
    if release['draft']:
        api.call('PATCH', path+'/'+str(release['id']), {'draft': False})
    print('GitHub release complete:', manifest['tag'])


def publish_gitlab(api, project, directory, manifest, dry):
    path = '/projects/'+Q(project)
    release_path = path+'/releases/'+Q(manifest['tag'])
    release = api.call('GET', release_path, missing=True)
    links = []
    missing = []
    for name, sha in manifest['assets'].items():
        endpoint = path+'/packages/generic/novadock/'+Q(manifest['version'])+'/'+Q(name)
        data = api.file_digest(endpoint, missing=True)
        if data is None:
            missing.append((name, endpoint))
        elif data != sha:
            raise ValueError('GitLab already has different asset: '+name)
        links.append({'name': name, 'url': api.base+endpoint, 'link_type': 'package'})
    if release and (len(release['assets']['links']) != len(links) or release.get('description', '') != (directory/'RELEASE-NOTES.md').read_text() or missing or {(x['name'], x['url']) for x in release['assets']['links']} !=
                    {(x['name'], x['url']) for x in links}):
        raise ValueError('Existing GitLab release differs; refusing to replace it.')
    if dry:
        return
    for name, endpoint in missing:
        api.call('PUT', endpoint, upload=directory/name)
        uploaded = api.file_digest(endpoint)
        if uploaded != manifest['assets'][name]:
            raise ValueError('GitLab upload verification failed: '+name)
    if not release:
        api.call('POST', path+'/releases', {'tag_name': manifest['tag'], 'name': 'NovaDock '+manifest['version'],
                 'description': (directory/'RELEASE-NOTES.md').read_text(), 'assets': {'links': links}})
    print('GitLab release complete:', manifest['tag'])


def selected_targets(args):
    if getattr(args, 'to', None) and getattr(args, 'target', None):
        raise ValueError('Use --to or --target, not both.')
    values = args.to.split(',') if getattr(args, 'to', None) else (getattr(args, 'target', None) or ['gitlab', 'github'])
    if not values or len(values) != len(set(values)) or set(values) - {'github', 'gitlab', 'aminet'}:
        raise ValueError('Choose distinct destinations from gitlab,github,aminet.')
    return values


def remote_tag(url, tag):
    output = run('git', 'ls-remote', url, 'refs/tags/'+tag, 'refs/tags/'+tag+'^{}', capture=True)
    entries = dict((ref, sha) for sha, ref in (line.split() for line in output.splitlines()))
    return entries.get('refs/tags/'+tag+'^{}', entries.get('refs/tags/'+tag))


def local_tag(tag, sha, create=False, message=None):
    existing = run('git', 'tag', '--list', tag, capture=True).strip()
    if existing:
        if run('git', 'rev-parse', tag+'^{commit}', capture=True).strip() != sha:
            raise ValueError('Local tag points to a different commit.')
        if run('git', 'cat-file', '-t', 'refs/tags/'+tag, capture=True).strip() != 'tag':
            raise ValueError('Existing local release tag must be annotated.')
    elif create:
        run('git', 'tag', '-a', tag, sha, '-m', message or 'NovaDock '+version())


def push_tag(url, tag, sha):
    # Pin the annotated object itself: a moving local ref must not change the upload.
    obj = run('git', 'rev-parse', 'refs/tags/'+tag, capture=True).strip()
    if (run('git', 'cat-file', '-t', obj, capture=True).strip() != 'tag'
            or run('git', 'rev-parse', obj+'^{commit}', capture=True).strip() != sha):
        raise ValueError('Release tag changed before publication.')
    run('git', 'push', '--no-follow-tags', url, obj+':refs/tags/'+tag)
    if remote_tag(url, tag) != sha:
        raise ValueError('Remote tag verification failed.')


def publication_receipt(directory):
    manifest_sha = digest(directory/'manifest.json')
    path = directory/'receipt.json'
    receipt = load_json(path) if path.exists() else {'manifest_sha256': manifest_sha, 'current': None, 'completed': []}
    if receipt.get('manifest_sha256') != manifest_sha:
        raise ValueError('Receipt belongs to a different manifest; preserve it and investigate.')
    if len(receipt.get('completed', [])) != len(set(receipt.get('completed', []))) or set(receipt.get('completed', [])) - {'gitlab', 'github', 'aminet'} or receipt.get('current') not in (None, 'gitlab', 'github', 'aminet'):
        raise ValueError('Invalid publication receipt.')
    return receipt


def aminet_preflight(directory, manifest, lha=None):
    for suffix in ('lha', 'readme'):
        release = directory/('NovaDock-'+manifest['version']+'.'+suffix)
        submission = directory/'aminet'/('novadock.'+suffix)
        if digest(release) != digest(submission):
            raise ValueError('Aminet files differ from the verified repository release.')
    import aminet_release
    return aminet_release.check(directory/'aminet', lha or os.environ.get('LHA', 'lha'))


def publish(args):
    targets = selected_targets(args)
    if not (getattr(args, 'confirm_publish', False) or getattr(args, 'execute', False)):
        raise ValueError('Publication requires --confirm-publish. Use check to validate locally.')
    directory = args.output.resolve()
    m = bundle(directory)
    cfg = config(args.config)
    configured = destinations(cfg)
    if configured != m['destinations']:
        raise ValueError('Configured destinations differ from the prepared manifest.')
    sha = commit(clean=True)
    if not m['commit'] or sha != m['commit']:
        raise ValueError('Bundle must be built from the current committed checkout; rebuild and prepare again.')
    provenance.tracked_inputs(provenance.source_inputs(ROOT), ROOT)
    receipt = publication_receipt(directory)
    adapters = {}
    local_tag(m['tag'], sha)
    # Preflight every selected destination before any tag creation, push or upload.
    for target in targets:
        if target == 'aminet':
            submission, fields = aminet_preflight(directory, m, cfg.get('env', {}).get('LHA'))
            import aminet_release
            status = aminet_release.submission_status(directory/'aminet', submission)
            if receipt['current'] == 'aminet' and status != 'completed':
                raise ValueError('Interrupted Aminet submission requires inspection; do not blindly resubmit.')
            adapters[target] = (submission, fields)
            continue
        if target not in configured:
            raise ValueError('Destination missing from prepared manifest: '+target)
        url, project, origin = repo_url(configured[target]['url'], github=target == 'github')
        token = credentials.token(target, cfg)
        api = API('https://api.github.com' if target == 'github' else origin+'/api/v4', token, github=target == 'github')
        api.call('GET', '/repos/'+project if target == 'github' else '/projects/'+Q(project))
        (publish_github if target == 'github' else publish_gitlab)(api, project, directory, m, True)
        push_url = configured[target]['push_url']
        remote = remote_tag(push_url, m['tag'])
        if remote and remote != sha:
            raise ValueError('Remote tag points to a different commit: '+target)
        adapters[target] = (api, project, push_url)
    # Revalidate after the potentially long remote preflight.
    bundle(directory)
    if commit(clean=True) != sha:
        raise ValueError('Checkout changed during preflight.')
    for target in targets:
        bundle(directory)
        if commit(clean=True) != sha:
            raise ValueError('Checkout changed before publication.')
        receipt['current'] = target
        write_json(directory/'receipt.json', receipt)
        if target == 'aminet':
            import aminet_release
            submission, fields = adapters[target]
            aminet_release.upload(directory/'aminet', submission, fields)
            print('Aminet submitted; moderator acceptance is separate.')
        else:
            api, project, url = adapters[target]
            local_tag(m['tag'], sha, create=True, message=m.get('tag_message'))
            remote = remote_tag(url, m['tag'])
            if remote and remote != sha:
                raise ValueError('Remote tag changed before publication: '+target)
            push_tag(url, m['tag'], sha)
            (publish_github if target == 'github' else publish_gitlab)(api, project, directory, m, False)
        if target not in receipt['completed']:
            receipt['completed'].append(target)
        receipt['current'] = None
        write_json(directory/'receipt.json', receipt)
    print('Selected destinations complete. Receipt:', directory/'receipt.json')


def aminet(args):
    if getattr(args, 'execute', False):
        args.to = 'aminet'
        args.confirm_publish = True
        return publish(args)
    m = bundle(args.output.resolve())
    aminet_preflight(args.output.resolve(), m, config(args.config).get('env', {}).get('LHA'))
    print('Aminet pair verified locally. Publish with --to aminet --confirm-publish to submit.')


def default_config():
    override = os.environ.get('NOVADOCK_RELEASE_CONFIG')
    if override:
        return Path(override).expanduser()
    shared = Path.home()/'Development/tools/git-release/projects/novadock.json'
    local = ROOT/'work/release-config.json'
    return local if local.is_file() and not shared.is_file() else shared


def source(args):
    cfg = config(args.config)
    selected = selected_targets(args)
    targets = destinations(cfg)
    if set(selected) - set(targets):
        raise ValueError('Configure every selected source destination before pushing.')
    release_source.push(ROOT, cfg, {name: targets[name] for name in selected},
                        args.confirm_push, args.dry_run, lambda: commit(clean=True),
                        args.initialize, args.commit_message)


def release(args):
    if not (args.confirm_publish or args.execute):
        raise ValueError('Combined release requires --confirm-publish before preparing or uploading.')
    if not (args.output/'manifest.json').exists():
        prepare(args)
    elif args.notes or args.tag_message:
        manifest = bundle(args.output.resolve())
        if args.notes and args.notes.read_text() != manifest['notes']:
            raise ValueError('Prepared release notes differ; use a fresh output directory.')
        if args.tag_message and args.tag_message != manifest.get('tag_message'):
            raise ValueError('Prepared tag annotation differs; use a fresh output directory.')
    publish(args)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    for name, action in (('plan', plan), ('build', build), ('check', check), ('prepare', prepare),
                         ('publish', publish), ('aminet', aminet), ('source', source), ('release', release)):
        p = sub.add_parser(name)
        p.add_argument('--output', type=Path, default=ROOT/'outputs/release')
        p.add_argument('--config', type=Path, default=default_config())
        if name in ('build', 'prepare', 'release'):
            p.add_argument('--uploader')
        if name in ('prepare', 'release'):
            p.add_argument('--notes', type=Path)
            p.add_argument('--tag-message')
        if name in ('publish', 'release'):
            p.add_argument('--to', help='Comma-separated gitlab,github,aminet (default: gitlab,github)')
            p.add_argument('--target', choices=('github', 'gitlab'), action='append', help=argparse.SUPPRESS)
            p.add_argument('--confirm-publish', action='store_true')
            p.add_argument('--execute', action='store_true', help='Compatibility alias for --confirm-publish')
        if name == 'source':
            p.add_argument('--to', default='gitlab,github')
            p.add_argument('--confirm-push', action='store_true')
            p.add_argument('--dry-run', action='store_true')
            p.add_argument('--init', dest='initialize', action='store_true')
            p.add_argument('--commit-message')
        if name == 'aminet':
            p.add_argument('--execute', action='store_true')
        p.set_defaults(action=action)
    args = parser.parse_args()
    try:
        if args.command == 'source' and args.commit_message and not args.initialize:
            raise ValueError('--commit-message requires source --init.')
        if args.output.is_symlink():
            raise ValueError('Symlinked release directory.')
        args.action(args)
    except (ValueError, KeyError, OSError, subprocess.SubprocessError) as error:
        # Never echo command lines, subprocess output, credentials or remote bodies.
        message = str(error) if isinstance(error, (ValueError, KeyError)) else type(error).__name__+'; check local tools, paths and access.'
        message = credentials.redact(message)
        print('Release stopped:', message, file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
