"""Install pinned official Qt archives on a fresh Windows CI runner with receipts."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import urllib.request
import xml.etree.ElementTree as ET

REPOSITORY = 'https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt6_6112/qt6_6112_msvc2022_64/'
COMPONENTS = ('qt.qt6.6112.win64_msvc2022_64',
              'qt.qt6.6112.addons.qtshadertools.win64_msvc2022_64',
              'qt.qt6.6112.addons.qttasktree.win64_msvc2022_64')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--metadata-only', action='store_true')
    args = parser.parse_args()
    evidence = args.evidence.resolve()
    evidence.mkdir(parents=True, exist_ok=True)
    metadata = urllib.request.urlopen(REPOSITORY+'Updates.xml', timeout=120).read()
    (evidence/'Updates.xml').write_bytes(metadata)
    packages = {item.findtext('Name'): item for item in ET.fromstring(metadata).findall('PackageUpdate')}
    receipt = {'repository': REPOSITORY, 'metadataSha256': hashlib.sha256(metadata).hexdigest(),
               'components': list(COMPONENTS), 'archives': [],
               'status': 'metadata-verified' if args.metadata_only else 'installing'}

    def save():
        (evidence/'RESULT.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')

    for component in COMPONENTS:
        package = packages[component]
        version = package.findtext('Version')
        if not version.startswith('6.11.2-'):
            raise RuntimeError('Official metadata returned an unexpected Qt version')
        operations = {operation.findall('Argument')[1].text: operation.findall('Argument')[0].text
                      for operation in package.findall('Operations/Operation') if operation.get('name') == 'Extract'}
        for archive in package.findtext('DownloadableArchives').split(','):
            archive = archive.strip()
            url = REPOSITORY+component+'/'+version+archive
            expected = urllib.request.urlopen(url+'.sha1', timeout=120).read().decode('ascii').strip()
            if not re.fullmatch('[0-9a-f]{40}', expected):
                raise RuntimeError('Invalid official archive checksum')
            record = {'component': component, 'version': version, 'url': url, 'officialSha1': expected,
                      'destination': operations[archive].replace('@TargetDir@', 'C:/Qt')}
            receipt['archives'].append(record)
            save()
            if args.metadata_only:
                continue
            inputs = Path('C:/Qt/installer-inputs')
            inputs.mkdir(parents=True, exist_ok=True)
            incoming = inputs/(component+'-'+archive)
            sha1, sha256 = hashlib.sha1(), hashlib.sha256()
            with urllib.request.urlopen(url, timeout=300) as response, incoming.open('wb') as output:
                record['resolvedUrl'] = response.url
                while chunk := response.read(1024*1024):
                    output.write(chunk)
                    sha1.update(chunk)
                    sha256.update(chunk)
            record['sha256'] = sha256.hexdigest()
            if sha1.hexdigest() != expected:
                raise RuntimeError(f'Official archive checksum mismatch: {url}')
            log = evidence/f'extract-{len(receipt["archives"]):02}.txt'
            command = ['C:/Program Files/7-Zip/7z.exe', 'x', str(incoming), '-y', '-o'+record['destination']]
            with log.open('w', encoding='utf-8') as output:
                result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT)
            record.update(command=command, exitCode=result.returncode, log=log.name,
                          logSha256=hashlib.sha256(log.read_bytes()).hexdigest())
            save()
            if result.returncode:
                raise RuntimeError(f'Archive extraction failed: {log}')
            print(f'Installed official {component}: {archive}', flush=True)
    if not args.metadata_only:
        qt = Path('C:/Qt/6.11.2/msvc2022_64')
        (qt/'bin/qt.conf').write_text('[Paths]\nPrefix=..\n', encoding='utf-8')
        receipt['status'] = 'installed-probes-required'
    save()


if __name__ == '__main__':
    main()
