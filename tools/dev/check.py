"""Check local Markdown links, embedded resources and maintained layout."""
from pathlib import Path
import re, subprocess, sys, xml.etree.ElementTree as ET
root = Path(__file__).resolve().parents[2]
errors=[]
for directory in ['include/anteater', 'src/rules', 'src/session', 'src/ai', 'src/policy',
                  'apps/qt/runtime', 'apps/qt/models', 'apps/qt/app',
                  'apps/qt/qml', 'apps/qt/async', 'assets/pieces',
                  'assets/icons', 'docs/user', 'docs/architecture', 'docs/development',
                  'docs/legacy', 'tools/packaging', 'tools/dev', 'tools/legacy']:
    if not (root/directory).is_dir(): errors.append('Missing '+directory)
for path in [*root.glob('*.md'), * (root/'docs').rglob('*.md')]:
    text=path.read_text(encoding='utf-8')
    for link in re.findall(r'\]\(([^)]+)\)',text):
        target=link.split('#')[0]
        if not target or '://' in target or target.startswith('mailto:'): continue
        if not (path.parent/target).exists(): errors.append(f'{path.relative_to(root)}: {link}')
aliases = set()
for manifest in ['assets/resources.qrc','apps/qt/qml/qml.qrc']:
    for resource in ET.parse(root/manifest).iter('qresource'):
        for item in resource.iter('file'):
            if not ((root/manifest).parent/item.text).is_file(): errors.append('Missing resource '+item.text)
            alias = resource.get('prefix','')+'/'+item.get('alias',item.text)
            if alias in aliases: errors.append('Duplicate resource alias '+alias)
            aliases.add(alias)
for module in ['rules','session','ai','policy']:
    for path in (root/'src'/module).iterdir():
        if path.suffix not in {'.h', '.hpp', '.cpp'}: continue
        if re.search(r'#include\s*[<"](?:Q[A-Z]|gtk/|glib|gio/)',path.read_text(encoding='utf-8')):
            errors.append('Desktop dependency in core: '+str(path.relative_to(root)))
if errors:
    sys.exit('\n'.join(errors))
subprocess.run([sys.executable, root/'tests/packaging/test_dependencies.py'], check=True)
print('Documentation links, resources and maintained layout: OK')
