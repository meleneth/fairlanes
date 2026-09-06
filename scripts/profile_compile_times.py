#!/usr/bin/env python3
"""Replay selected compile commands into isolated artifacts, with compiler timings.

Does not relink or modify the normal build tree. Generate its compile_commands.json
and content artifacts first. Run without another build for comparable timings.
"""
import argparse
import collections
import json
from pathlib import Path
import shlex
import subprocess
import time


def timed_events(events):
    """Normalize complete spans and Clang 22 asynchronous Source spans."""
    starts = collections.defaultdict(list)
    for event in events:
        phase = event.get('ph')
        key = (event.get('pid'), event.get('tid'), event.get('cat'), event.get('id'), event.get('name'))
        if phase in ['b', 'B']:
            starts[key].append(event)
        elif phase in ['e', 'E'] and starts[key]:
            start = starts[key].pop()
            yield dict(start, dur=event['ts'] - start['ts'])
        elif phase == 'X':
            yield event


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--database', type=Path, default=Path('build-linux-debug/compile_commands.json'))
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--summarize-only', action='store_true', help='Re-read an existing capture without compiling')
    parser.add_argument('--mode', choices=['clang', 'gcc'], default='clang')
    parser.add_argument('--file', action='append', required=True, help='Source path suffix; repeat or use ALL for project sources')
    args = parser.parse_args()
    output = args.output.resolve()
    # A new output directory prevents stale traces from contaminating a capture.
    if not args.summarize_only:
        output.mkdir(parents=True, exist_ok=False)
    database = json.loads(args.database.read_text())
    project = Path(__file__).resolve().parent.parent
    selected = [entry for entry in database if
                any(entry['file'].endswith(suffix) for suffix in args.file) or
                ('ALL' in args.file and (entry['file'].startswith(str(project / 'src')) or
                                        entry['file'].startswith(str(project / 'tests')) or
                                        '/generated/fairlanes_content/' in entry['file']))]
    if not selected:
        parser.error('No compile commands matched')
    results = json.loads((output / "results.json").read_text()) if args.summarize_only else []
    replay_database = []
    if args.summarize_only:
        selected = []
    for index, entry in enumerate(selected):
        original = entry.get('arguments') or shlex.split(entry['command'])
        command = [args.compiler]
        skip = False
        for argument in original[1:]:
            if skip:
                skip = False
                continue
            if argument in ['-o', '-MF', '-MT', '-MQ']:
                skip = True
                continue
            if argument in ['-MD', '-MMD', '-MP', '-Werror']:
                continue
            if args.mode == 'clang' and (argument.startswith('-ftrack-macro-expansion=') or argument == '-fno-diagnostics-show-caret'):
                continue
            command.append(argument)
        artifact = output / f'{index:03d}-{Path(entry["file"]).stem}'
        command += ['-o', str(artifact.with_suffix('.o'))]
        if args.mode == 'clang':
            command += [f'-ftime-trace={artifact.with_suffix(".json")}', '-ftime-trace-granularity=500']
        else:
            command += ['-ftime-report']
        replay_database.append(dict(directory=entry['directory'], file=entry['file'], arguments=command))
        start = time.monotonic()
        with artifact.with_suffix('.log').open('w') as log:
            completed = subprocess.run(command, cwd=entry['directory'], stdout=log, stderr=subprocess.STDOUT)
        result = dict(file=entry['file'], seconds=time.monotonic() - start,
                      returncode=completed.returncode, artifact=str(artifact))
        results.append(result)
        print(f'{result["seconds"]:.2f}s [{completed.returncode}] {entry["file"]}', flush=True)
    if not args.summarize_only:
        (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
        (output / 'compile_commands.json').write_text(json.dumps(replay_database, indent=2) + '\n')
    headers, templates, phases = collections.defaultdict(list), collections.defaultdict(list), collections.Counter()
    if args.mode == 'clang':
        for result in results:
            trace = Path(result['artifact']).with_suffix('.json')
            if result['returncode'] or not trace.exists():
                continue
            for event in timed_events(json.loads(trace.read_text()).get('traceEvents', [])):
                name, duration = event.get('name'), event.get('dur', 0) / 1000000
                detail = event.get('args', {}).get('detail', '')
                if name == 'Source':
                    headers[detail].append(duration)
                elif name in ['InstantiateClass', 'InstantiateFunction']:
                    templates[detail].append(duration)
                elif name in ['Total Frontend', 'Total Backend', 'Total ExecuteCompiler']:
                    phases[name] += duration
    lines = ['# Compile-time capture', '',
             f'Compiler: `{args.compiler}`; mode: {args.mode}; sequential compilation.',
             'Warnings are retained but do not fail profiling. No normal build outputs are overwritten.', '',
             'These are measured compilation costs, not predicted savings. Header and template',
             'durations are inclusive, overlap, and must not be added together. Clang costs',
             'are not GCC costs. Traces omit events below 500 microseconds.', '',
             '| Source | Wall seconds | Exit |', '| --- | ---: | ---: |']
    for result in sorted(results, key=lambda row: row['seconds'], reverse=True):
        lines.append(f'| `{result["file"]}` | {result["seconds"]:.2f} | {result["returncode"]} |')
    for title, values in [('Headers (inclusive)', headers), ('Templates (inclusive)', templates)]:
        if not values:
            continue
        lines += ['', f'## {title}', '', '| Name | Aggregate seconds | Recorded occurrences |', '| --- | ---: | ---: |']
        for name, durations in sorted(values.items(), key=lambda item: sum(item[1]), reverse=True)[:25]:
            lines.append(f'| `{name}` | {sum(durations):.3f} | {len(durations)} |')
    if phases:
        lines += ['', '## Compiler phase totals', '']
        lines += [f'- {name}: {duration:.3f}s' for name, duration in phases.items()]
    (output / 'report.md').write_text('\n'.join(lines) + '\n')
    return int(any(result['returncode'] for result in results))


if __name__ == '__main__':
    raise SystemExit(main())
