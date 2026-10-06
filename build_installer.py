"""Package only this mod's hash-pinned payload and build standalone guided installers.
The input directory must contain qualified own files from manifest.json. Never
bundle downloaded vendor runtimes, SDKs, game content or private diagnostics.
"""
from pathlib import Path
import argparse,hashlib,json,subprocess,shutil
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
output=a.output.resolve()
if output.exists() and any(output.iterdir()):raise ValueError('Use an empty installer output directory')
output.mkdir(parents=True,exist_ok=True)
subprocess.run([str(a.dotnet.resolve()),'publish',str(root/'src/installer/MCD2.Installer.csproj'),'-c','Release','-r',a.rid,'--self-contained','true','-p:PublishSingleFile=false','-p:RestoreLockedMode=true','-p:Version='+manifest['version'],'-o',str(output),'--nologo'],check=True)
for relative,path in paths:
 target=output/'payload'/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
 assert hashlib.sha256(target.read_bytes()).hexdigest()==manifest['files'][relative]
for name in ['manifest.json','dependencies.lock.json']:
 shutil.copyfile(root/name,output/name)
shutil.copyfile(root/'third-party/INSTALLER_NOTICES.txt',output/'INSTALLER_NOTICES.txt')
assert not any(p.suffix.lower() in ('.zip','.7z','.rar') for p in output.rglob('*') if p.is_file()),'Nested archive refused'
exe=a.output.resolve()/('mcd2-graphics-installer.exe' if a.rid=='win-x64' else 'mcd2-graphics-installer')
if a.rid=='win-x64':
 renamed=exe.with_name('MCD2-Graphics-Installer.exe');exe.replace(renamed);exe=renamed
receipt={'candidateVersion':manifest['version'],'rid':a.rid,'installer':exe.name,'sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'deployment':'self-contained-folder','payloadLocation':'payload/','embeddedPayloadArchive':False,'selfExtraction':False,'payload':manifest['files'],'files':{p.relative_to(output).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(output.rglob('*')) if p.is_file()},'signedReleaseQualified':False,'windowsRuntimeQualified':False}
(a.output.resolve()/'installer-build.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Standalone candidate built. This receipt is build evidence, not gameplay or Windows qualification.')
