#!/usr/bin/env python3
"""Static checks for the Install-NovaDock Installer script.

The Commodore Installer cannot run on the build host, so this parses the
script and checks what would otherwise only fail on an Amiga: balanced
parentheses, terminated strings, known function names (Installer 42, OS 3.1,
has no procedure arguments), prompt/help/choices on every question,
variables set before use, and that every package file the script copies or
runs is really in the package.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'Install-NovaDock'

FUNCTIONS = {
    # control flow and data
    'set', 'if', 'while', 'until', 'select', 'procedure', 'foreach', 'trap',
    'onerror', 'cat', 'tackon', 'pathonly', 'fileonly', 'expandpath',
    'exists', 'getsize', 'getdiskspace', 'getenv', 'getversion', 'earlier',
    'substr', 'strlen', 'patmatch', 'symbolset', 'symbolval', 'database',
    '=', '<>', '>', '<', '>=', '<=', '+', '-', '*', '/', 'and', 'or', 'not',
    'xor', 'bitand', 'bitor', 'bitxor', 'bitnot', 'shiftleft', 'shiftright',
    'in', 'user', 'debug', 'transcript',
    # user interface
    'welcome', 'message', 'working', 'complete', 'exit', 'abort', 'askbool',
    'askchoice', 'askoptions', 'askdir', 'askfile', 'asknumber', 'askstring',
    'askdisk',
    # actions
    'copyfiles', 'copylib', 'makedir', 'makeassign', 'delete', 'rename',
    'protect', 'run', 'execute', 'rexx', 'startup', 'textfile', 'tooltype',
    'iconinfo',
}
PARAMETERS = {
    'prompt', 'help', 'choices', 'default', 'source', 'dest', 'all',
    'pattern', 'files', 'infos', 'noposition', 'optional', 'confirm', 'safe',
    'quiet', 'noreq', 'append', 'include', 'command', 'setdefaulttool',
    'settooltype', 'newname', 'disk', 'assigns', 'override', 'nogauge',
    'delopts', 'back', 'fonts', 'swapcolors', 'range', 'newpath', 'resident',
}
QUESTIONS = {'askbool', 'askchoice', 'askoptions'}
# Built at packaging time and guarded by (exists ...) in the script.
OPTIONAL = {'Catalogs'}
SOURCE_ONLY = '--source-only' in sys.argv
BUILT_FILES = {'NovaDock', 'NovaDockPrefs', 'SaveConfig', 'MakeIcon', 'StartInstaller', 'SetupStartup'}


def tokenize(text):
    tokens, i, line = [], 0, 1
    while i < len(text):
        c = text[i]
        if c == '\n':
            line += 1
        if c in ' \t\r\n':
            i += 1
        elif c == ';':
            while i < len(text) and text[i] != '\n':
                i += 1
        elif c in '()':
            tokens.append((c, line))
            i += 1
        elif c == '"':
            j, value = i + 1, ''
            while True:
                assert j < len(text) and text[j] != '\n', \
                    f'line {line}: unterminated string'
                if text[j] == '\\':
                    value += text[j:j + 2]
                    j += 2
                    continue
                if text[j] == '"':
                    break
                value += text[j]
                j += 1
            tokens.append(('str', line, value))
            i = j + 1
        else:
            j = i
            while j < len(text) and text[j] not in ' \t\r\n();"':
                j += 1
            tokens.append(('sym', line, text[i:j]))
            i = j
    return tokens


def parse(tokens):
    stack, root = [], []
    for tok in tokens:
        if tok[0] == '(':
            node = {'line': tok[1], 'items': []}
            (stack[-1]['items'] if stack else root).append(node)
            stack.append(node)
        elif tok[0] == ')':
            assert stack, f'line {tok[1]}: unbalanced ")"'
            stack.pop()
        else:
            assert stack, f'line {tok[1]}: value outside an expression'
            stack[-1]['items'].append(tok)
    assert not stack, f'line {stack[-1]["line"]}: unclosed "("'
    return root


def walk(nodes):
    for node in nodes:
        if isinstance(node, dict):
            yield node
            yield from walk(node['items'])


def head(node):
    first = node['items'][0] if node['items'] else None
    return first[2].lower() if isinstance(first, tuple) and first[0] == 'sym' else None


def check_maintenance_detection(text, nodes):
    """Execute the real detection expressions against representative disks."""
    detection = text.split('; An installed application,', 1)[1].split('; Remove the NovaDock:', 1)[0]
    detection = parse(tokenize('; An installed application,' + detection))
    app = 'DH0:Utilities/NovaDock'
    shared = 'DH0:NovaDock'
    package = 'DH1:NovaDock'
    def scenario(files, assigns=None):
        assigns = {'SYS': 'DH0:', 'HELP': 'DH0:Locale/Help', **(assigns or {})}
        env = {'#appdir': 'SYS:Utilities', '#app': 'SYS:Utilities/NovaDock', '#prefs': 'SYS:Prefs',
               '#standard-dest': 'SYS:NovaDock', '#src': package,
               '#helpdir': 'HELP:'}
        def resolve(path):
            if ':' not in path: return path
            name, tail = path.split(':', 1)
            if name in assigns:
                base = assigns[name]
                return base + ('' if base.endswith(':') or not tail else '/') + tail
            return path
        def evaluate(node):
            if isinstance(node, tuple):
                if node[0] == 'str': return node[2]
                if node[2] in env: return env[node[2]]
                return int(node[2])
            name = head(node)
            args = node['items'][1:]
            if name is None:
                result = None
                for n in node['items']: result = evaluate(n)
                return result
            if name == 'set': env[args[0][2]] = evaluate(args[1]); return env[args[0][2]]
            if name == 'if':
                return evaluate(args[1]) if evaluate(args[0]) else (evaluate(args[2]) if len(args)>2 else None)
            if name == 'and': return all(evaluate(n) for n in args)
            if name == 'or': return any(evaluate(n) for n in args)
            if name == 'noreq': return None
            values = [evaluate(n) for n in args]
            if name == '=': return values[0] == values[1]
            if name == '<>': return values[0] != values[1]
            if name == 'expandpath': return resolve(values[0])
            if name == 'pathonly': return values[0].rsplit('/', 1)[0]
            if name == 'tackon': return values[0] + ('' if values[0].endswith(':') else '/') + values[1]
            if name == 'cat': return ''.join(str(v) for v in values)
            if name == 'exists':
                path = resolve(values[0])
                if path in files: return files[path]
                if any(f.startswith(path.rstrip('/')+'/') for f in files): return 2
                return 0
            raise AssertionError('unhandled detection operation: '+name)
        for node in detection: evaluate(node)
        return env
    # Incomplete installs still resolve their shared-data directory without
    # asking the user to find it manually.
    for component in ('Themes', 'Tools'):
        partial = scenario({app: 1, shared+'/'+component: 2})
        assert partial['#installed'] and partial['#current'] == shared
        partial_custom = scenario({'Work:Apps/NovaDock': 1, 'Work:Shared/NovaDock/'+component: 2},
                                  {'NovaDockApp': 'Work:Apps', 'NovaDock': 'Work:Shared/NovaDock'})
        assert partial_custom['#current'] == 'Work:Shared/NovaDock'
        partial_legacy = scenario({app: 1, 'DH0:Resources/NovaDock/'+component: 2})
        assert partial_legacy['#current'] == 'DH0:Resources/NovaDock'
    # Only the explicit confirmation and keep/remove-settings question are
    # reachable in removal; installation's path chooser cannot be called.
    removal = next(n for n in nodes if head(n) == 'procedure' and n['items'][1][2] == 'P_REMOVE')
    procedures = {n['items'][1][2].lower(): n for n in nodes if head(n) == 'procedure'}
    seen = set()
    def check_removal_calls(proc):
        name = proc['items'][1][2]
        if name in seen: return
        seen.add(name)
        for node in walk([proc]):
            assert head(node) not in ('askdir', 'askfile', 'p_install'), 'uninstall reached a path chooser'
            if head(node) in procedures: check_removal_calls(procedures[head(node)])
    check_removal_calls(removal)
    assert sum(head(n) == 'askbool' for n in walk([removal])) == 2
    clean = scenario({})
    assert not clean['#installed']
    normal = scenario({app: 1, shared+'/Plugins': 2, 'DH0:Locale/Help/NovaDock.guide': 1})
    assert normal['#installed'] and normal['#installed-app'] == app
    assert normal['#current'] == shared and app in normal['#locations']
    legacy = scenario({app+'/NovaDock': 1, app+'/Plugins': 2, app+'/NovaDock.guide': 1})
    assert legacy['#installed'] and legacy['#installed-app'] == app+'/NovaDock'
    assert legacy['#current'] == app and legacy['#oldhelpdir'] == app and legacy['#helpdir'] == 'HELP:'
    custom = scenario({'Work:Dock/NovaDock': 1, 'Work:Dock/Plugins': 2,
                       'Work:Manuals/NovaDock.guide': 1},
                      {'NovaDock': 'Work:Dock', 'NovaDockHelp': 'Work:Manuals'})
    assert custom['#installed'] and custom['#current'] == 'Work:Dock'
    assert custom['#oldhelpdir'] == 'Work:Manuals' and custom['#helpdir'] == 'HELP:'
    selected = scenario({'Work:Apps/NovaDock': 1, 'Work:Shared/NovaDock/Plugins': 2,
                         'Work:Prefs/NovaDockPrefs': 1, 'Work:Docs/NovaDock.guide': 1},
                        {'NovaDockApp': 'Work:Apps', 'NovaDockPrefs': 'Work:Prefs',
                         'NovaDock': 'Work:Shared/NovaDock', 'NovaDockHelp': 'Work:Docs'})
    assert selected['#installed-app'] == 'Work:Apps/NovaDock'
    assert selected['#appdir'] == 'Work:Apps' and selected['#prefs'] == 'Work:Prefs'
    assert selected['#current'] == 'Work:Shared/NovaDock'
    assert 'Work:Prefs/NovaDockPrefs' in selected['#locations']
    previous = scenario({app: 1, 'DH0:Resources/NovaDock/Plugins': 2})
    assert previous['#current'] == 'DH0:Resources/NovaDock'
    nested = scenario({app: 1, shared+'/Plugins': 2,
                       'DH0:Locale/Help/NovaDock/NovaDock.guide': 1})
    assert nested['#oldhelpdir'] == 'HELP:NovaDock'
    assert nested['#displayhelp'] == 'HELP:NovaDock'
    both = scenario({app: 1, shared+'/Plugins': 2,
                     'DH0:Locale/Help/NovaDock/NovaDock.guide': 1,
                     'DH0:Locale/Help/NovaDock.guide': 1})
    assert both['#displayhelp'] == 'HELP:' 
    assert scenario({app: 1})['#installed']  # damaged/missing shared files
    damaged = scenario({app: 1, 'DH0:Utilities/Plugins': 2})
    assert damaged['#installed'] and damaged['#current'] == ''  # never claim an app parent as shared data
    assert not scenario({shared+'/Plugins': 2})['#installed']  # leftovers
    assert not scenario({package+'/NovaDock': 1, package+'/Plugins': 2},
                        {'NovaDock': package})['#installed']  # unpacked package
    install = next(n for n in nodes if head(n) == 'procedure' and n['items'][1][2] == 'P_INSTALL')
    choose = install['items'][2]['items'][0]
    assert head(choose) == 'if' and choose['items'][1][2] == '#installed'
    assert not any(head(n) == 'askdir' for n in walk([choose['items'][2]]))
    assert sum(head(n) == 'askdir' for n in walk([choose['items'][3]])) == 3
    final = nodes[-1]
    assert head(final) == 'if' and final['items'][1][2] == '#installed'
    maintenance = next(n for n in walk([final['items'][2]]) if head(n) == 'askchoice')
    choices = next(n for n in maintenance['items'] if isinstance(n, dict) and head(n) == 'choices')
    assert [n[2] for n in choices['items'][1:]] == ['Update NovaDock', 'Uninstall NovaDock']
    print('Installer maintenance: fresh, split, legacy, custom binary drawers, damaged, leftover and package-only detection passed.')


def check_installation_levels(nodes):
    """Exercise the actual path/options branches at all three user levels."""
    install = next(n for n in nodes if head(n) == 'procedure' and n['items'][1][2] == 'P_INSTALL')
    statements = install['items'][2]['items']
    path_choice = statements[0]
    options_default = next(n for n in statements if head(n) == 'set' and n['items'][1][2] == '#options')
    options_choice = next(n for n in statements if head(n) == 'if' and any(head(c) == 'askbool' for c in walk([n])))
    reset_default = next(n for n in statements if head(n) == 'set' and n['items'][1][2] == '#reset')
    reset_choice = next(n for n in statements if head(n) == 'if' and any(head(c) == 'askchoice' for c in walk([n])))
    from itertools import product
    for level, boot, launch in product((0, 1, 2), (0, 1), (0, 1)):
        env = {'@user-level': level, '#installed': 0, '#appdir': 'SYS:Utilities',
               '#app': 'SYS:Utilities/NovaDock', '#prefs': 'SYS:Prefs',
               '#current': '', '#config': 'ENVARC:NovaDock/novadock.cfg'}
        questions = []
        def evaluate(node):
            if isinstance(node, tuple):
                if node[0] == 'str': return node[2]
                return env[node[2]] if node[2] in env else int(node[2])
            name, args = head(node), node['items'][1:]
            if name is None:
                for child in node['items']: evaluate(child)
                return
            if name == 'set': env[args[0][2]] = evaluate(args[1]); return env[args[0][2]]
            if name == 'if':
                return evaluate(args[1]) if evaluate(args[0]) else (evaluate(args[2]) if len(args)>2 else None)
            if name == 'and': return all(evaluate(a) for a in args)
            if name == 'or': return any(evaluate(a) for a in args)
            if name in ('askdir', 'askbool', 'askchoice'):
                questions.append(name)
                if name == 'askdir':
                    return ['Work:Apps', 'Work:Prefs', 'Work:Shared'][questions.count('askdir')-1]
                if name == 'askbool':
                    return boot if questions.count('askbool') == 1 else launch
                return evaluate(next(a for a in args if isinstance(a, dict) and head(a) == 'default')['items'][1])
            values = [evaluate(a) for a in args]
            if name == '+': return values[0] + values[1]
            if name == '>': return values[0] > values[1]
            if name == '=': return values[0] == values[1]
            if name == 'exists': return 1  # Existing settings must be preserved in Novice.
            if name == 'tackon': return values[0] + ('' if values[0].endswith(':') else '/') + values[1]
            if name == 'expandpath': return values[0]
            if name == 'pathonly': return values[0].rsplit('/', 1)[0]
            raise AssertionError('unhandled installation operation: ' + str(name))
        for node in (path_choice, options_default, options_choice, reset_default, reset_choice): evaluate(node)
        assert env['#app'] == ('SYS:Utilities/NovaDock' if level == 0 else 'Work:Apps/NovaDock')
        assert env['#prefs'] == ('SYS:Prefs' if level == 0 else 'Work:Prefs')
        assert env['#dest'] == ('Work:Shared/NovaDock' if level == 2 else 'SYS:NovaDock')
        assert env['#options'] == (3 if level == 0 else boot + 2*launch)
        assert env['#reset'] == 0
        assert questions == ([] if level == 0 else ['askdir'] * (3 if level == 2 else 2) + ['askbool', 'askbool', 'askchoice'])
    startup_questions = [n for n in walk([options_choice]) if head(n) == 'askbool']
    prompts = [next(c for c in n['items'] if isinstance(c, dict) and head(c) == 'prompt')['items'][1][2] for n in startup_questions]
    assert prompts == ['Start NovaDock automatically with Workbench?', 'Start NovaDock when installation finishes?']
    launcher = (ROOT/'tools/startinstaller.c').read_text()
    assert 'MINUSER NOVICE DEFUSER NOVICE' in launcher
    icon = (ROOT/'Install-NovaDock.info').read_bytes()
    assert b'MINUSER=NOVICE\0' in icon and b'DEFUSER=NOVICE\0' in icon
    print('Installer levels: 0/2/3 path questions; independent boot/launch choices checked in all 12 scenarios.')


def check_maintenance_levels(nodes):
    """Model Installer's suppressed Novice prompts and real user-level changes."""
    final = nodes[-1]
    removal = next(n for n in nodes if head(n) == 'procedure' and n['items'][1][2] == 'P_REMOVE')
    class Stopped(Exception): pass
    def scenario(level, action=0, remove=0, keep=1, uninstall=False):
        env = {'@user-level': level, '#installed': 1, '#locations': 'Detected locations',
               '@askchoice-help': '', '@askbool-help': ''}
        asked, actions = [], []
        def evaluate(node):
            if isinstance(node, tuple):
                if node[0] == 'str': return node[2]
                return env[node[2]] if node[2] in env else int(node[2])
            name, args = head(node), node['items'][1:]
            if name is None:
                for child in node['items']: evaluate(child)
                return
            if name == 'set': env[args[0][2]] = evaluate(args[1]); return env[args[0][2]]
            if name == 'user': env['@user-level'] = evaluate(args[0]); return
            if name == 'if':
                return evaluate(args[1]) if evaluate(args[0]) else (evaluate(args[2]) if len(args)>2 else None)
            if name == '=': return evaluate(args[0]) == evaluate(args[1])
            if name in ('askchoice', 'askbool'):
                # Without (user 1), the Amiga Installer would take the default.
                assert env['@user-level'] > 0, 'Novice maintenance question would be suppressed'
                asked.append(name)
                if name == 'askchoice':
                    default = next(a for a in args if isinstance(a, dict) and head(a) == 'default')
                    assert evaluate(default['items'][1]) == 0
                    return action
                return remove if asked.count('askbool') == 1 else keep
            if name in ('p_install', 'p_remove', 'p_stop_dock', 'exit'):
                actions.append(name.upper() if name.startswith('p_') else name)
                assert env['@user-level'] == level, 'chosen level was not restored'
                raise Stopped()
            if name == 'run': return 0  # only the trace header occurs before P_STOP_DOCK
            raise AssertionError('unhandled maintenance operation: ' + str(name))
        try: evaluate(removal['items'][2] if uninstall else final)
        except Stopped: pass
        return env, asked, actions
    for level in (0, 1, 2):
        for action, expected in enumerate(('P_INSTALL', 'P_REMOVE')):
            env, asked, actions = scenario(level, action=action)
            assert asked == ['askchoice'] and actions == [expected]
        env, asked, actions = scenario(level, uninstall=True, remove=0)
        assert asked == ['askbool'] and actions == ['exit']
        for keep in (0, 1):
            env, asked, actions = scenario(level, uninstall=True, remove=1, keep=keep)
            assert asked == ['askbool', 'askbool'] and actions == ['P_STOP_DOCK']
            assert env['#settings'] == keep
    print('Maintenance: Update/Uninstall, removal consent and settings choices pass at all three levels.')


