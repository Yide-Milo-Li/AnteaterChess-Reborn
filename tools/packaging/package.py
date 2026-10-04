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

def windows_package_owners(paths, prefix):
    # The CI action installs MSYS2 under RUNNER_TEMP; cygpath knows its mounts.
    mount = run('cygpath', '-u', str(prefix)).rstrip('/')
    unix = [mount + '/' + p.relative_to(prefix).as_posix() for p in paths]
    required = set(unix)
    owners = {}
    for line in run('pacman', '-Ql').splitlines():
        owner, path = line.split(maxsplit=1)
        if path in required:
            owners[path] = owner
    missing = required - owners.keys()
    if missing:
        raise RuntimeError('Unowned MSYS2 dependency: ' + ', '.join(sorted(missing)))
    return {p.as_posix(): owners[path] for p, path in zip(paths, unix)}

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
    unique = list(dict.fromkeys(p for p in resolved if p))
    owner_map = windows_package_owners(unique, prefix) if unique else {}
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

def linux_dependencies(executable):
    """Record ELF closure, imported QML payloads and available GUI plugin families.

    Discovery is static so packaging also works without DISPLAY. Plugin families
    include alternative platform/image backends; this is a runtime-capability
    inventory, not a claim that every listed plugin was loaded in one smoke run.
    """
    qml = Path(run('qtpaths6', '--query', 'QT_INSTALL_QML'))
    plugins = Path(run('qtpaths6', '--query', 'QT_INSTALL_PLUGINS'))
    scanner = Path(run('qtpaths6', '--query', 'QT_INSTALL_LIBEXECS')) / 'qmlimportscanner'
    scanned = json.loads(run(str(scanner), '-rootPath', str(ROOT/'apps/qt/qml'), '-importPath', str(qml)))
    modules = {item['name']: Path(item['path']) for item in scanned
               if item.get('type') == 'module' and item.get('path')
               and (Path(item['path'])/'qmldir').is_file()}
    # Ubuntu's Qt 6.4 QtQuick plugin loads WorkerScript through C++ even when
    # the QtQml namespace has no qmldir for the import scanner to follow.
    modules['QtQml.WorkerScript'] = qml/'QtQml/WorkerScript'
    for name in ('QtQuick', 'QtQuick.Window', 'QtQuick.Controls',
                 'QtQuick.Controls.Basic', 'QtQuick.Layouts', 'QtQuick.Templates', 'QtQml.WorkerScript'):
        if name not in modules or not (modules[name]/'qmldir').is_file():
            raise RuntimeError('Missing required QML runtime module: '+name)
    paths = set()
    for directory in modules.values():
        # Nested modules have their own descriptors and are handled by the scanner.
        for parent, directories, names in os.walk(directory):
            directories[:] = [name for name in directories if not (Path(parent)/name/'qmldir').is_file()]
            paths.update(Path(parent)/name for name in names if (Path(parent)/name).is_file())
    families = ('platforms', 'platforminputcontexts', 'platformthemes', 'imageformats',
                'iconengines', 'xcbglintegrations', 'egldeviceintegrations')
    plugin_paths = sorted(p for family in families for p in (plugins/family).glob('*.so'))
    paths.update(plugin_paths)
    binaries = [executable, *sorted(p for p in paths if p.read_bytes()[:4] == b'\x7fELF')]
    imports = {}
    for binary in binaries:
        output = run('ldd', str(binary))
        if '=> not found' in output:
            raise RuntimeError('Unresolved ELF dependency of '+str(binary)+'\n'+output)
        # ldd includes ASLR addresses, which must not invalidate an unchanged package.
        imports[str(binary)] = re.sub(r'\(0x[0-9a-fA-F]+\)', '(address omitted)', output)
        paths.update(Path(target or loader) for target,loader in
                     re.findall(r'=>\s+(\S+)|^\s*(/\S+)', output, re.M))
    aliases = sorted({p for path in paths for p in (str(path), str(path.resolve()))})
    query = subprocess.run(['dpkg-query', '-S', *aliases], capture_output=True, text=True)
    owners = {}
    for line in query.stdout.splitlines():
        if ': ' in line:
            owner, path = line.split(': ', 1)
            # dpkg-query also emits diversion metadata for the merged-/usr loader.
            if re.fullmatch(r'[a-z0-9][a-z0-9+.-]*(?::[a-z0-9-]+)?', owner):
                owners[path] = owner
    entries = {}
    for path in sorted(paths):
        actual = path.resolve()
        owner = owners.get(str(path)) or owners.get(str(actual))
        if not owner:
            raise RuntimeError('Unowned Ubuntu dependency: '+str(path))
        entries[str(actual)] = {'path':str(actual), 'sha256':digest(actual), 'package':owner}
    packages = {owner:run('dpkg-query', '-W', '-f=${Version}', owner)
                for owner in sorted({entry['package'] for entry in entries.values()})}
    return {'provider':'Ubuntu system packages', 'catalog':'https://packages.ubuntu.com/',
            'packages':packages, 'imports':imports,
            'qmlModules':{name:str(path) for name,path in sorted(modules.items())},
            'qtPlugins':[str(path) for path in plugin_paths],
            'files':[entries[path] for path in sorted(entries)]}

def linux_runtime(stage, executable, dependencies=None):
    manifest = dependencies if dependencies is not None else linux_dependencies(executable)
    (stage/'DEPENDENCIES.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')


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
    dependencies = None
    if args.kind == 'binary':
        inputs['executable'] = digest(executable)
        if windows:
            inputs['packages'] = run('pacman', '-Q')
        else:
            dependencies = linux_dependencies(executable)
            inputs['dependencies'] = hashlib.sha256(json.dumps(dependencies,sort_keys=True).encode()).hexdigest()
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
                linux_runtime(stage, executable, dependencies)
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
