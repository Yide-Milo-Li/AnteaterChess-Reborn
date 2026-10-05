"""Verify CPack candidates, portable startup/logging, and no-Git source rebuilds."""
from pathlib import Path, PurePosixPath
import argparse
import concurrent.futures
import hashlib
import json
import os
import shutil
import subprocess
import tarfile
import time
import re
import threading
import itertools
import zipfile
from manifest import digest, pe_info, windows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--rebuild', action='store_true')
    args = parser.parse_args()
    work = args.work.resolve()
    if work.exists():
        raise RuntimeError('Use a new evidence directory; previous results are preserved')
    work.mkdir(parents=True)
    records = []
    receipt_lock = threading.RLock()
    command_ids = itertools.count()

    def run(command, cwd, env=None, expected=0):
        started = time.time()
        result = subprocess.run([str(item) for item in command], cwd=cwd, env=env,
                                capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=900)
        with receipt_lock:
            command_id = next(command_ids)
        log = work/f'command-{command_id:02}.txt'
        log.write_text(result.stdout+'\n'+result.stderr, encoding='utf-8')
        record = {'command': [str(item) for item in command], 'cwd': str(cwd),
                        'startedUnix': started, 'exitCode': result.returncode, 'expectedExitCode': expected,
                        'environment': {key: value for key, value in (env or os.environ).items()
                                        if key.startswith(('QT_', 'QML_', 'AC_')) or key == 'PATH'},
                        'log': log.name, 'sha256': digest(log)}
        with receipt_lock:
            records.append(record)
            save()
        if result.returncode != expected:
            raise RuntimeError(f'Command failed: {command}; see {log}')

    def save():
        receipt = {'runtime': {'file': str(args.runtime), 'sha256': digest(args.runtime)},
                   'source': {'file': str(args.source), 'sha256': digest(args.source)},
                   'checks': records, 'externalAcceptance': 'pending'}
        (work/'RESULT.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')

    runtime, source = work/'runtime 棋', work/'source 棋'
    for archive, destination in ((args.runtime, runtime), (args.source, source)):
        destination.mkdir()
        prefix = archive.name.removesuffix('.zip').removesuffix('.tar.gz')

        def relative(name):
            path = PurePosixPath(name)
            if path.is_absolute() or '..' in path.parts or not path.parts or path.parts[0] != prefix:
                raise RuntimeError('Unsafe archive path or unexpected package root')
            return Path(*path.parts[1:])

        # Strip only the known CPack root. Short Unicode/space working names
        # keep ordinary Windows paths bounded without changing machine policy.
        if archive.suffix == '.zip':
            with zipfile.ZipFile(archive) as contents:
                for member in contents.infolist():
                    path = destination/relative(member.filename)
                    if not path.resolve().is_relative_to(destination):
                        raise RuntimeError('Unsafe archive path')
                    if member.is_dir():
                        path.mkdir(parents=True, exist_ok=True)
                    else:
                        path.parent.mkdir(parents=True, exist_ok=True)
                        with contents.open(member) as incoming, path.open('wb') as outgoing:
                            shutil.copyfileobj(incoming, outgoing)
        else:
            with tarfile.open(archive) as contents:
                members = [member.replace(name=relative(member.name).as_posix())
                           for member in contents.getmembers() if relative(member.name) != Path('.')]
                contents.extractall(destination, members=members, filter='data')
    if (source/'.git').exists():
        raise RuntimeError('Source archive includes Git')
    for tree in (source, runtime):
        for line in (tree/'FILES.sha256').read_text(encoding='utf-8').splitlines():
            expected, name = line.split('  ', 1)
            path = tree/name
            if not path.resolve().is_relative_to(tree.resolve()) or digest(path) != expected:
                raise RuntimeError(f'Archive file hash mismatch: {name}')
    revision = (source/'SOURCE_REVISION').read_text().strip()
    if not re.fullmatch('[0-9a-f]{40}', revision):
        raise RuntimeError('Formal candidates require an identified clean source commit')
    manifest = json.loads((runtime/'DEPENDENCIES.json').read_text())
    if manifest['sourceCommit'] != revision or (runtime/'SOURCE_REVISION').read_text().strip() != revision:
        raise RuntimeError('Source/runtime commits differ')
    for name, expected in manifest['inventory'].items():
        if digest(runtime/name) != expected:
            raise RuntimeError(f'Dependency hash mismatch: {name}')
    inventory = json.loads((source/'docs/development/native-migration/protected-files.json').read_text())
    changes = json.loads((source/'docs/development/native-migration/structural-changes.json').read_text())
    overrides = {item['path']: item['sha256After'] for item in changes['files']}
    for item in inventory['files']:
        if digest(source/item['path']) != overrides.get(item['path'], item['sha256']):
            raise RuntimeError(f'Protected source changed: {item["path"]}')
    executable = runtime/('anteater-chess.exe' if os.name == 'nt' else 'anteater-chess')
    env = os.environ.copy()
    for key in ('QTDIR', 'Qt6_DIR', 'QT_PLUGIN_PATH', 'QML_IMPORT_PATH', 'QML2_IMPORT_PATH', 'CMAKE_PREFIX_PATH'):
        env.pop(key, None)
    env['QT_QUICK_BACKEND'] = 'software'
    env['QT_QUICK_CONTROLS_STYLE'] = 'Basic'
    env['QT_QPA_PLATFORM'] = 'windows' if os.name == 'nt' else env.get('QT_QPA_PLATFORM', 'offscreen')
    env['PATH'] = str(Path(os.environ['SystemRoot'])/'System32') if os.name == 'nt' else '/usr/bin:/bin'
    cwd = work/'unrelated cwd'
    cwd.mkdir()
    run([executable, '--smoke-test'], cwd, env)
    logs = set((runtime/'logs').glob('session-*.log'))
    if len(logs) != 1 or (cwd/'logs').exists():
        raise RuntimeError('Logs are not beside the actual executable')
    # Each process owns an independent log; concurrent replacements cannot collide.
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        list(pool.map(lambda _: run([executable, '--smoke-test'], cwd, env), range(2)))
    if len(list((runtime/'logs').glob('session-*.log'))) != 3:
        raise RuntimeError('Concurrent logs collided')
    shutil.move(runtime/'logs', runtime/'successful-logs')
    (runtime/'logs').write_text('blocked directory', encoding='utf-8')
    run([executable, '--smoke-test'], cwd, env, expected=1)
    (runtime/'logs').unlink()
    shutil.move(runtime/'successful-logs', runtime/'logs')
    if os.name != 'nt':
        link = cwd/'launch-link'
        link.symlink_to(executable)
        run([link, '--smoke-test'], cwd, env)
        if len(list((runtime/'logs').glob('session-*.log'))) != 4 or (cwd/'logs').exists():
            raise RuntimeError('Symlink launch changed log placement')
        if manifest['mode'] != 'Ubuntu-24.04-system-Qt-6.4.2':
            raise RuntimeError('Wrong Linux distribution mode')
    else:
        if pe_info(executable)['subsystem'] != 2:
            raise RuntimeError('Game uses a console subsystem')
        qt = Path(os.environ['QTDIR'])
        redist = Path(os.environ['VCToolsRedistDir'])/'x64/Microsoft.VC143.CRT'
        windows(runtime, qt, redist)
        dll = runtime/'Qt6Core.dll'
        backup = runtime/'Qt6Core.disabled'
        dll.rename(backup)
        try:
            try:
                windows(runtime, qt, redist)
            except RuntimeError as error:
                if 'Unresolved import' not in str(error):
                    raise
            else:
                raise RuntimeError('Missing DLL was accepted')
        finally:
            backup.rename(dll)
    if args.rebuild:
        platform = 'windows' if os.name == 'nt' else 'linux'
        run(['python' if os.name == 'nt' else 'python3', '-X', 'utf8', 'tools/dev/check.py'], source)
        run(['cmake', '-S', source, '-B', source/'build/rebuilt', '-G', 'Ninja',
             '-DCMAKE_BUILD_TYPE=Release', '-DAC_BUILD_DEV_TOOLS=ON',
             '-DAC_BUILD_DISTRIBUTION=OFF', '-DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE'], source)
        run(['cmake', '--build', source/'build/rebuilt', '--parallel', '2'], source)
        test_env = os.environ.copy()
        test_env.update({'QT_QPA_PLATFORM': 'offscreen', 'QT_QUICK_BACKEND': 'software', 'QT_QUICK_CONTROLS_STYLE': 'Basic'})
        run(['ctest', '--test-dir', source/'build/rebuilt', '--output-on-failure'], source, test_env)
        suffix = '.exe' if os.name == 'nt' else ''
        run([source/'build/rebuilt'/('ac_benchmark'+suffix)], source)
        run([source/'build/rebuilt/bin'/('anteater-chess'+suffix), '--smoke-test'], cwd, os.environ.copy())
    save()
    print(f'Candidate and source verified at {revision}; clean Windows/manual acceptance remains pending')


if __name__ == '__main__':
    main()