def main():
    nodes = parse(tokenize(SCRIPT.read_text('latin-1')))
    check_maintenance_detection(SCRIPT.read_text('latin-1'), nodes)
    check_installation_levels(nodes)
    check_maintenance_levels(nodes)
    procedures = {head_node['items'][1][2] for head_node in walk(nodes)
                  if head(head_node) == 'procedure'}
    for proc in walk(nodes):
        if head(proc) == 'procedure':
            name, rest = proc['items'][1], proc['items'][2:]
            # Installer 42 procedures take no arguments.
            assert not (rest and isinstance(rest[0], tuple)), \
                f'line {proc["line"]}: procedure {name[2]} has arguments'
    assigned = {'@app-name', '@default-dest', '@icon', '@each-name',
                '@each-type', '@user-level', '@askoptions-help',
                '@askchoice-help', '@askbool-help', '@copyfiles-help',
                '@askdir-help', '@startup-help'}
    for node in walk(nodes):
        if head(node) == 'set':
            assigned.add(node['items'][1][2])
    checked = 0
    for node in walk(nodes):
        name = head(node)
        if name is None:
            continue                       # a block of statements
        assert (name in FUNCTIONS or name in PARAMETERS or
                node['items'][0][2] in procedures), \
            f'line {node["line"]}: unknown function "{name}"'
        if name in QUESTIONS:
            parts = {head(n) for n in node['items'] if isinstance(n, dict)}
            for required in ('prompt', 'help', 'choices'):
                assert required in parts, \
                    f'line {node["line"]}: {name} without ({required})'
        for item in node['items'][1:]:
            if isinstance(item, tuple) and item[0] == 'sym' and \
                    item[2][0] in '#@':
                assert item[2] in assigned, \
                    f'line {item[1]}: {item[2]} is never set'
        # (tackon #src "X") names a file or drawer that must be in the package.
        args = node['items'][1:]
        if name == 'tackon' and len(args) == 2 and \
                isinstance(args[0], tuple) and args[0][2] == '#src' and \
                isinstance(args[1], tuple) and args[1][0] == 'str' and \
                args[1][2] not in OPTIONAL:
            target = ROOT / node['items'][2][2]
            assert target.exists() or (SOURCE_ONLY and (target.name in BUILT_FILES or target.parent.name == "Plugins") and target.suffix == ""), \
                f'line {node["line"]}: package lacks {node["items"][2][2]}'
            checked += 1
    # Installation must only copy icons that were generated on the host.
    # Running MakeIcon on the Amiga made installation unnecessarily slow.
    text = SCRIPT.read_text('latin-1')
    assert '#makeicon' not in text.lower(), 'installer still builds plugin icons on the Amiga'
    assert not re.search(r'\(run[^)]*makeicon', text, re.I | re.S), \
        'installer still runs MakeIcon on the Amiga'
    plugin_icons = (
        'Activity', 'AmIRC', 'ArchiveExtractor', 'BitSkim', 'Clipboard',
        'Clock', 'Command', 'DOpus4', 'Eyes', 'Memory', 'MusicPlayer',
        'Network', 'RunningApps',
    )
    for plugin in plugin_icons:
        assert SOURCE_ONLY or (ROOT / 'Plugins' / plugin).exists(), f'no Plugins/{plugin}'
        assert SOURCE_ONLY or (ROOT / 'Plugins' / f'{plugin}.info').exists(), \
            f'no prebuilt Plugins/{plugin}.info'
        checked += 1
    # Split layout: copy executables to their system drawers and guide to
    # its selected directory. Shared components remain in a dedicated drawer.
    copies = [node for node in walk(nodes) if head(node) == 'copyfiles']
    def parameters(node, name):
        return [n['items'][1:] for n in node['items']
                if isinstance(n, dict) and head(n) == name]
    def source_name(node):
        source = parameters(node, 'source')
        if not source or not isinstance(source[0][0], dict): return None
        return source[0][0]['items'][-1][2]
    def destination(node):
        dest = parameters(node, 'dest')
        return dest[0][0][2] if dest and isinstance(dest[0][0], tuple) else None
    expected = {'NovaDock': '#appdir', 'NovaDockPrefs': '#prefs',
                'NovaDock.guide': '#helpdir'}
    for source, dest in expected.items():
        assert any(source_name(c) == source and destination(c) == dest for c in copies), source
    assert '(set #app "SYS:Utilities/NovaDock")' in text
    assert '(set #standard-dest "SYS:NovaDock")' in text
    assert '(set #helpdir "HELP:")' in text
    helper = next(c for c in copies if source_name(c) == 'SaveConfig')
    target = parameters(helper, 'dest')[0][0]
    assert head(target) == 'tackon' and [a[2] for a in target['items'][1:]] == ['#dest', 'Tools']
    assert '(makedir #dest (infos))' not in text
    assert '(set #del (cat #dest ".info")) (P_DELETE)' in text
    assert not any(source_name(c) == 'Icons/SoftwarePackage.info' and destination(c) == '#parent' for c in copies)
    assert not any(source_name(c) == 'Icons/SoftwarePackage.info' and destination(c) == 'HELP:' for c in copies)
    assert any(source_name(c) == 'Icons/SoftwarePackage.info' and destination(c) == 'ENVARC:' for c in copies)
    assert 'Tools/SaveConfig' in (ROOT/'src/dock_save.c').read_text()
    shell = (ROOT/'install.amiga').read_text()
    assert 'Copy SaveConfig TO SYS:NovaDock/Tools/SaveConfig' in shell
    assert 'Copy Icons/SoftwarePackage.info TO SYS:NovaDock.info' not in shell
    assert 'Copy NovaDock.guide TO HELP:NovaDock.guide' in shell
    assert shell.index('Copy SYS:Resources/NovaDock/Themes') < shell.index('Copy Themes/Mocha/theme.cfg')
    assert '(set #app (tackon #appdir "NovaDock"))' in text
    assert '(makeassign "NovaDockApp" #appdir)' in text
    assert '(makeassign "NovaDockPrefs" #prefs)' in text
    assert '(makeassign "NovaDockHelp" #helpdir)' not in text
    assert '(makeassign "NovaDock" #dest)' in text
    assert '(rename #app (cat #app ".previous"))' in text
    # Removal must never recursively delete the shared HELP directory.
    assert 'Delete \"" #helpdir' not in text
    assert '(set #del (tackon #helpdir "NovaDock.guide"))' in text
    assert 'Delete HELP: ALL' not in text and 'Delete HELP: QUIET' not in text
    assert 'Delete HELP:NovaDock ALL' not in text
    assert '(set #del "HELP:NovaDock/NovaDock.guide")' in text
    assert 'Delete ENV:NovaDock ALL' not in text
    assert text.index('(set #del "ENV:NovaDock/novadock.cfg")') < text.index('Delete ENVARC:NovaDock ALL')
    # The project uses the package launcher, which selects the system Installer.
    assert SOURCE_ONLY or (ROOT / "StartInstaller").exists()
    icon = (ROOT / 'Install-NovaDock.info').read_bytes()
    assert icon[48] == 4 and b'StartInstaller\0' in icon and b'APPNAME=NovaDock\0' in icon
    print(f'Installer script: syntax, functions, questions, variables and '
          f'{checked} package files checked.')


if __name__ == '__main__':
    try:
        main()
    except AssertionError as error:
        print(f'Install-NovaDock: {error}', file=sys.stderr)
        sys.exit(1)
