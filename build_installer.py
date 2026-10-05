"""Package only this mod's hash-pinned payload and build standalone guided installers.
The input directory must contain qualified own files from manifest.json. Never
bundle downloaded vendor runtimes, SDKs, game content or private diagnostics.
"""
from pathlib import Path
import argparse,hashlib,json,subprocess,zipfile
root=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--dotnet',required=True,type=Path)
p.add_argument('--payload',required=True,type=Path)
p.add_argument('--rid',choices=['linux-x64','win-x64'],required=True)
p.add_argument('--output',required=True,type=Path)
a=p.parse_args();manifest=json.loads((root/'manifest.json').read_text())
source=a.payload.resolve();paths=[]
for relative,expected in manifest['files'].items():
 assert ':' not in relative and '\\' not in relative and not any(x in ('','.','..')for x in relative.split('/'))
 path=(source/relative).resolve();assert path.is_relative_to(source) and path.is_file(),relative
 assert hashlib.sha256(path.read_bytes()).hexdigest()==expected,'Payload hash mismatch: '+relative
 paths.append((relative,path))
assert all(not Path(x).name.startswith('sl.') and Path(x).name!='nvngx_dlss.dll' for x,_ in paths),'Vendor runtime is not owned payload'
archive=root/'dist/installer-payload.zip';archive.parent.mkdir(exist_ok=True)
with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED)as z:
 for relative,path in paths:
  info=zipfile.ZipInfo(relative,(2026,10,5,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;z.writestr(info,path.read_bytes())
subprocess.run([str(a.dotnet.resolve()),'publish',str(root/'src/installer/MCD2.Installer.csproj'),'-c','Release','-r',a.rid,'--self-contained','true','-p:PublishSingleFile=true','-p:RestoreLockedMode=true','-o',str(a.output.resolve()),'--nologo'],check=True)
exe=a.output.resolve()/('mcd2-graphics-installer.exe' if a.rid=='win-x64' else 'mcd2-graphics-installer')
if a.rid=='win-x64':
 renamed=exe.with_name('MCD2-Graphics-Installer.exe');exe.replace(renamed);exe=renamed
receipt={'candidateVersion':manifest['version'],'rid':a.rid,'installer':exe.name,'sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'payloadArchiveSHA256':hashlib.sha256(archive.read_bytes()).hexdigest(),'payload':manifest['files'],'windowsRuntimeQualified':False}
(a.output.resolve()/'installer-build.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Standalone candidate built. This receipt is build evidence, not gameplay or Windows qualification.')
