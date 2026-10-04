"""Translate the established Make commands into one CMake/Ninja graph."""
from pathlib import Path
import argparse
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]

def run(args, **kwargs):
    subprocess.run([str(a) for a in args], check=True, cwd=ROOT, **kwargs)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', choices=['debug','release','sanitize'], default='debug')
    parser.add_argument('--build', default='')
    parser.add_argument('--console', choices=['0','1'], default='0')
    parser.add_argument('goals', nargs='+')
    args = parser.parse_args()
    platform = 'windows-x64' if os.name == 'nt' else 'linux-x64'
    suffix = '-console' if os.name == 'nt' and args.console == '1' else ''
    build = args.build or f'build/{platform}/{args.config}{suffix}'
    goals = set(args.goals)
    if 'clean' in goals:
        run([sys.executable,'tools/dev/clean.py'])
        return
    if 'help' in goals:
        print('make [gui|headless|test|test-rules|test-session|test-ai|test-platform|test-gui|check|benchmark|package|package-source]')
        print('CONFIG=debug|release|sanitize BUILD=path WINDOWS_CONSOLE=0|1; CMake/Ninja own all build dependencies.')
        return
    if 'check' in goals: run([sys.executable,'tools/dev/check.py'])
    if goals.intersection({'package-source','tar'}): run([sys.executable,'tools/packaging/package.py','source'])
    build_goals = goals - {'check','package-source','tar'}
    if not build_goals: return
    desktop = bool(build_goals.intersection({'all','gui','run','test-platform','test-gui','package','tar-user'}))
    # Core-only commands configure without finding Qt, even on a desktop checkout.
    run(['cmake','-S','.', '-B',build,'-G','Ninja',
         '-DCMAKE_BUILD_TYPE='+('Release' if args.config=='release' else 'Debug'),
         '-DAC_BUILD_DESKTOP='+('ON' if desktop else 'OFF'),
         '-DAC_SANITIZE='+('ON' if args.config=='sanitize' else 'OFF'),
         '-DAC_WINDOWS_CONSOLE='+('ON' if args.console=='1' else 'OFF')])
    targets=[]
    if desktop: targets.append('all')
    else: targets.append('all' if build_goals-{'headless','benchmark'} else 'ac_session')
    if 'benchmark' in goals: targets.append('ac_benchmark')
    run(['cmake','--build',build,'--parallel','4','--target',*targets])
    labels=[]
    if goals.intersection({'test','tests'}):
        if 'test' in goals: labels.append('core')
    for goal,label in [('test-rules','rules'),('test-session','session'),('test-ai','ai'),('test-platform','platform')]:
        if goal in goals: labels.append(label)
    if 'test-gui' in goals: labels.extend(['gui','platform'])
    if labels: run(['ctest','--test-dir',build,'--output-on-failure','-L','^('+ '|'.join(labels)+')$'])
    exe = '.exe' if os.name=='nt' else ''
    if 'benchmark' in goals: run([ROOT/build/('ac_benchmark'+exe)])
    if 'run' in goals: run([ROOT/build/'bin'/('anteater-chess'+exe)])
    if goals.intersection({'package','tar-user'}):
        run([sys.executable,'tools/packaging/package.py','binary','--build',build])

if __name__ == '__main__': main()
