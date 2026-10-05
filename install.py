"""Install/remove only MCD2 Graphics files; dependencies and saves remain owned by users."""
from pathlib import Path
import argparse, csv, hashlib, io, json, os, shutil, subprocess, sys
HERE=Path(__file__).resolve().parent

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def target(root, name):
    p=root.joinpath(*Path(name).parts)
    if Path(name).is_absolute() or '..' in Path(name).parts or root not in p.resolve().parents:
        raise ValueError('Unsafe package path')
    return p

def require_closed():
    if os.name=='nt':
        # The default table truncates the executable name, defeating this guard.
        output=subprocess.check_output(['tasklist','/FI','IMAGENAME eq Dungeons-Win64-Shipping.exe','/FO','CSV','/NH'],text=True)
        if any(row and row[0].casefold()=='dungeons-win64-shipping.exe' for row in csv.reader(io.StringIO(output))):
            raise ValueError('Close Minecraft Dungeons II before changing files.')
    else:
        for p in Path('/proc').glob('[0-9]*/cmdline'):
            try: name=p.read_bytes().split(b'\0')[0]
            except OSError: continue
            if b'Dungeons-Win64-Shipping.exe' in name:
                raise ValueError('Close Minecraft Dungeons II before changing files.')

def install(root, runtime):
    lock=json.loads((HERE/'dependencies.lock.json').read_text())
    manifest=json.loads((HERE/'manifest.json').read_text())
    expected=manifest['files']
    required={'Dungeons/Binaries/Win64/Dungeons-Win64-Shipping.exe':lock['game']['exeSHA256']}
    for name in ('ReShade','RenoDXUEExtended'):
        d=lock['dependencies'][name];required[d['file']]=d['sha256']
    required.update(lock['dependencies']['BlueprintLoader']['files'])
    for name,sha in required.items():
        p=target(root,name)
        if not p.is_file() or digest(p)!=sha:
            raise ValueError('Missing or unsupported dependency/game file: '+name+'. See dependencies.lock.json and INSTALL.md.')
    if not runtime.is_file() or digest(runtime)!=lock['dependencies']['DLSSRuntime']['sha256']:
        raise ValueError('Supply the pinned official DLSS runtime. It is not bundled; see INSTALL.md.')
    sources={n:HERE/'package'/n for n in expected}
    for n,p in sources.items():
        if not p.is_file() or digest(p)!=expected[n]:raise ValueError('Package integrity failure: '+n)
    n='Dungeons/Binaries/Win64/MCD2Graphics/ngx-runtime/nvngx_dlss.dll'
    expected[n]=digest(runtime);sources[n]=runtime
    marker=root/'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json'
    if marker.exists():raise ValueError('Already installed. Uninstall first; saved preferences are retained.')
    for n in expected:
        if target(root,n).exists():raise ValueError('Refusing to overwrite existing file: '+n)
    old=root/'Dungeons/Binaries/Win64/mcd2-graphics-ui02.addon64'
    if old.exists():raise ValueError('Remove the earlier private trial addon before installing this release.')
    created=[]
    try:
        for n,p in sources.items():
            dest=target(root,n);dest.parent.mkdir(parents=True,exist_ok=True)
            with dest.open('xb') as out:out.write(p.read_bytes())
            created.append(dest)
            if digest(dest)!=expected[n]:raise ValueError('Copy verification failed: '+n)
        with marker.open('x') as out:json.dump({'version':manifest['version'],'files':expected},out,indent=2)
    except Exception:
        for p in reversed(created):p.unlink(missing_ok=True)
        raise
    print('Installed and verified. Existing dependencies, configuration and game saves were preserved.')

def uninstall(root):
    marker=root/'Dungeons/Binaries/Win64/MCD2Graphics/install-manifest.json'
    data=json.loads(marker.read_text());files=data['files']
    allowed=set(json.loads((HERE/'manifest.json').read_text())['files'])|{'Dungeons/Binaries/Win64/MCD2Graphics/ngx-runtime/nvngx_dlss.dll'}
    if set(files)!=allowed:raise ValueError('Unexpected installed file list; refusing removal.')
    for n,sha in files.items():
        p=target(root,n)
        if p.exists() and digest(p)!=sha:raise ValueError('Modified file retained; resolve manually before uninstall: '+n)
    dirs=set()
    for n in files:
        p=target(root,n);dirs.add(p.parent);p.unlink(missing_ok=True)
    marker.unlink()
    for p in sorted(dirs,key=lambda p:len(p.parts),reverse=True):
        while p!=root and p.name in ('MCD2Graphics','ngx-runtime'):
            try:p.rmdir()
            except OSError:break
            p=p.parent
    print('Uninstalled own files. Dependencies, configuration and all saved games/preferences were preserved.')

if __name__=='__main__':
    a=argparse.ArgumentParser(description=__doc__);a.add_argument('action',choices=['install','uninstall','check']);a.add_argument('--game',required=True,type=Path);a.add_argument('--dlss-runtime',type=Path)
    args=a.parse_args()
    try:
        if "Dungeons/Binaries/Win64/mcd2-display-latency.addon64" in json.loads((HERE/"manifest.json").read_text())["files"]:
            raise ValueError("This candidate uses the standalone guided installer for HDR ownership and Reflex dependencies. The Python CLI is retained only for historical preview checks.")
        root=args.game.resolve()
        if args.action=='check':
            expected=json.loads((HERE/'manifest.json').read_text())['files']
            lock=json.loads((HERE/'dependencies.lock.json').read_text())
            expected['Dungeons/Binaries/Win64/MCD2Graphics/ngx-runtime/nvngx_dlss.dll']=lock['dependencies']['DLSSRuntime']['sha256']
            for n,sha in expected.items():
                if digest(target(root,n))!=sha:raise ValueError('Installed file mismatch: '+n)
            print('All six installed mod/runtime files match the release.')
        else:
            require_closed()
            if args.action=='install':
                if args.dlss_runtime is None:raise ValueError('--dlss-runtime is required')
                install(root,args.dlss_runtime.resolve())
            else:uninstall(root)
    except (ValueError,OSError,json.JSONDecodeError) as e:
        print(str(e),file=sys.stderr);sys.exit(1)
