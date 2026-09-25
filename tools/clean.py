from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[1]
for name in ('build','dist'):
    raw=root/name
    target=raw.resolve()
    if target.parent != root or raw.is_symlink() or target != raw:
        raise SystemExit('Unsafe generated-directory path')
    if target.exists(): shutil.rmtree(target)
