#!/usr/bin/env python3
"""Keep cross-compiler intermediate files in a private, ignored build directory."""
import argparse
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile


def compile_command(compiler, arguments, root):
    root = Path(root).resolve()
    parent = root/'build'/'intermediates'
    parent.mkdir(parents=True, exist_ok=True)
    # Different programs compile shared C sources concurrently. A workspace per
    # invocation prevents their assembly/object outputs from overwriting each other.
    with tempfile.TemporaryDirectory(prefix='compile-', dir=parent) as temporary:
        workspace = Path(temporary)
        command = shlex.split(compiler)
        if not command:
            raise ValueError('Compiler command is empty')
        if '/' in command[0] and not Path(command[0]).is_absolute():
            command[0] = str(root/command[0])
        rewritten = []
        i = 0
        while i < len(arguments):
            value = arguments[i]
            if value in ('-o', '-I', '-L', '-include'):
                if i+1 == len(arguments):
                    raise ValueError('Missing path after '+value)
                rewritten += [value, str((root/arguments[i+1]).resolve())]
                i += 2
                continue
            if value.startswith(('-I', '-L')) and len(value) > 2:
                rewritten.append(value[:2]+str((root/value[2:]).resolve()))
            elif value.endswith(('.c', '.asm', '.o', '.a', '.lib')):
                source = (root/value).resolve()
                if source.suffix in ('.c', '.asm'):
                    try:
                        relative = source.relative_to(root)
                    except ValueError:
                        relative = Path('external')/str(i)/source.name
                    staged = workspace/relative
                    staged.parent.mkdir(parents=True, exist_ok=True)
                    if source.suffix == '.c':
                        # Include the original translation unit so its quoted
                        # headers, __FILE__ and debug line numbers retain meaning.
                        escaped = str(source).replace('\\', '\\\\').replace('"', '\\"')
                        staged.write_text('#include "'+escaped+'"\n')
                    else:
                        shutil.copyfile(source, staged)
                    rewritten.append(str(staged))
                else:
                    rewritten.append(str(source))
            else:
                rewritten.append(value)
            i += 1
        return subprocess.run(command+rewritten, cwd=workspace).returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', required=True)
    parser.add_argument('arguments', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    arguments = args.arguments[1:] if args.arguments[:1] == ['--'] else args.arguments
    try:
        return compile_command(args.compiler, arguments, Path.cwd())
    except (OSError, ValueError) as error:
        parser.exit(1, 'NovaDock compile failed: '+str(error)+'\n')


if __name__ == '__main__':
    raise SystemExit(main())
