"""Build using separately acquired, pinned SDKs. No vendor downloads or license acceptance."""
from pathlib import Path
import argparse, subprocess, re, json, hashlib
r=Path(__file__).resolve().parent
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--dotnet',required=True,type=Path);p.add_argument('--neorune-sdk',required=True,type=Path);p.add_argument('--pack-tools',required=True,type=Path)
p.add_argument('--reshade-include',required=True,type=Path);p.add_argument('--ngx-include',required=True,type=Path)
p.add_argument('--dxc',required=True,type=Path);p.add_argument('--output',type=Path,default=r/'dist/build')
a=p.parse_args();o=a.output.resolve();o.mkdir(parents=True,exist_ok=True);native=r/'src/native'
assert '#define RESHADE_API_VERSION 20' in (a.reshade_include/'reshade.hpp').read_text(),'Expected ReShade API 20'
assert '<version>0.1.2</version>' in (a.neorune_sdk/'NeoRune.Sdk.nuspec').read_text(encoding='utf-8-sig'),'Expected NeoRune 0.1.2'
# SDK include files stay outside the public source tree. Our include shim preserves the tested case spelling.
inc=o/'include';inc.mkdir(exist_ok=True);(inc/'Windows.h').write_text('#include <windows.h>\n')
for f in a.reshade_include.glob('reshade*.hpp'):(inc/f.name).write_bytes(f.read_bytes())
ngx=o/'ngx-research';ngx.mkdir(exist_ok=True)
for f in a.ngx_include.glob('nvsdk_ngx*.h'):(ngx/f.name).write_bytes(f.read_bytes())
commands=[
 ['clang++','--target=x86_64-pc-windows-msvc','-std=c++20','-O2','-fno-exceptions','-fno-rtti','-I'+str(ngx),'-c',str(native/'ngx_parameter_bridge.cpp'),'-o',str(o/'ngx_parameter_bridge.obj')],
 ['clang++','--target=x86_64-pc-windows-msvc','-ffreestanding','-std=c++20','-O2','-fno-exceptions','-fno-rtti','-nostdinc++','-I'+str(native/'bridge-freestanding'),'-I'+str(inc),'-c',str(native/'reshade_public_bridge.cpp'),'-o',str(o/'reshade_public_bridge.obj')],
 ['x86_64-w64-mingw32-g++','-DMCD2_ENABLE_DIAGNOSTICS=0','-std=c++20','-O2','-shared','-static','-I'+str(inc),'-I'+str(o),str(native/'observer.cpp'),str(o/'ngx_parameter_bridge.obj'),str(o/'reshade_public_bridge.obj'),'-o',str(o/'mcd2-graphics.addon64'),'-ld3d12','-ldxgi','-ld3dcompiler','-lole32','-luuid'],
 [str(a.dxc),'-T','cs_6_0','-E','main',str(r/'src/shaders/live_dense.hlsl'),'-Fo',str(o/'live_dense.cso')]
]
for cmd in commands:subprocess.run(cmd,check=True)
dotnet_root=a.dotnet.resolve().parent
refs=dotnet_root/'packs/Microsoft.NETCore.App.Ref/10.0.12/ref/net10.0'
assert refs.is_dir(),'Expected .NET 10.0.12 reference pack'
args=['build','--mod=MCD2Graphics','--out='+str(o/'ui'),'--tools='+str(a.pack_tools.resolve())]
args += ['--source='+str(a.neorune_sdk/'src'/f)for f in ('Log.cs','Timer.cs','World.cs')]
args += ['--ref='+str(f)for f in sorted(refs.glob('*.dll'))]
args += ['--ref='+str(a.neorune_sdk/'ref'/f)for f in ('NeoRune.Abstractions.dll','NeoRune.Game.dll')]
args += ['--source='+str(r/'src/ui/ModActor.cs')]
rsp=o/'ui-build.rsp';rsp.write_text('\n'.join(args)+'\n')
c=subprocess.run([str(a.dotnet),str(a.neorune_sdk/'tools/neorune.dll'),'@'+str(rsp)],capture_output=True,text=True)
(o/'ui-build.log').write_text(c.stdout+c.stderr)
assert c.returncode==0 and not re.search(r'\berror\s+\w+\d+:',c.stdout+c.stderr,re.I),'NeoRune build diagnostic failure'
print('Build completed. Rebuilt artifacts require gameplay qualification before replacing the published payload.')
