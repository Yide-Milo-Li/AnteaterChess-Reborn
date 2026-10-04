"""Compare against captured pre-refactor behavior, never regenerate expectations."""
from pathlib import Path
import subprocess
import sys
actual = subprocess.check_output([sys.argv[1]], text=True).splitlines()
expected = Path(sys.argv[2]).read_text().splitlines()
if actual != expected:
    import difflib
    sys.exit('\n'.join(difflib.unified_diff(expected, actual, fromfile='baseline', tofile='current')))
print(f'AI reference: {len(expected)} positions match scores, SEE and search exactly')
