"""Verify qualification inputs before extracting this project's files only."""
import hashlib,json,zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
manifest=json.loads((root/'manifest.json').read_text())
expected=manifest['files'];output=root/'dist/qualified-own'
archive = root/('qualification/fg-release/own-payload.zip' if manifest['version']=='0.3.0-preview.1' else 'qualification/update2/own-payload.zip')
with zipfile.ZipFile(archive) as z:
    if len(z.infolist())!=len(expected) or set(z.namelist())!=set(expected):
        raise ValueError('Qualification archive members differ from the manifest')
    data={}
    for p,h in expected.items():
        if ':' in p or '\\' in p or any(v in ('','.','..') for v in p.split('/')):
            raise ValueError('Unsafe qualification path')
        if Path(p).name.startswith('sl.') or Path(p).name=='nvngx_dlss.dll':
            raise ValueError('Vendor runtime is not own payload')
        info=z.getinfo(p)
        if info.file_size>64*1024*1024 or (info.external_attr>>16&0xf000)==0xa000:
            raise ValueError('Unsafe qualification entry')
        b=z.read(p)
        if hashlib.sha256(b).hexdigest()!=h:raise ValueError('Qualification hash mismatch: '+p)
        data[p]=b
    for p,b in data.items():
        f=output/p;f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(b)
print(f'{len(expected)} own payload files verified. No vendor runtimes or private data included.')
