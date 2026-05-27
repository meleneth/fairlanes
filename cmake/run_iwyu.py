#!/usr/bin/env python3
import argparse
import json
import re
import shlex
import subprocess
import sys
from pathlib import Path


UNSUPPORTED_IWYU_FLAGS = {
    '-fno-diagnostics-show-caret',
    '-ftrack-macro-expansion=0',
}


def source_filter(path: str) -> bool:
    suffix = Path(path).suffix
    return suffix in {'.cc', '.cpp', '.cxx'}


def project_source_filter(path: str) -> bool:
    return path.startswith(('src/', 'tests/'))


def iwyu_command(entry: dict, iwyu: str) -> list[str]:
    if 'arguments' in entry:
        args = list(entry['arguments'])
    else:
        args = shlex.split(entry['command'])

    if not args:
        raise RuntimeError(f"empty compile command for {entry.get('file', '<unknown>')}")

    args[0] = iwyu
    return [arg for arg in args if arg not in UNSUPPORTED_IWYU_FLAGS]


def main() -> int:
    parser = argparse.ArgumentParser(description='Run include-what-you-use over a compile_commands.json database.')
    parser.add_argument('--compile-commands', required=True, type=Path)
    parser.add_argument('--iwyu', required=True)
    parser.add_argument('--root', required=True, type=Path)
    parser.add_argument('--jobs', '-j', type=int, default=1)
    parser.add_argument('--timeout', type=int, default=90)
    parser.add_argument('--only-regex')
    parser.add_argument('--exclude-regex')
    args = parser.parse_args()

    with args.compile_commands.open(encoding='utf-8') as f:
        entries = json.load(f)

    only = re.compile(args.only_regex) if args.only_regex else None
    exclude = re.compile(args.exclude_regex) if args.exclude_regex else None

    commands: list[tuple[str, Path, list[str]]] = []
    seen: set[str] = set()
    root = args.root.resolve()
    for entry in entries:
        file = entry.get('file')
        if not file or not source_filter(file):
            continue
        path = Path(file)
        if path.is_absolute():
            try:
                rel = path.resolve().relative_to(root).as_posix()
            except ValueError:
                continue
        else:
            rel = path.as_posix()
        if not project_source_filter(rel):
            continue
        if rel in seen:
            continue
        if only is not None and not only.search(rel):
            continue
        if exclude is not None and exclude.search(rel):
            continue
        seen.add(rel)
        commands.append((rel, Path(entry.get('directory', args.root)), iwyu_command(entry, args.iwyu)))

    failed = 0
    for index, (rel, cwd, command) in enumerate(commands, start=1):
        print(f'[{index}/{len(commands)}] IWYU {rel}', flush=True)
        try:
            proc = subprocess.run(command, cwd=cwd, text=True, timeout=args.timeout)
        except subprocess.TimeoutExpired:
            print(f'IWYU timed out after {args.timeout}s: {rel}', file=sys.stderr)
            failed += 1
            continue
        if proc.returncode != 0:
            failed += 1

    if failed:
        print(f'IWYU completed with diagnostics/failures in {failed} translation unit(s).', file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
