"""Check local Markdown links, embedded resources and maintained layout."""
from pathlib import Path
import re, sys, xml.etree.ElementTree as ET
root = Path(__file__).resolve().parents[2]
errors=[]
for directory in ['include/anteater', 'src/rules', 'src/session', 'src/ai',
                  'src/platform/runtime', 'src/platform/logging', 'apps/gtk/app',
                  'apps/gtk/screens', 'apps/gtk/ui', 'apps/gtk/async', 'assets/pieces',
                  'assets/icons', 'docs/user', 'docs/architecture', 'docs/development',
                  'docs/legacy', 'tools/packaging', 'tools/dev', 'tools/legacy']:
    if not (root/directory).is_dir(): errors.append('Missing '+directory)
for path in [*root.glob('*.md'), * (root/'docs').rglob('*.md')]:
    text=path.read_text(encoding='utf-8')
    for link in re.findall(r'\]\(([^)]+)\)',text):
        target=link.split('#')[0]
        if not target or '://' in target or target.startswith('mailto:'): continue
        if not (path.parent/target).exists(): errors.append(f'{path.relative_to(root)}: {link}')
for item in ET.parse(root/'assets/resources.xml').iter('file'):
    if not (root/'assets'/item.text).is_file(): errors.append('Missing resource '+item.text)
if errors:
    sys.exit('\n'.join(errors))
print('Documentation links, resources and maintained layout: OK')
