"""Verify host archives, source rebuilds, GUI subsystems and portable log behavior."""
from pathlib import Path
import hashlib
import os
import re
import struct
import subprocess
import sys
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
DIST = ROOT / 'dist'
PLATFORM = 'windows-x64' if os.name == 'nt' else 'linux-x64'


def run(args, cwd, env=None, expected=0):
    result = subprocess.run(args, cwd=cwd, env=env, capture_output=True,
                            text=True, encoding='utf-8', errors='replace', timeout=240)
    if result.returncode != expected:
        raise RuntimeError(f'{args}: exit {result.returncode}\n{result.stdout}\n{result.stderr}')
    return result.stdout


def subsystem(executable):
    """Read the PE subsystem directly, independent of objdump or the host PATH."""
    data = executable.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    assert data[pe:pe + 4] == b'PE\0\0'
    return struct.unpack_from('<H', data, pe + 24 + 68)[0]


def main():
    (ROOT / 'build').mkdir(exist_ok=True)
    version = (ROOT / 'VERSION').read_text().strip()
    names = [f'AnteaterChess-Reborn-{version}-source.tar.gz',
             f'AnteaterChess-Reborn-{version}-{PLATFORM}' + ('.zip' if os.name == 'nt' else '.tar.gz')]
    with tempfile.TemporaryDirectory(prefix='verify-', dir=ROOT / 'build',
                                     ignore_cleanup_errors=True) as temporary:
        work = Path(temporary) / 'path with spaces 棋'
        work.mkdir()
        for name in names:
            archive = DIST / name
            if name.endswith('.zip'):
                with zipfile.ZipFile(archive) as contents:
                    assert all('logs' not in Path(item).parts for item in contents.namelist())
                    contents.extractall(work)
            else:
                with tarfile.open(archive) as contents:
                    assert all('logs' not in Path(item.name).parts for item in contents)
                    contents.extractall(work, filter='data')

        source = next(work.glob('*-source'))
        runtime = next(work.glob('*-' + PLATFORM))
        run([sys.executable, 'tools/dev/check.py'], source)
        run(['make', '-s', '-j4', 'CONFIG=release', 'test', 'headless'], source)
        run(['make', '-s', '-j4', 'CONFIG=release', 'gui', 'test-platform'], source)
        if os.name == 'nt':
            assert subsystem(source / 'build/windows-x64/release/bin/anteater-chess.exe') == 2
            for test in (source / 'build/windows-x64/release/tests').rglob('*.exe'):
                assert subsystem(test) == 3
            run(['make', '-s', '-j4', 'CONFIG=release', 'WINDOWS_CONSOLE=1', 'gui'], source)
            assert subsystem(source / 'build/windows-x64/release-console/bin/anteater-chess.exe') == 3

        run([sys.executable, 'tools/packaging/package.py', 'source'], source)
        packed = next((source / 'dist').glob('*.tar.gz'))
        previous = hashlib.sha256(packed.read_bytes()).digest()
        with (source / 'README.md').open('a', encoding='utf-8') as document:
            document.write('\nTemporary packaging invalidation check.\n')
        run([sys.executable, 'tools/packaging/package.py', 'source'], source)
        assert hashlib.sha256(packed.read_bytes()).digest() != previous

        assert (runtime / 'docs/user/manual.md').exists()
        for pdf in (ROOT / 'docs/legacy').glob('*.pdf'):
            assert (source / 'docs/legacy' / pdf.name).read_bytes() == pdf.read_bytes()
        assert (runtime / 'docs/legacy/Chess_UserManual.pdf').read_bytes() == (
            ROOT / 'docs/legacy/Chess_UserManual.pdf').read_bytes()

        exe = runtime / ('anteater-chess.exe' if os.name == 'nt' else 'anteater-chess')
        environment = os.environ.copy()
        for key in ('QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH'):
            environment.pop(key,None)
        if os.name == 'nt':
            environment['PATH'] = str(runtime) + os.pathsep + str(
                Path(environment.get('SystemRoot', 'C:/Windows')) / 'System32')
            assert subsystem(exe) == 2
            forbidden = ('libgtk', 'libgdk', 'libcairo', 'libpango', 'libatk', 'libpixbuf')
            assert not any(p.name.lower().startswith(forbidden) for p in runtime.rglob('*.dll'))
            import json
            manifest = json.loads((runtime/'DEPENDENCIES.json').read_text(encoding='utf-8'))
            for relative,checksum in manifest['inventory'].items():
                assert hashlib.sha256((runtime/relative).read_bytes()).hexdigest() == checksum
            assert set(manifest['inventory']) == {p.relative_to(runtime).as_posix() for p in runtime.rglob('*') if p.is_file()} - {'DEPENDENCIES.json'}
        assert run([str(exe), '--version'], work, environment).strip() == f'AnteaterChess Reborn {version}'
        logs = runtime / 'logs'
        assert not logs.exists()

        # Concurrent processes must keep separate files, even with identical game IDs.
        processes = [subprocess.Popen([str(exe), '--smoke-test'], cwd=work, env=environment,
                                      stdout=subprocess.PIPE, stderr=subprocess.PIPE) for _ in range(2)]
        try:
            for process in processes:
                output, errors = process.communicate(timeout=60)
                assert process.returncode == 0, (output, errors)
        finally:
            for process in processes:
                if process.poll() is None:
                    process.kill()
                    process.wait()
        files = set(logs.glob('session-*.log'))
        assert len(files) == 2
        for path in files:
            assert re.fullmatch(r'session-[0-9a-f-]{36}\.log', path.name)
            assert f'AnteaterChess Reborn {version}' in path.read_text()
        assert not (work / 'logs').exists()

        run([str(exe), '--smoke-test'], work, environment)
        assert len(set(logs.glob('session-*.log')) - files) == 1
        if os.name != 'nt':
            link = work / 'launch chess'
            link.symlink_to(exe)
            run([str(link), '--smoke-test'], work, environment)
            assert len(set(logs.glob('session-*.log')) - files) == 2
            assert not (work / 'logs').exists()

        # A file occupying logs/ exercises unwritable paths on both hosts.
        saved = runtime / 'saved-smoke-logs'
        assert logs.resolve().is_relative_to(work.resolve())
        assert saved.resolve().is_relative_to(work.resolve())
        logs.rename(saved)
        try:
            logs.write_text('Blocked log directory', encoding='utf-8')
            run([str(exe), '--smoke-test'], work, environment, expected=1)
            assert not (work / 'logs').exists()
        finally:
            logs.unlink()
            saved.rename(logs)

        for line in (DIST / 'SHA256SUMS').read_text().splitlines():
            checksum, name = line.split('  ', 1)
            assert hashlib.sha256((DIST / name).read_bytes()).hexdigest() == checksum
    print('Host archives, source rebuilds, resource smoke, portable logs, failure paths and checksums: OK')


if __name__ == '__main__':
    main()
