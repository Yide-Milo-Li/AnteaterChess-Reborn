"""Execute the single preset/install/CPack graph with durable CI command receipts."""
from pathlib import Path
import argparse, hashlib, json, os, subprocess, sys, time
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('platform', choices=('windows','linux'))
parser.add_argument('--evidence', type=Path, default=root/'build/ci-evidence')
args=parser.parse_args()
platform=args.platform
evidence=args.evidence.resolve()
evidence.mkdir(parents=True,exist_ok=True)
checks=[]
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
if subprocess.check_output(['git','status','--porcelain'],cwd=root,text=True).strip():
    raise RuntimeError('Formal matrix and candidates require a clean source commit')
def run(args,env=None):
    log=evidence/f'command-{len(checks):02}.txt'
    started=time.time()
    with log.open('w',encoding='utf-8') as output:
        result=subprocess.run(args,cwd=root,env=env,stdout=output,stderr=subprocess.STDOUT)
    current_env=env or os.environ
    checks.append({'command':args,'cwd':str(root),'platform':platform,
                   'environment':{key:value for key,value in current_env.items()
                                  if key.startswith(('QT_','QML_','AC_')) or key in ('PATH','VCToolsVersion','VCToolsRedistDir','WindowsSDKVersion')},
                   'startedUnix':started,'exitCode':result.returncode,'log':log.name,'sha256':hashlib.sha256(log.read_bytes()).hexdigest()})
    (evidence/'RESULT.json').write_text(json.dumps({'testedCommit':commit,'checks':checks,'cleanWindowsAndManualAcceptance':'pending'},indent=2)+'\n',encoding='utf-8')
    print(f'{args}: exit {result.returncode}',flush=True)
    if result.returncode: raise SystemExit(result.returncode)
presets=[f'{platform}-{mode}{suffix}' for mode in ('debug','release') for suffix in ('','-core')]
if platform=='linux': presets.append('linux-sanitizer')
for preset in presets:
    run(['cmake','--preset',preset])
    run(['cmake','--build','--preset',preset])
    env=os.environ.copy()
    if preset.endswith('sanitizer'):
        env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    run(['ctest','--preset',preset],env)
suffix='.exe' if platform=='windows' else ''
run([sys.executable, '-X', 'utf8', 'tools/packaging/dpi_checks.py',
     '--executable', str(root/f'build/{platform}-release/tests/qt/test_qt_desktop{suffix}'),
     '--evidence', str(evidence/'dpi')])
run([sys.executable,'-X','utf8','tools/dev/check.py'])
build=f'build/{platform}-release'
run(['cmake','--install',build,'--prefix',str(root/f'build/{platform}-install')])
run(['cpack','--config',f'{build}/CPackConfig.cmake'])
run(['cpack','--config',f'{build}/CPackSourceConfig.cmake'])
version=(root/'VERSION').read_text().strip()
extension='.zip' if platform=='windows' else '.tar.gz'
env=os.environ.copy()
env.update(QT_QPA_PLATFORM='windows' if platform=='windows' else 'xcb',QT_QUICK_BACKEND='software')
run([sys.executable,'-X','utf8','tools/packaging/verify_candidate.py','--runtime',str(root/f'dist/AnteaterChess-Reborn-{version}-{platform}-x64{extension}'),'--source',str(root/f'dist/AnteaterChess-Reborn-{version}-source.tar.gz'),'--work',str(evidence/'candidate'),'--rebuild'],env)
