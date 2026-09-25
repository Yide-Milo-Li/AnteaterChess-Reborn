"""Publish only a complete, green CI build. Upload failures leave a draft."""
from pathlib import Path
import argparse, hashlib, json, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
def gh(*args):
    return subprocess.check_output(['gh',*args],cwd=ROOT,text=True,encoding='utf-8').strip()
parser=argparse.ArgumentParser()
parser.add_argument('run_id')
args=parser.parse_args()
run=json.loads(gh('run','view',args.run_id,'--json','headSha,conclusion,jobs'))
assert run['conclusion']=='success', 'CI must be successful'
assert {job['name'] for job in run['jobs'] if job['conclusion']=='success'}>={'linux','windows'}
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
assert head==run['headSha'], 'Release must use the validated HEAD'
version=(ROOT/'VERSION').read_text().strip()
tag='v'+version
with tempfile.TemporaryDirectory(prefix='release-',dir=ROOT/'build') as temp:
    staging=Path(temp)
    gh('run','download',args.run_id,'--dir',str(staging))
    archives=[*staging.rglob('*.zip'),*staging.rglob('*.tar.gz')]
    expected={f'AnteaterChess-Reborn-{version}-{suffix}' for suffix in ['source.tar.gz','windows-x64.zip','linux-x64.tar.gz']}
    assert {p.name for p in archives}==expected
    for manifest in staging.rglob('SHA256SUMS'):
        for line in manifest.read_text().splitlines():
            digest,name=line.split('  ',1)
            artifact=manifest.parent/name
            # Windows source is intentionally supplied only by Linux artifact.
            if artifact.exists(): assert hashlib.sha256(artifact.read_bytes()).hexdigest()==digest
    checksum=staging/'SHA256SUMS'
    checksum.write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in sorted(archives)))
    try:
        existing=json.loads(gh('release','view',tag,'--json','isDraft'))
        assert existing['isDraft'], 'Refusing to replace a published release'
    except subprocess.CalledProcessError:
        gh('release','create',tag,'--target',head,'--draft','--title',f'AnteaterChess Reborn {version}',
           '--notes-file',str(ROOT/'docs/RELEASE-NOTES.md'))
    gh('release','upload',tag,*[str(p) for p in archives],str(checksum),'--clobber')
    remote=json.loads(gh('release','view',tag,'--json','assets'))
    assert {a['name'] for a in remote['assets']}==expected|{'SHA256SUMS'}
    gh('release','edit',tag,'--draft=false','--latest')
    print(gh('release','view',tag,'--json','url','--jq','.url'))
