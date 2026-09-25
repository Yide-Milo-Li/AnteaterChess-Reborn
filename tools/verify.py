"""Verify archives in temporary copies, including rebuild and invalidation."""
from pathlib import Path
import hashlib, os, shutil, subprocess, sys, tarfile, tempfile, zipfile
root=Path(__file__).resolve().parents[1]
dist=root/'dist'
def run(args,cwd,env=None):
    subprocess.run(args,cwd=cwd,env=env,check=True,timeout=240)
with tempfile.TemporaryDirectory(prefix='verify-',dir=root/'build',ignore_cleanup_errors=True) as temp:
    work=Path(temp)/'path with spaces 棋'
    work.mkdir()
    for archive in sorted(dist.iterdir()):
        if archive.name.endswith('.zip'):
            with zipfile.ZipFile(archive) as z:z.extractall(work)
        elif archive.name.endswith('.tar.gz'):
            with tarfile.open(archive) as t:t.extractall(work,filter='data')
    source=next(work.glob('*-source'))
    run([sys.executable,'tools/check.py'],source)
    run(['make','-s','-j4','test','headless','GTK_CFLAGS=','GTK_LIBS=','GLIB_CFLAGS='],source)
    run([sys.executable,'tools/package.py','source'],source)
    packed=next((source/'dist').glob('*.tar.gz'))
    previous=hashlib.sha256(packed.read_bytes()).digest()
    with (source/'README.md').open('a',encoding='utf-8') as f:f.write('\nTemporary packaging invalidation check.\n')
    run([sys.executable,'tools/package.py','source'],source)
    assert hashlib.sha256(packed.read_bytes()).digest()!=previous
    for platform in ['windows-x64','linux-x64']:
        for runtime in work.glob('*-'+platform):
            assert (runtime/'docs/Chess_UserManual.md').exists()
            assert (runtime/'docs/legacy/Chess_UserManual.pdf').exists()
            exe=runtime/('anteater-chess.exe' if platform.startswith('windows') else 'anteater-chess')
            env=os.environ.copy()
            if os.name=='nt':env['PATH']=str(runtime)+os.pathsep+str(Path(env.get('SystemRoot',env.get('SYSTEMROOT','C:/Windows')))/'System32')
            run([str(exe),'--version'],work,env)
            run([str(exe),'--smoke-test'],work,env)
    for line in (dist/'SHA256SUMS').read_text().splitlines():
        checksum,name=line.split('  ',1)
        assert hashlib.sha256((dist/name).read_bytes()).hexdigest()==checksum
print('Archives, clean-path runtime smoke, source rebuild/repackage and document invalidation: OK')
