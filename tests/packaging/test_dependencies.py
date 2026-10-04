"""Packaging regressions run without a Qt installation or package manager."""
from pathlib import Path
import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('ac_package', ROOT/'tools/packaging/package.py')
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


class DependencyTests(unittest.TestCase):
    def test_msys_ownership_uses_active_mount(self):
        for location in ('C:/msys64/ucrt64', 'D:/a/_temp/msys64/ucrt64',
                         'C:/tools with spaces/msys64/ucrt64'):
            with self.subTest(prefix=location):
                prefix = Path(location)
                files = [prefix/'bin/Qt6Core.dll', prefix/'share/qt6/qml/QtQuick/qmldir']
                catalog = ('qt-base /ucrt64/bin/Qt6Core.dll\n'
                           'qt-declarative /ucrt64/share/qt6/qml/QtQuick/qmldir')
                with patch.object(package, 'run', side_effect=['/ucrt64', catalog]) as run:
                    owners = package.windows_package_owners(files, prefix)
                self.assertEqual(owners, {files[0].as_posix():'qt-base',
                                          files[1].as_posix():'qt-declarative'})
                run.assert_any_call('cygpath', '-u', str(prefix))

    def test_unowned_msys_file_is_rejected(self):
        prefix = Path('D:/tools/msys64/ucrt64')
        with patch.object(package, 'run', side_effect=['/ucrt64', '']):
            with self.assertRaisesRegex(RuntimeError, 'Unowned MSYS2 dependency'):
                package.windows_package_owners([prefix/'bin/missing.dll'], prefix)

    def test_linux_dynamic_modules_and_data_invalidate_manifest(self):
        with tempfile.TemporaryDirectory(prefix='ac-dependencies-') as temporary:
            root = Path(temporary)
            qml = root/'qml'; plugins = root/'plugins'; library = root/'libworker.so'
            library.write_bytes(b'\x7fELF-library')
            names = ('QtQuick', 'QtQuick.Window', 'QtQuick.Controls', 'QtQuick.Controls.Basic',
                     'QtQuick.Layouts', 'QtQuick.Templates', 'QtQml.WorkerScript')
            scanned = []
            for name in names:
                directory = qml/Path(*name.split('.')); directory.mkdir(parents=True, exist_ok=True)
                (directory/'qmldir').write_text('module '+name)
                (directory/'plugin.so').write_bytes(b'\x7fELF-plugin')
                scanned.append({'name':name, 'path':str(directory), 'type':'module'})
            data = qml/'QtQuick/Controls/Basic/Button.qml'; data.write_text('initial payload')
            platform = plugins/'platforms/libqxcb.so'; platform.parent.mkdir(parents=True)
            platform.write_bytes(b'\x7fELF-platform')
            executable = root/'anteater-chess'
            addresses = [0]

            def run(*args):
                if args[0] == 'qtpaths6':
                    return str({'QT_INSTALL_QML':qml, 'QT_INSTALL_PLUGINS':plugins,
                                'QT_INSTALL_LIBEXECS':root}[args[2]])
                if args[0].endswith('qmlimportscanner'):
                    return json.dumps(scanned)
                if args[0] == 'ldd':
                    addresses[0] += 1
                    return f'libworker.so => {library} (0x{addresses[0]:x})'
                if args[0] == 'dpkg-query':
                    return '6.4.2-1'
                raise AssertionError(args)

            def query(args, **unused):
                # A real package lookup is keyed by the requested paths, including
                # QML text files which never appear in ldd output.
                lines = [f'qml6-module-qtquick-controls:amd64: {path}' for path in args[2:]]
                lines.append(f'diversion by libc6 from: {library}')
                return subprocess.CompletedProcess(args, 0, '\n'.join(lines), '')

            with patch.object(package, 'run', side_effect=run), patch.object(package.subprocess, 'run', side_effect=query):
                first = package.linux_dependencies(executable)
                again = package.linux_dependencies(executable)
                self.assertEqual(first, again, 'ASLR must not churn unchanged packages')
                files = {entry['path'] for entry in first['files']}
                self.assertIn(str(data.resolve()), files)
                self.assertIn(str(platform.resolve()), files)
                self.assertIn(str(library.resolve()), files)
                self.assertIn('QtQml.WorkerScript', first['qmlModules'])
                data.write_text('upgraded payload')
                after_data = package.linux_dependencies(executable)
                self.assertNotEqual(first, after_data)
                library.write_bytes(b'\x7fELF-upgraded-library')
                self.assertNotEqual(after_data, package.linux_dependencies(executable))
                with patch.object(package.subprocess, 'run', return_value=subprocess.CompletedProcess([], 1, '', '')):
                    with self.assertRaisesRegex(RuntimeError, 'Unowned Ubuntu dependency'):
                        package.linux_dependencies(executable)
                (qml/'QtQuick/Templates/qmldir').unlink()
                with self.assertRaisesRegex(RuntimeError, 'Missing required QML runtime module'):
                    package.linux_dependencies(executable)


if __name__ == '__main__':
    unittest.main()
