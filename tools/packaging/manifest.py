"""Validate installed dependency closure and write provenance, never build/archive."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import ctypes


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args):
    return subprocess.check_output(args, text=True, encoding='utf-8', errors='replace', stderr=subprocess.PIPE).strip()


def pe_info(path):
    """Read regular and delay-load PE imports, independent of installed tool PATH."""
    data = path.read_bytes()
    offset = struct.unpack_from('<I', data, 0x3c)[0]
    if data[offset:offset+4] != b'PE\0\0':
        raise RuntimeError(f'Invalid PE: {path}')
    machine, count = struct.unpack_from('<HH', data, offset+4)
    if machine != 0x8664:
        raise RuntimeError(f'Non-x64 PE: {path}')
    optional = offset+24
    size = struct.unpack_from('<H', data, offset+20)[0]
    if struct.unpack_from('<H', data, optional)[0] != 0x20b:
        raise RuntimeError(f'Non-PE32+: {path}')
    sections = optional+size

    def rva(value):
        for index in range(count):
            start = sections+index*40
            virtual_size, virtual, raw_size, raw = struct.unpack_from('<IIII', data, start+8)
            if virtual <= value < virtual+max(virtual_size, raw_size):
                return raw+value-virtual
        raise RuntimeError(f'Invalid RVA in {path}')

    def name(value):
        start = rva(value)
        return data[start:data.index(0, start)].decode('ascii')

    imports = set()
    for table, stride, field in ((1, 20, 12), (13, 32, 4)):
        address = struct.unpack_from('<I', data, optional+112+table*8)[0]
        if not address:
            continue
        start = rva(address)
        while any(data[start:start+stride]):
            imports.add(name(struct.unpack_from('<I', data, start+field)[0]))
            start += stride
    return {'machine': 'x64', 'subsystem': struct.unpack_from('<H', data, optional+68)[0],
            'imports': sorted(imports, key=str.lower)}


def file_version(path):
    api = ctypes.windll.version
    api.GetFileVersionInfoSizeW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_ulong)]
    api.GetFileVersionInfoW.argtypes = [ctypes.c_wchar_p, ctypes.c_ulong, ctypes.c_ulong, ctypes.c_void_p]
    api.VerQueryValueW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_uint)]
    size = api.GetFileVersionInfoSizeW(str(path), None)
    if not size:
        return None
    buffer = ctypes.create_string_buffer(size)
    if not api.GetFileVersionInfoW(str(path), 0, size, buffer):
        raise RuntimeError(f'Cannot read file version: {path}')
    value, length = ctypes.c_void_p(), ctypes.c_uint()
    if not api.VerQueryValueW(buffer, '\\', ctypes.byref(value), ctypes.byref(length)):
        raise RuntimeError(f'Cannot query file version: {path}')
    fields = struct.unpack('<13I', ctypes.string_at(value, 52))
    return '.'.join(str(item) for item in (fields[2] >> 16, fields[2] & 65535, fields[3] >> 16, fields[3] & 65535))


def windows(stage, qt, redist):
    if not re.search(r'/14\.44\.[0-9]+/x64/Microsoft\.VC143\.CRT$', redist.as_posix()):
        raise RuntimeError('Only the permitted v143 14.44 Release REDIST directory is accepted')
    binaries = [*stage.rglob('*.exe'), *stage.rglob('*.dll')]
    names = {}
    for path in binaries:
        names.setdefault(path.name.lower(), []).append(path)
    system = Path(os.environ['SystemRoot'])/'System32'
    sdk = Path(os.environ['WindowsSdkDir'])
    sdk_redist = sdk/'Redist/D3D/x64'
    edges = {}
    origins = []
    for binary in sorted(binaries):
        info = pe_info(binary)
        relative = binary.relative_to(stage).as_posix()
        if binary.suffix.lower() == '.exe' and info['subsystem'] != 2:
            raise RuntimeError('The distributed game must use the Windows GUI subsystem')
        dependencies = {}
        for imported in info['imports']:
            key = imported.lower()
            if key in names:
                candidates = names[key]
                hashes = {digest(item) for item in candidates}
                if len(hashes) != 1:
                    raise RuntimeError(f'Ambiguous deployed import {imported}')
                dependencies[imported] = candidates[0].relative_to(stage).as_posix()
            elif key.startswith(('api-ms-', 'ext-ms-')) or (system/imported).is_file():
                dependencies[imported] = 'Windows system'
            else:
                raise RuntimeError(f'Unresolved import {imported} of {relative}')
        edges[relative] = {**info, 'resolved': dependencies}
        if binary.suffix.lower() == '.exe':
            continue
        if re.search(r'(?:libgtk|libgdk|libcairo|libpango|libgcc|libstdc\+\+|libwinpthread)', binary.name, re.I):
            raise RuntimeError(f'Retired runtime deployed: {relative}')
        candidates = [redist/binary.name, qt/'bin'/binary.name]
        if binary.name.lower() in ('dxcompiler.dll', 'dxil.dll'):
            candidates.append(sdk_redist/binary.name)
        if relative.startswith('plugins/'):
            candidates.append(qt/relative)
        if relative.startswith('qml/'):
            candidates.append(qt/relative)
        origin = next((item for item in candidates if item.is_file() and digest(item) == digest(binary)), None)
        if origin is None:
            raise RuntimeError(f'No byte-identical permitted origin for {relative}')
        provider = ('Microsoft Release REDIST' if origin.is_relative_to(redist) else
                    'Microsoft Windows SDK D3D REDIST' if origin.is_relative_to(sdk_redist) else
                    'Official Qt 6.11.2 MSVC 2022 x64')
        origins.append({'file': relative, 'sha256': digest(binary), 'source': str(origin),
                        'version': file_version(origin), 'provider': provider})
    for item in sorted((stage/'qml').rglob('*')):
        if not item.is_file() or item.suffix.lower() == '.dll':
            continue
        relative = item.relative_to(stage)
        origin = qt/relative
        if origin.is_file() and digest(origin) == digest(item):
            origins.append({'file': relative.as_posix(), 'sha256': digest(item), 'source': str(origin),
                            'version': '6.11.2', 'provider': 'Official Qt QML module'})
        elif relative.parts[1] != 'AnteaterChess':
            raise RuntimeError(f'No permitted QML origin for {relative}')
    if not (stage/'plugins/platforms/qwindows.dll').is_file():
        raise RuntimeError('Native Windows platform plugin missing')
    if not (stage/'plugins/imageformats/qsvg.dll').is_file():
        raise RuntimeError('Qt SVG image plugin missing')
    sdk_license = sdk/'Licenses'/os.environ['WindowsSDKVersion'].strip('\\/')/'sdk_license.rtf'
    (stage/'licenses/microsoft').mkdir(parents=True, exist_ok=True)
    shutil.copy2(sdk_license, stage/'licenses/microsoft/sdk_license.rtf')
    return {'mode': 'official-Qt-deployment-and-Release-REDIST', 'closure': edges, 'origins': origins}


def linux(stage, qt, source):
    qml = Path(run('/usr/lib/qt6/bin/qtpaths6', '--query', 'QT_INSTALL_QML'))
    plugins = Path(run('/usr/lib/qt6/bin/qtpaths6', '--query', 'QT_INSTALL_PLUGINS'))
    scanner = Path(run('/usr/lib/qt6/bin/qtpaths6', '--query', 'QT_INSTALL_LIBEXECS'))/'qmlimportscanner'
    scanned = json.loads(run(str(scanner), '-rootPath', str(source/'apps/qt/qml'), '-importPath', str(qml)))
    modules = {item['name']: Path(item['path']) for item in scanned if item.get('type') == 'module'
               and item.get('path') and (Path(item['path'])/'qmldir').is_file()}
    for name in ('QtQuick', 'QtQuick.Window', 'QtQuick.Controls', 'QtQuick.Controls.Basic',
                 'QtQuick.Layouts', 'QtQuick.Templates', 'QtQml.WorkerScript'):
        path = qml/Path(name.replace('.', '/'))
        if not (path/'qmldir').is_file():
            raise RuntimeError(f'Missing system QML module: {name}')
        modules[name] = path
    files = set()
    for directory in modules.values():
        for parent, directories, names in os.walk(directory):
            directories[:] = [name for name in directories if not (Path(parent)/name/'qmldir').is_file()]
            files.update(Path(parent)/name for name in names if (Path(parent)/name).is_file())
    families = ('platforms', 'imageformats', 'iconengines', 'platforminputcontexts', 'xcbglintegrations')
    files.update(path for family in families for path in (plugins/family).glob('*.so'))
    binaries = [stage/'anteater-chess', *sorted(path for path in files if path.read_bytes()[:4] == b'\x7fELF')]
    edges = {}
    for binary in binaries:
        output = run('ldd', str(binary))
        if 'not found' in output:
            raise RuntimeError(f'Unresolved ELF dependency: {binary}\n{output}')
        paths = re.findall(r'(?:=>\s+)?(/\S+)\s+\(0x', output)
        files.update(Path(path) for path in paths)
        edges[str(binary)] = paths
    records = []
    packages = set()
    for path in sorted(files):
        # Ubuntu usr-merge ownership records sometimes use the /lib spelling.
        spellings = [str(path), str(path.resolve()), str(path).replace('/usr/lib/', '/lib/', 1)]
        owner = None
        for spelling in dict.fromkeys(spellings):
            try:
                matches = re.findall(r'^([a-z0-9][a-z0-9+.-]*(?::[a-z0-9]+)?): ',
                                     run('dpkg-query', '-S', spelling), re.M)
                if matches:
                    owner = matches[0]
                    break
            except subprocess.CalledProcessError:
                pass
        if not owner:
            raise RuntimeError(f'Unowned system dependency: {path}')
        packages.add(owner)
        records.append({'source': str(path), 'sha256': digest(path), 'package': owner})
    versions = {package: run('dpkg-query', '-W', '-f=${Version}', package) for package in sorted(packages)}
    for package in versions:
        copyright_file = Path('/usr/share/doc')/package.split(':')[0]/'copyright'
        if not copyright_file.is_file():
            raise RuntimeError(f'Missing package copyright: {package}')
        destination = stage/'licenses/system'/package.replace(':', '_')
        destination.mkdir(parents=True, exist_ok=True)
        shutil.copy2(copyright_file, destination/'copyright')
    common = Path('/usr/share/common-licenses')
    shutil.copytree(common, stage/'licenses/system/common-licenses', dirs_exist_ok=True)
    return {'mode': 'Ubuntu-24.04-system-Qt-6.4.2', 'qtDeploymentAPI': '6.4.2 user modules only; system libraries remain external',
            'closure': edges, 'qmlModules': {name: str(path) for name, path in sorted(modules.items())},
            'packages': versions, 'origins': records}


def main():
    parser = argparse.ArgumentParser()
    for name in ('stage', 'source', 'qt', 'redist', 'compiler', 'commit'):
        parser.add_argument('--'+name, required=True)
    args = parser.parse_args()
    stage, source, qt = Path(args.stage), Path(args.source), Path(args.qt)
    if not stage.is_dir():
        raise RuntimeError('Install stage missing')
    shutil.copytree(source/'third-party', stage/'licenses/notices', dirs_exist_ok=True)
    if os.name == 'nt':
        receipt = windows(stage, qt, Path(args.redist))
        shutil.copytree(qt/'sbom', stage/'licenses/qt-sbom', dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('qttools*', 'qtdoc*', 'qttranslations*'))
        for bom in (stage/'licenses/qt-sbom').glob('*.spdx.json'):
            for item in json.loads(bom.read_text(encoding='utf-8')).get('hasExtractedLicensingInfos', []):
                (stage/'licenses/notices'/f"{item['licenseId']}.txt").write_text(item['extractedText'], encoding='utf-8')
    else:
        receipt = linux(stage, qt, source)
    commit = args.commit
    (stage/'SOURCE_REVISION').write_text(commit+'\n', encoding='utf-8')
    receipt.update({'sourceCommit': commit, 'compiler': args.compiler, 'qtPrefix': str(qt),
                    'localCandidate': True, 'externalAcceptance': 'pending'})
    receipt['inventory'] = {path.relative_to(stage).as_posix(): digest(path) for path in sorted(stage.rglob('*'))
                            if path.is_file() and path.name not in ('DEPENDENCIES.json', 'FILES.sha256')}
    (stage/'DEPENDENCIES.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    print(f"Verified {len(receipt['closure'])} binary dependency closures; {len(receipt['inventory'])} installed files")


if __name__ == '__main__':
    main()
