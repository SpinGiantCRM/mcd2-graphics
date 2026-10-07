from pathlib import Path
import re, sys
source = Path(sys.argv[1]).resolve()
output = Path(sys.argv[2]).resolve()
text = (source / 'res/resource.rc').read_text()
def normalize(match):
    relative = match.group(2).replace('\\', '/')
    resolved = (source / 'res' / relative).resolve()
    if not resolved.is_file():
        raise FileNotFoundError(resolved)
    return match.group(1) + '"' + resolved.as_posix() + '"'
text = re.sub(r'(?m)^(\s*\w+\s+(?:RCDATA|ICON)\s+)"([^"\n]+)"', normalize, text)
output.write_text(text)
