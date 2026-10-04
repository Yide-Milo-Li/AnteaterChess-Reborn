"""Build source/portable runtime archives; no Git checkout is required."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess, tarfile, tempfile, zipfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ['.clang-format', '.gitignore', 'COPYRIGHT', 'README.md', 'AGENTS.md', 'VERSION',
          'Makefile', 'CMakeLists.txt', 'include', 'src', 'apps', 'assets', 'tests', 'docs', 'tools', '.github']

def files():
    for name in SOURCE:
        path = ROOT / name
        if path.is_file():
            yield path
        elif path.is_dir():
            yield from sorted(p for p in path.rglob('*') if p.is_file()
                              and not {'__pycache__', 'logs'}.intersection(p.relative_to(ROOT).parts))

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def run(*args):
    return subprocess.check_output(args, text=True, encoding='utf-8', errors='replace').strip()

def copy(src, dst):
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)

def windows_runtime(stage, executable):
    """Official Qt deployment plus recursive third-party import verification."""
    prefix = Path(run('qtpaths6', '--query', 'QT_INSTALL_PREFIX'))
    plugins = Path(run('qtpaths6', '--query', 'QT_INSTALL_PLUGINS'))
    qml = Path(run('qtpaths6', '--query', 'QT_INSTALL_QML'))
    subprocess.run(['windeployqt6', '--release', '--no-translations', '--qmldir',
                    str(ROOT/'apps/qt/qml'), '--dir', str(stage), str(stage/executable.name)], check=True)
    # MSYS2 Qt uses installation-relative QML paths; make relocation explicit.
    (stage/'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=.\nQmlImports=qml\n',encoding='utf-8')
    system = Path(os.environ.get('SystemRoot', 'C:/Windows')) / 'System32'
    queue = [stage/executable.name, *stage.rglob('*.dll')]
    seen = set()
    origins = {}
    while queue:
        binary = queue.pop()
        for name in re.findall(r'DLL Name:\s*(\S+)', run('objdump', '-p', str(binary))):
            if name.lower() in seen: continue
            seen.add(name.lower())
            candidate = prefix/'bin'/name
            if candidate.exists():
                destination = stage/name
                if not destination.exists(): copy(candidate, destination)
                origins[destination] = candidate
                queue.append(destination)
            elif name.lower().startswith(('api-ms-', 'ext-ms-')) or (system/name).exists():
                continue
            else:
                raise RuntimeError(f'Unresolved dependency: {name} of {binary}')
    # MSYS2 Qt/font dependencies can themselves use GLib. Reject the retired
    # rendering chain, while recording all legitimate transitive dependencies.
    forbidden = ('libgtk', 'libgdk', 'libcairo', 'libpango', 'libatk', 'libpixbuf')
    rejected = [str(p.relative_to(stage)) for p in stage.rglob('*.dll') if p.name.lower().startswith(forbidden)]
    if rejected: raise RuntimeError('Unexpected desktop dependencies: '+', '.join(rejected))
    owners = set()
    entries = []
    resolved = []
    for p in sorted(stage.rglob('*')):
        if not p.is_file():
            continue
        relative = p.relative_to(stage)
        origin = origins.get(p)
        if origin is None:
            candidates = [prefix/'bin'/p.name, plugins/relative, qml/relative]
            if relative.parts[0] == 'qml': candidates.append(qml/Path(*relative.parts[1:]))
            origin = next((x for x in candidates if x.exists()), None)
        resolved.append(origin)
        entries.append({'path':relative.as_posix(),'sha256':digest(p),
                        'package':None,'source':str(origin) if origin else 'project'})
    # Query owners in one process; deployment includes hundreds of QML files.
    unique = list(dict.fromkeys(p.as_posix() for p in resolved if p))
    owner_map = {}
    if unique:
        unix = [p.replace('C:/msys64/ucrt64/','/ucrt64/') for p in unique]
        required = set(unix)
        package_paths = {}
        # One package database traversal avoids per-file processes and repeated scans.
        for line in run('pacman','-Ql').splitlines():
            owner,path = line.split(maxsplit=1)
            if path in required: package_paths[path] = owner
        output = [package_paths[path] for path in unix]
        if len(output) != len(unique): raise RuntimeError('Incomplete package ownership query')
        owner_map = dict(zip(unique,output))
    for entry,origin in zip(entries,resolved):
        if origin:
            owner = owner_map[origin.as_posix()]
            entry['package'] = owner
            owners.add(owner)
    for owner in sorted(owners):
        short = owner.removeprefix('mingw-w64-ucrt-x86_64-')
        license_dir = prefix/'share/licenses'/short
        if license_dir.exists(): shutil.copytree(license_dir, stage/'licenses'/short, dirs_exist_ok=True)
        elif short == 'icu':
            candidates = list((prefix/'share/icu').glob('*/LICENSE'))
            if not candidates: raise RuntimeError('Missing ICU license')
            copy(candidates[-1],stage/'licenses/icu/LICENSE')
        else: raise RuntimeError('Missing license directory for '+owner)
    versions = {owner:run('pacman','-Q',owner).split(maxsplit=1)[1] for owner in sorted(owners)}
    manifest = {'provider':'MSYS2 UCRT64','catalog':'https://packages.msys2.org/',
                'packages':versions,'files':entries}
    (stage/'DEPENDENCIES.txt').write_text(''.join(f'{p} {v}\n' for p,v in versions.items()),encoding='utf-8')
    (stage/'THIRD-PARTY.md').write_text(
        '# Bundled third-party components\n\n'
        'Qt libraries and QML/plugins are deployed with windeployqt. Other DLL imports '
        'are resolved recursively from MSYS2 UCRT64; Windows system DLLs are excluded. '
        'DEPENDENCIES.json records the verified file hashes, owners and package versions. '
        'License texts are in licenses/. Compatible dynamically linked libraries remain replaceable.\n\n'
        'Recipes and corresponding sources: [MSYS2 MINGW-packages](https://github.com/msys2/MINGW-packages).\n',
        encoding='utf-8')
    manifest['inventory'] = {p.relative_to(stage).as_posix():digest(p)
                             for p in sorted(stage.rglob('*')) if p.is_file()}
    (stage/'DEPENDENCIES.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    return prefix

def linux_runtime(stage, executable):
    imports = run('ldd', str(executable))
    paths = re.findall(r'=> (/\S+)', imports)
    packages = {}
    entries = []
    for path in paths:
        actual = Path(path).resolve()
        query = subprocess.run(['dpkg-query','-S',path],capture_output=True,text=True)
        if query.returncode:
            query = subprocess.run(['dpkg-query','-S',str(actual)],capture_output=True,text=True)
        owner = query.stdout.split(': ')[0] if query.returncode == 0 else None
        if owner:
            packages[owner] = run('dpkg-query','-W','-f=${Version}',owner)
        entries.append({'path':path,'sha256':digest(actual),'package':owner})
    (stage/'DEPENDENCIES.json').write_text(json.dumps({
        'provider':'Ubuntu system packages','catalog':'https://packages.ubuntu.com/',
        'packages':packages,'imports':imports,'files':entries},indent=2)+'\n',encoding='utf-8')


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
    if archive.exists() and manifest.exists() and json.loads(manifest.read_text(encoding='utf-8')) == inputs:
        print(f'Unchanged: {archive.name}')
        return
    with tempfile.TemporaryDirectory(prefix='package-', dir=dist, ignore_cleanup_errors=True) as temporary:
        stage = Path(temporary)/name
        stage.mkdir()
        if args.kind == 'source':
            for p in files():
                copy(p, stage/p.relative_to(ROOT))
        else:
            copy(executable, stage/executable.name)
            for relative in ['COPYRIGHT', 'docs/user/manual.md', 'docs/legacy/Chess_UserManual.pdf']:
                copy(ROOT/relative, stage/relative)
            copy(ROOT/'tools/packaging/templates/README.md', stage/'README.md')
            copy(ROOT/f'tools/packaging/templates/INSTALL-{"windows" if windows else "linux"}.md', stage/'INSTALL.md')
            # Runtime manual links resolve within this archive.
            manual = stage/'docs/user/manual.md'
            manual.write_text(manual.read_text(encoding='utf-8').replace(
                '[Development](../development/guide.md)',
                '[Development](https://github.com/Yide-Milo-Li/AnteaterChess-Reborn/blob/main/docs/development/guide.md)'), encoding='utf-8')
            if windows:
                windows_runtime(stage, executable)
            else:
                linux_runtime(stage, executable)
        pending = Path(temporary)/(name+suffix)
        if suffix == '.zip':
            with zipfile.ZipFile(pending, 'w', zipfile.ZIP_DEFLATED) as z:
                for p in sorted(stage.rglob('*')):
                    if p.is_file(): z.write(p, p.relative_to(stage.parent))
        else:
            with tarfile.open(pending, 'w:gz') as tar:
                tar.add(stage, arcname=name)
        pending.replace(archive)
    manifest.write_text(json.dumps(inputs, sort_keys=True, indent=2)+'\n',encoding='utf-8')
    archives = sorted(p for p in dist.iterdir() if p.name.endswith(('.zip','.tar.gz')))
    (dist/'SHA256SUMS').write_text(''.join(f'{digest(p)}  {p.name}\n' for p in archives))
    print(f'Created: {archive.name}')

if __name__ == '__main__':
    main()
