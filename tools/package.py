"""Build source/portable runtime archives; no Git checkout is required."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess, tarfile, tempfile, zipfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ['.clang-format', '.gitignore', 'COPYRIGHT', 'README.md', 'AGENTS.md', 'VERSION',
          'Makefile', 'include', 'src', 'apps', 'assets', 'tests', 'docs', 'mk', 'tools', '.github']

def files():
    for name in SOURCE:
        path = ROOT / name
        if path.is_file():
            yield path
        elif path.is_dir():
            yield from sorted(p for p in path.rglob('*') if p.is_file() and '__pycache__' not in p.parts)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def run(*args):
    return subprocess.check_output(args, text=True, encoding='utf-8', errors='replace').strip()

def copy(src, dst):
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)

def windows_runtime(stage, executable):
    prefix = Path(run('pkg-config', '--variable=prefix', 'gtk+-3.0'))
    # UCRT Python gets Windows pkg-config paths. cygpath also handles /ucrt64.
    if not prefix.exists():
        prefix = Path(run('cygpath', '-w', str(prefix)))
    query = prefix / 'bin/gdk-pixbuf-query-loaders.exe'
    loader = next((prefix/'lib/gdk-pixbuf-2.0/2.10.0/loaders').glob('*svg.dll'))
    copy(query, stage/query.name)
    copy(loader, stage/'lib/gdk-pixbuf-2.0/2.10.0/loaders/libpixbufloader-svg.dll')
    queue = [executable, query, loader]
    for helper in (prefix/'bin').glob('gspawn-win64-helper*.exe'):
        copy(helper,stage/helper.name)
        queue.append(helper)
    seen = set()
    source_paths = [query, loader]
    system = Path(os.environ.get('SystemRoot', 'C:/Windows')) / 'System32'
    while queue:
        binary = queue.pop()
        imports = re.findall(r'DLL Name:\s*(\S+)', run('objdump', '-p', str(binary)))
        for name in imports:
            if name.lower() in seen:
                continue
            seen.add(name.lower())
            candidate = prefix/'bin'/name
            if candidate.exists():
                copy(candidate, stage/name)
                queue.append(candidate)
                source_paths.append(candidate)
            elif name.lower().startswith(('api-ms-', 'ext-ms-')) or (system/name).exists():
                continue
            else:
                raise RuntimeError(f'Unresolved dependency: {name} of {binary}')
    for relative in ['share/glib-2.0/schemas', 'share/icons/Adwaita', 'share/icons/hicolor']:
        src = prefix/relative
        if src.exists():
            shutil.copytree(src, stage/relative)
    # Retain the full installed license directory: this also covers icon/schema packages.
    shutil.copytree(prefix/'share/licenses', stage/'licenses')
    package_list = run('pacman', '-Q')
    (stage/'DEPENDENCIES.txt').write_text(package_list+'\n', encoding='utf-8')
    (stage/'THIRD-PARTY.md').write_text(
        '# Bundled third-party components\n\n'
        'DLL imports are resolved recursively from MSYS2 UCRT64; operating-system DLLs are excluded. '
        'The SVG loader, schemas and Adwaita icons are included. License texts are in licenses/. '
        'DEPENDENCIES.txt records the build environment package versions.\n\n'
        'Corresponding MSYS2 package recipes and source URLs: '
        '[MINGW-packages](https://github.com/msys2/MINGW-packages), '
        '[package catalog](https://packages.msys2.org/). '
        'GTK/GLib and other LGPL components are dynamically linked and can be replaced by compatible builds.\n\n'
        'Bundled binaries:\n\n'+''.join(f'- `{p.name}`\n' for p in sorted(source_paths)), encoding='utf-8')
    return prefix

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('kind', choices=['source', 'binary'])
    parser.add_argument('--build')
    args = parser.parse_args()
    os.chdir(ROOT)
    version = (ROOT/'VERSION').read_text().strip()
    windows = os.name == 'nt'
    platform = 'windows-x64' if windows else 'linux-x64'
    name = f'AnteaterChess-Reborn-{version}-{args.kind if args.kind == "source" else platform}'
    suffix = '.zip' if windows and args.kind == 'binary' else '.tar.gz'
    dist = ROOT/'dist'
    dist.mkdir(exist_ok=True)
    archive = dist/(name+suffix)
    inputs = {str(p.relative_to(ROOT)):digest(p) for p in files()}
    executable = ROOT/(args.build or f'build/{platform}/release')/('bin/anteater-chess.exe' if windows else 'bin/anteater-chess')
    if args.kind == 'binary':
        inputs['executable'] = digest(executable)
        if windows:
            inputs['packages'] = run('pacman', '-Q')
    manifest = dist/(name+'.inputs.json')
    if archive.exists() and manifest.exists() and json.loads(manifest.read_text()) == inputs:
        print(f'Unchanged: {archive.name}')
        return
    with tempfile.TemporaryDirectory(prefix='package-', dir=dist) as temporary:
        stage = Path(temporary)/name
        stage.mkdir()
        if args.kind == 'source':
            for p in files():
                copy(p, stage/p.relative_to(ROOT))
        else:
            copy(executable, stage/executable.name)
            for relative in ['COPYRIGHT', 'docs/Chess_UserManual.md', 'docs/legacy/Chess_UserManual.pdf']:
                copy(ROOT/relative, stage/relative)
            copy(ROOT/'tools/templates/README.md', stage/'README.md')
            copy(ROOT/f'tools/templates/INSTALL-{"windows" if windows else "linux"}.md', stage/'INSTALL.md')
            # Runtime manual links resolve within this archive.
            manual = stage/'docs/Chess_UserManual.md'
            manual.write_text(manual.read_text(encoding='utf-8').replace(
                '[Development](DEVELOPMENT.md)',
                '[Development](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/blob/main/docs/DEVELOPMENT.md)'), encoding='utf-8')
            if windows:
                windows_runtime(stage, executable)
        pending = Path(temporary)/(name+suffix)
        if suffix == '.zip':
            with zipfile.ZipFile(pending, 'w', zipfile.ZIP_DEFLATED) as z:
                for p in sorted(stage.rglob('*')):
                    if p.is_file(): z.write(p, p.relative_to(stage.parent))
        else:
            with tarfile.open(pending, 'w:gz') as tar:
                tar.add(stage, arcname=name)
        pending.replace(archive)
    manifest.write_text(json.dumps(inputs, sort_keys=True, indent=2)+'\n')
    archives = sorted(p for p in dist.iterdir() if p.name.endswith(('.zip','.tar.gz')))
    (dist/'SHA256SUMS').write_text(''.join(f'{digest(p)}  {p.name}\n' for p in archives))
    print(f'Created: {archive.name}')

if __name__ == '__main__':
    main()
