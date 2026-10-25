"""Package only our companion, never the community game extension or runtimes."""
import argparse
from pathlib import Path
import zipfile
ROOT=Path(__file__).resolve().parents[1]
def build(output):
    output=Path(output)
    if output.exists():raise ValueError('Output already exists')
    source=ROOT/'integrations/vortex'
    members=[source/'index.js',source/'info.json',*sorted((source/'src').glob('*.js'))]
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as z:
        for member in members:z.write(member,member.relative_to(source).as_posix())
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);build(p.parse_args().output)
