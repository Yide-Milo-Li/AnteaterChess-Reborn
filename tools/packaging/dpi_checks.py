"""Calibrate native DPI, then verify actual 100/150/200% Qt window rendering."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    args = parser.parse_args()
    evidence = args.evidence.resolve()
    evidence.mkdir(parents=True, exist_ok=False)
    executable = args.executable.resolve()
    records = []
    env = os.environ.copy()
    for key in ('QT_SCALE_FACTOR', 'QT_SCREEN_SCALE_FACTORS', 'QT_FONT_DPI',
                'QT_ENABLE_HIGHDPI_SCALING', 'AC_EXPECT_DEVICE_PIXEL_RATIO', 'AC_CAPTURE_DIR'):
        env.pop(key, None)
    env.update(QT_QUICK_BACKEND='software', QT_QUICK_CONTROLS_STYLE='Basic',
               QT_QPA_PLATFORM='windows' if os.name == 'nt' else 'xcb')

    def run(name, current_env, slots=()):
        log = evidence/f'{name}.txt'
        # An absolute QtTest output argument avoids shell comma expansion and
        # keeps QtTest's reporter separate from redirected process diagnostics.
        command = [str(executable), '-o', log.as_posix()+',txt', *slots]
        started = time.time()
        result = subprocess.run(command, env=current_env, capture_output=True, timeout=180)
        console = evidence/f'{name}-console.txt'
        console.write_bytes(result.stdout+b'\n'+result.stderr)
        output = log.read_text(encoding='utf-8', errors='replace') if log.exists() else ''
        ratios = [float(value) for value in re.findall(r'devicePixelRatio ([0-9.]+)', output)]
        records.append({'command': command, 'startedUnix': started, 'exitCode': result.returncode,
                        'environment': {key: value for key, value in current_env.items()
                                        if key.startswith(('QT_', 'AC_'))},
                        'log': log.name, 'sha256': hashlib.sha256(log.read_bytes()).hexdigest() if log.exists() else None,
                        'console': console.name, 'consoleSha256': hashlib.sha256(console.read_bytes()).hexdigest(),
                        'actualDevicePixelRatios': ratios})
        (evidence/'RESULT.json').write_text(json.dumps({'checks': records}, indent=2)+'\n', encoding='utf-8')
        print(f'{name}: exit={result.returncode}, actual DPR={ratios}', flush=True)
        if result.returncode or len(ratios) != 8:
            raise RuntimeError(f'DPI check failed; see {log}')
        return ratios

    native = run('native-calibration', env, ('renderedPagesAndFocus',))
    if min(native) != max(native) or native[0] <= 0:
        raise RuntimeError('Window moved between different native DPI screens during calibration')
    # Qt multiplies QT_SCALE_FACTOR by the native window DPR. Calibrate each
    # machine instead of assuming its display is configured at 100%.
    for desired in (1.0, 1.5, 2.0):
        name = f'dpi-{desired:g}'
        captures = evidence/name
        current_env = env.copy()
        current_env.update(QT_SCALE_FACTOR=f'{desired/native[0]:.12g}',
                           AC_EXPECT_DEVICE_PIXEL_RATIO=f'{desired:g}', AC_CAPTURE_DIR=str(captures))
        ratios = run(name, current_env)
        if any(abs(value-desired) >= 0.01 for value in ratios):
            raise RuntimeError('Rendered DPI differs from the requested effective scale')
        records[-1]['screenshots'] = {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                                     for path in sorted(captures.glob('*.png'))}
        (evidence/'RESULT.json').write_text(json.dumps({'nativeDevicePixelRatio': native[0], 'checks': records}, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
